// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/TerrainGenerator.h"
#include "Array2D.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Public/UnityLibrary.h"
#include "PerlinNoiseGenerator.h"
#include "TerrainGeneratorWorker.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Public/TerrainChunk.h"
#include "Kismet/GameplayStatics.h"
#include <Engine/GameInstance.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/Actor.h>


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	bRunConstructionScriptOnDrag = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	NumUnfinishedMeshDataJobs.Set(0);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().ClearTimer(TimerHandleFinishedJobsQueue);
	WorkerThreads.Empty(0);
}

void ATerrainGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandleFinishedJobsQueue);
	WorkerThreads.Empty(0);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::GenerateMap()
{
	NumUnfinishedMeshDataJobs.Set(0);
	GetWorld()->GetTimerManager().SetTimer(TimerHandleFinishedJobsQueue, this, &ATerrainGenerator::DequeAndHandleMeshDataJob, 1/60.0f, true);
	
	const int32 numThreads = Configuration.GetNumberOfThreads();
	const int32 chunksPerDirection = Configuration.NumChunks;	
	const int32 chunkSize = Configuration.GetChunkSize();
	
	/* Create worker threads. */
	WorkerThreads.Empty();
	WorkerThreads.SetNum(numThreads);
	for (int32 i = 0; i < numThreads; i++)
	{
		WorkerThreads[i] = new FTerrainGeneratorWorker();
	}	

	/* Clear all chunks. */
	for (auto& chunk : Chunks)
	{
		if(chunk.Value)
		{
			chunk.Value->DestroyComponent();			
		}
	}
	Chunks.Empty(chunksPerDirection*chunksPerDirection);
	
	/* The top positions for chunks. These are the chunk's relative positions to the terrain generator actor,
	 * measured from their centers. */
	const float topLeftChunkPositionX = ((chunksPerDirection - 1) * chunkSize) / -2.0f;
	const float topLeftChunkPositionY = ((chunksPerDirection - 1) * chunkSize) / 2.0f;

	/* Create mesh data jobs and add them to the worker threads. */
	const int32 levelOfDetail = 0;
	int32 i = 0;
	for (int32 y = 0; y < chunksPerDirection; ++y)
	{
		for (int32 x = 0; x < chunksPerDirection; ++x)
		{
			const FName chunkName = *FString::Printf(TEXT("Terrain chunk %d"), i);
			UTerrainChunk* newChunk = NewObject<UTerrainChunk>(this, chunkName);
			newChunk->InitChunk(this, &Configuration.LODs);
			newChunk->bEnableAutoLODGeneration = true;
			newChunk->bUseAsyncCooking = true;
			newChunk->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			newChunk->SetCollisionResponseToAllChannels(ECR_Block);
			
			const FVector2D chunkPosition = FVector2D(topLeftChunkPositionX + (x * chunkSize), topLeftChunkPositionY - (y * chunkSize));
			newChunk->SetRelativeLocation(FVector(chunkPosition, 0.0f));
			Chunks.Add(FVector2D(chunkPosition), newChunk);

			/* Create a new mesh data jump for the new chunk and assign it to a worker thread. */
			CreateAndEnqueueMeshDataJob(newChunk, levelOfDetail, chunkSize + 1);
			++i;
		}
	}

	SetActorScale3D(Configuration.MapScale);
}

void ATerrainGenerator::UpdateMap()
{

}

void ATerrainGenerator::CreateAndEnqueueMeshDataJob(UTerrainChunk* chunk, int32 levelOfDetail, int32 numVertices, bool bUpdateMeshSection /*= false*/, UObject* noiseGenerator /*= nullptr*/, const FVector2D& offset /*= FVector2D::ZeroVector*/)
{
	FMeshDataJob newJob = FMeshDataJob(noiseGenerator, chunk, Configuration.Amplitude, levelOfDetail, numVertices, bUpdateMeshSection, offset, Configuration.HeightCurve);
	NumUnfinishedMeshDataJobs.Increment();
	
	const int32 numThreads = Configuration.GetNumberOfThreads();
	if (numThreads == 0)
	{
		FTerrainGeneratorWorker::DoWork(newJob);
	}
	else
	{
		if(noiseGenerator)
		{
			/* When in multi-threading mode, we have to create one noise generator object per thread,
			* because they are not thread-safe. */
			UObject* newGenerator = DuplicateObject(noiseGenerator, nullptr);
			newJob.NoiseGenerator = newGenerator;
		}

		static int32 i = 0;
		FTerrainGeneratorWorker* worker = WorkerThreads[i++ % numThreads];
		worker->PendingJobs.Enqueue(newJob);
		worker->UnPause();
	}
}

/////////////////////////////////////////////////////
void ATerrainGenerator::DequeAndHandleMeshDataJob()
{
	FMeshDataJob job;
	if(FinishedMeshDataJobs.Dequeue(job))
	{
		{
			FMeshData* meshData = job.GeneratedMeshData;
			UTerrainChunk* chunk = job.Chunk;

			if (job.bUpdateMeshSection)
			{
				chunk->UpdateMeshSection(0, meshData->Vertices, meshData->Normals, meshData->UVs, meshData->VertexColors, meshData->Tangents);
			}
			else
			{
				const bool bCreateCollision = meshData->LOD == 0; // Only create collision for LOD 0 mesh.
				chunk->CreateMeshSection(0, meshData->Vertices, meshData->Triangles, meshData->Normals, meshData->UVs, meshData->VertexColors, meshData->Tangents, true);
				chunk->Status = EChunkStatus::IDLE;
			}
			chunk->LODMeshes[meshData->LOD] = meshData;
			chunk->HeightMap = job.GeneratedHeightMap;
			
			delete job.NoiseGenerator.GetObject();
		}
	
		const bool bJobsFinished = NumUnfinishedMeshDataJobs.Decrement() == 0;
		if(bJobsFinished)
		{
			UObject* noiseGenerator = Configuration.NoiseGenerator.GetObject();
			if (noiseGenerator)
			{
				const int32 chunksPerDirection = Configuration.NumChunks;
				const int32 chunkSize = Configuration.GetChunkSize();

				/* The top positions for chunks. These are the chunk's relative positions to the terrain generator actor,
				* measured from their centers. */
				const float topLeftChunkPositionX = ((chunksPerDirection - 1) * chunkSize) / -2.0f;
				const float topLeftChunkPositionY = ((chunksPerDirection - 1) * chunkSize) / 2.0f;

				/* The top positions. Used for noise generation. */
				const int32 totalHeight = chunkSize * chunksPerDirection;
				const float topLeftPositionX = (totalHeight) / -2.0f;
				const float topLeftPositionY = (totalHeight) / 2.0f;

				int32 i = 0;
				for (int32 y = 0; y < Configuration.NumChunks; ++y)
				{
					for (int32 x = 0; x < Configuration.NumChunks; ++x)
					{
						const FVector2D chunkPosition = FVector2D(topLeftChunkPositionX + (x * chunkSize), topLeftChunkPositionY - (y * chunkSize));
						UTerrainChunk* chunk = *Chunks.Find(chunkPosition);
						FVector2D offset = FVector2D(topLeftPositionX - x * chunkSize, topLeftPositionY - y * chunkSize );
						CreateAndEnqueueMeshDataJob(chunk, Configuration.LODs[0].LOD, Configuration.GetNumVertices(), true, noiseGenerator, offset);

						++i;
					}
				}
			}
		}		
	}	
}

FVector ATerrainGenerator::GetCameraLocation()
{
	FVector cameraLocation = FVector::ZeroVector;

	APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (cameraManager)
	{
		cameraLocation = cameraManager->GetCameraLocation();
	}
	else
	{
		auto viewLocations = GetWorld()->ViewLocationsRenderedLastFrame;
		if (viewLocations.Num() != 0)
		{
			cameraLocation = viewLocations[0];
		}
	}

	return cameraLocation;
}

/////////////////////////////////////////////////////
void ATerrainGenerator::DrawMap(FArray2D& noiseMap, TArray<FLinearColor> colorMap)
{
	switch (DrawMode)
	{
	case EDrawMode::NoiseMap:
		DrawTexture(TextureFromHeightMap(noiseMap), Configuration.MapScale.X);
		break;
	case EDrawMode::ColorMap:
		DrawTexture(TextureFromColorMap(colorMap), Configuration.MapScale.X);
		break;
	case EDrawMode::Mesh:
	{
		/*SetPlaneVisibility(false);
		FMeshData meshData = FMeshData::GenerateMeshData(noiseMap, MeshHeightMultiplier, EditorPreviewLevelOfDetail, 0, FVector::ZeroVector, MeshHeightCurve);
		UTexture2D* texture = TextureFromColorMap(colorMap);
		DrawMesh(meshData, texture, MeshMaterial, MeshComponent, MapScale);*/
		break;
	}
	}

	noiseMap.~FArray2D();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::DrawTexture(UTexture2D* texture, float targetScale)
{
	/*PreviewPlane->SetMaterial(0, MeshMaterial);
	UMaterialInstanceDynamic* material = PreviewPlane->CreateDynamicMaterialInstance(0);
	if(material)
	{
		material->SetTextureParameterValue("Texture", texture);

		const float textureSize = texture->GetSizeX();
		const float scaleFactor = textureSize / 100.0f;
		PreviewPlane->SetRelativeScale3D(FVector(scaleFactor * targetScale));

		SetPlaneVisibility(true);
	}*/
}

void ATerrainGenerator::DrawMesh(FMeshData& meshData, UTexture2D* texture, UMaterial* material, UProceduralMeshComponent* mesh, float targetScale)
{
	//meshData.CreateMesh(mesh);
	//UMaterialInstanceDynamic* materialInstance = mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, material);
	//materialInstance->SetTextureParameterValue("Texture", texture);
	//
	//mesh->SetRelativeScale3D(FVector(targetScale));
}

/////////////////////////////////////////////////////
UTexture2D* ATerrainGenerator::TextureFromColorMap(const TArray<FLinearColor>& colorMap)
{
	const int32 size = FMath::Sqrt(colorMap.Num());

	UTexture2D* texture = UTexture2D::CreateTransient(size, size, EPixelFormat::PF_A32B32G32R32F); /* Create a texture with a 32 bit pixel format for the Linear Color. */
	texture->Filter = TextureFilter::TF_Nearest;
	texture->AddressX = TextureAddress::TA_Clamp;
	texture->AddressY = TextureAddress::TA_Clamp;
	
	UUnityLibrary::ReplaceTextureData(texture, colorMap, 0);

	return texture;
}

UTexture2D* ATerrainGenerator::TextureFromHeightMap(const FArray2D& heightMap)
{
	const int32 size = heightMap.GetHeight();

	TArray<FLinearColor> colorMap; colorMap.SetNum(size * size);
	for (int32 y = 0; y < size; y++)
	{
		for (int32 x = 0; x < size; x++)
		{
			FLinearColor color = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor::White, heightMap.GetValue(x, y));
			colorMap[y * size + x] = color;
		}
	}

	return TextureFromColorMap(colorMap);
}

