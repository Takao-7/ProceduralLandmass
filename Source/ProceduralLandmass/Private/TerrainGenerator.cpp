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
#include <RunnableThread.h>


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	bRunConstructionScriptOnDrag = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	NumUnfinishedMeshDataJobs.Set(0);
}

ATerrainGenerator::~ATerrainGenerator()
{
	ClearTimers();
	ClearThreads();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
}

void ATerrainGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);	
}

void ATerrainGenerator::EditorTick()
{
	UTerrainChunk::LastCameraLocation = UTerrainChunk::CameraLocation;
	UTerrainChunk::CameraLocation = GetCameraLocation();
	//float distanceMoved = FVector::Dist(UTerrainChunk::LastCameraLocation, UTerrainChunk::CameraLocation);

	//if (!FMath::IsNearlyZero(distanceMoved, 1.0f))
	//{
	//	/* Update chunk LOD */
	//	for (const TPair<FVector2D, UTerrainChunk*>& pair : Chunks)
	//	{
	//		UTerrainChunk* chunk = pair.Value;
	//		if (IsValid(chunk))
	//		{
	//			chunk->UpdateChunk();
	//		}
	//	}
	//}

	/* Update chunk LOD */
	for (const TPair<FVector2D, UTerrainChunk*>& pair : Chunks)
	{
		UTerrainChunk* chunk = pair.Value;
		if (IsValid(chunk))
		{
			chunk->UpdateChunk();
		}
	}
		
	HandleFinishedMeshDataJobs();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::ClearThreads()
{
	for (FTerrainGeneratorWorker* worker : WorkerThreads)
	{
		if (worker)
		{
			worker->Stop();
		}
	}
	WorkerThreads.Empty(0);
}

void ATerrainGenerator::ClearTimers()
{
	UWorld* world = GetWorld();
	if (world)
	{
		world->GetTimerManager().ClearAllTimersForObject(this);
	}
}

void ATerrainGenerator::ClearTerrain()
{
	ClearTimers();
	ClearThreads();
	NumUnfinishedMeshDataJobs.Set(0);

	/* Clear all chunks. */
	for (auto& chunk : Chunks)
	{
		if (chunk.Value)
		{
			chunk.Value->DestroyComponent();
		}
	}

	Chunks.Empty();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::GenerateTerrain()
{
	ClearTerrain();

	SetActorScale3D(FVector(Configuration.MapScale));
	Configuration.InitLODs();

	const int32 numThreads = Configuration.GetNumberOfThreads();
	const int32 chunksPerDirection = Configuration.NumChunks;	
	const int32 chunkSize = Configuration.GetChunkSize();
	
	/*if (Configuration.bUseFalloffMap && Configuration.FalloffMap == nullptr)
	{
		const int32 falloffMapSize = Configuration.bFalloffMapPerChunk ? chunkSize : chunkSize * chunksPerDirection;
		Configuration.FalloffMap = UUnityLibrary::GenerateFalloffMap(falloffMapSize);
	}*/
	
	/* Create worker threads. */	
	WorkerThreads.SetNum(numThreads);
	for (int32 i = 0; i < numThreads; i++)
	{
		WorkerThreads[i] = new FTerrainGeneratorWorker();
	}	
	
	/* The top positions for chunks. These are the chunk's relative positions to the terrain generator actor,
	 * measured from their centers. */
	const float topLeftChunkPositionX = ((chunksPerDirection - 1) * chunkSize) / -2.0f;
	const float topLeftChunkPositionY = ((chunksPerDirection - 1) * chunkSize) / 2.0f;

	/* Create mesh data jobs and add them to the worker threads. */
	UNoiseGeneratorInterface* noiseGenerator = Configuration.NoiseGenerator;
	const FVector cameraLocation = GetCameraLocation();
	int32 i = 0;
	for (int32 y = 0; y < chunksPerDirection; ++y)
	{
		for (int32 x = 0; x < chunksPerDirection; ++x)
		{
			const FVector2D noiseOffset = CalculateNoiseOffset(x, y);

			const FName chunkName = *FString::Printf(TEXT("Terrain chunk %d"), i);
			UTerrainChunk* newChunk = NewObject<UTerrainChunk>(this, chunkName);
			newChunk->InitChunk(this, &Configuration.LODs, noiseOffset);
			newChunk->bEnableAutoLODGeneration = true;
			newChunk->bUseAsyncCooking = true;
			newChunk->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			newChunk->SetCollisionResponseToAllChannels(ECR_Block);
			
			/* Chunk position is relative to the whole terrain actor. */
			const FVector chunkPosition = FVector(topLeftChunkPositionX + (x * chunkSize), topLeftChunkPositionY - (y * chunkSize), 0.0f);
			newChunk->SetRelativeLocation(chunkPosition);
			newChunk->SetChunkBoundingBox();
			Chunks.Add(FVector2D(chunkPosition), newChunk);
			
			/* Chunk location is in world space. */
			const FVector chunkLocation = GetTransform().TransformPosition(chunkPosition);
			const float distanceToChunk = FVector::Dist(chunkLocation, cameraLocation) - Configuration.GetChunkSize() * 50.0f;
			const int32 levelOfDetail = FLODInfo::FindLOD(Configuration.LODs, distanceToChunk);
			
			CreateAndEnqueueMeshDataJob(newChunk, levelOfDetail, chunkSize + 1, false, noiseGenerator, noiseOffset);
			newChunk->Status = EChunkStatus::MESH_DATA_REQUESTED;
			++i;
		}
	}
	
	OldConfiguration = Configuration;
	GetWorld()->GetTimerManager().SetTimer(TimerHandleEditorTick, this, &ATerrainGenerator::EditorTick, 1 / 60.0f, true);
}

void ATerrainGenerator::UpdateMap()
{
	if (Configuration == OldConfiguration)
	{
		return;
	}
	
	if (Configuration.NumChunks != OldConfiguration.NumChunks)
	{
		GenerateTerrain();
		return;
	}

	UpdateAllChunks();

	OldConfiguration = Configuration;
}

/////////////////////////////////////////////////////
void ATerrainGenerator::CreateAndEnqueueMeshDataJob(UTerrainChunk* chunk, int32 levelOfDetail, int32 numVertices, bool bUpdateMeshSection /*= false*/, 
	UNoiseGeneratorInterface* noiseGenerator /*= nullptr*/, const FVector2D& noiseOffset /*= FVector2D::ZeroVector*/)
{
	FMeshDataJob newJob = FMeshDataJob(noiseGenerator, chunk, Configuration.Amplitude, levelOfDetail, numVertices, bUpdateMeshSection, noiseOffset, 
		Configuration.HeightCurve, Configuration.bUseFalloffMap);
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
			newJob.NoiseGenerator = DuplicateObject<UNoiseGeneratorInterface>(noiseGenerator, nullptr);
		}

		static int32 i = 0;
		FTerrainGeneratorWorker* worker = WorkerThreads[i % numThreads];
		levelOfDetail == 0 ? worker->PriorityJobs.Enqueue(newJob) : worker->PendingJobs.Enqueue(newJob);
		worker->UnPause();
		++i;
	}
}

void ATerrainGenerator::CreateAndEnqueueMeshDataJob(UTerrainChunk* chunk, int32 levelOfDetail, bool bUpdateMeshSection /*= false*/, const FVector2D& offset /*= FVector2D::ZeroVector*/)
{
	CreateAndEnqueueMeshDataJob(chunk, levelOfDetail, Configuration.GetNumVertices(), bUpdateMeshSection, Configuration.NoiseGenerator, offset);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::HandleFinishedMeshDataJobs()
{
	FMeshDataJob job;
	while (FinishedMeshDataJobs.Dequeue(job))
	{
		FMeshData* meshData = job.GeneratedMeshData;
		UTerrainChunk* chunk = job.Chunk;
		const int32 lod = meshData->LOD;

		if (job.bUpdateMeshSection)
		{
			chunk->UpdateMeshSection(lod, meshData->Vertices, meshData->Normals, meshData->UVs, meshData->VertexColors, meshData->Tangents);
		}
		else
		{
			chunk->CreateMeshSection(lod, meshData->Vertices, meshData->Triangles, meshData->Normals, meshData->UVs, meshData->VertexColors, meshData->Tangents, false);
			chunk->LODMeshes[lod] = meshData;
			chunk->HeightMap = job.GeneratedHeightMap;

			/*if (lod == 0)
			{
				for (int32 i = 0; i < meshData->Vertices.Num(); i+=4)
				{
					FVector position = meshData->Vertices[i];
					position = chunk->GetComponentTransform().TransformPosition(position);
					UKismetSystemLibrary::DrawDebugArrow(this, position, position + meshData->Normals[i] * 100.0f, 10.0f, FLinearColor::Red, 100.0f, 5.0f);
				}
			}*/
		}

		chunk->SetNewLOD(lod);
		chunk->Status = EChunkStatus::IDLE;

		if (IsValid(job.NoiseGenerator))
		{
			delete job.NoiseGenerator;
		}

		NumUnfinishedMeshDataJobs.Decrement();
	}
}

/////////////////////////////////////////////////////
void ATerrainGenerator::UpdateAllChunks(int32 levelOfDetail /*= 0*/)
{
	const int32 chunksPerDirection = Configuration.NumChunks;
	const int32 chunkSize = Configuration.GetChunkSize();
	UNoiseGeneratorInterface* noiseGenerator = Configuration.NoiseGenerator;

	/* The top positions for chunks. These are the chunk's relative positions to the terrain generator actor,
	 * measured from their centers. */
	const float topLeftChunkPositionX = ((chunksPerDirection - 1) * chunkSize) / -2.0f;
	const float topLeftChunkPositionY = ((chunksPerDirection - 1) * chunkSize) / 2.0f;

	int32 i = 0;
	for (int32 y = 0; y < Configuration.NumChunks; ++y)
	{
		for (int32 x = 0; x < Configuration.NumChunks; ++x)
		{
			const FVector2D chunkPosition = FVector2D(topLeftChunkPositionX + (x * chunkSize), topLeftChunkPositionY - (y * chunkSize));
			UTerrainChunk** chunkPointer = Chunks.Find(chunkPosition);
			if (chunkPointer == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("Chunk position '%s' not viable."), *chunkPosition.ToString());
				continue;
			}

			UTerrainChunk* chunk = *chunkPointer;
			const FVector2D offset = CalculateNoiseOffset(x, y);
			CreateAndEnqueueMeshDataJob(chunk, levelOfDetail, Configuration.GetNumVertices(), true, noiseGenerator, offset);
			++i;
		}
	}
}

/////////////////////////////////////////////////////
FVector2D ATerrainGenerator::CalculateNoiseOffset(int32 x, int32 y)
{
	const int32 chunksPerDirection = Configuration.NumChunks;
	const int32 chunkSize = Configuration.GetChunkSize();

	/* The top positions. Used for noise generation. */
	const int32 totalHeight = chunkSize * chunksPerDirection;
	const float topLeftPositionX = totalHeight - 1 / -2.0f;
	const float topLeftPositionY = totalHeight - 1 / 2.0f;

	return FVector2D(topLeftPositionX + x * chunkSize, topLeftPositionY + y * chunkSize);
}

/////////////////////////////////////////////////////
FVector ATerrainGenerator::GetCameraLocation()
{
	FVector cameraLocation = FVector::ZeroVector;

	const APlayerCameraManager* cameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (IsValid(cameraManager))
	{
		cameraLocation = cameraManager->GetCameraLocation();
	}
	else
	{
		const UWorld* world = GetWorld();
		if (IsValid(world))
		{
			auto viewLocations = world->ViewLocationsRenderedLastFrame;
			if (viewLocations.Num() != 0)
			{
				cameraLocation = viewLocations[0];
			}			
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
		DrawTexture(TextureFromHeightMap(noiseMap), Configuration.MapScale);
		break;
	case EDrawMode::ColorMap:
		DrawTexture(TextureFromColorMap(colorMap), Configuration.MapScale);
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