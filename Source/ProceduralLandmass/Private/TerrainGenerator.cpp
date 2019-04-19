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
	RemoveJobQueueTimer();
	ClearThreads();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
	RemoveJobQueueTimer();
	ClearThreads();
}

void ATerrainGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	RemoveJobQueueTimer();
	ClearThreads();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ATerrainGenerator::EditorTick()
{
	/* Update chunk LOD */
	UTerrainChunk::CameraLocation = GetCameraLocation();

	for (const TPair<FVector2D, UTerrainChunk*>& pair : Chunks)
	{
		UTerrainChunk* chunk = pair.Value;
		if (IsValid(chunk))
		{
			chunk->UpdateChunk();
		}
	}

	HandleFinishedMeshDataJobs();
	HandleRequestedMeshDataJobs();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::ClearThreads()
{
	if (UpdateChunkLODThread)
	{
		UpdateChunkLODThread->bShouldRun = false;
	}

	for (FTerrainGeneratorWorker* worker : WorkerThreads)
	{
		if (worker)
		{
			worker->Stop();
		}
	}
	WorkerThreads.Empty(0);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::GenerateMap()
{
	RemoveJobQueueTimer();
	ClearThreads();
	NumUnfinishedMeshDataJobs.Set(0);

	UTerrainChunk::CameraLocation = GetCameraLocation();

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
	UNoiseGeneratorInterface* noiseGenerator = Configuration.NoiseGenerator;
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
			Chunks.Add(FVector2D(chunkPosition), newChunk);
			
			/* Chunk location in world space. */
			const FVector chunkLocation = GetTransform().TransformPosition(chunkPosition);
			const float distanceToChunk = FVector::Dist(chunkLocation, UTerrainChunk::CameraLocation) - Configuration.GetChunkSize() * 50.0f;
			const int32 levelOfDetail = FLODInfo::FindLOD(Configuration.LODs, distanceToChunk);
			
			CreateAndEnqueueMeshDataJob(newChunk, levelOfDetail, chunkSize + 1, false, noiseGenerator, noiseOffset);
			++i;
			
			//UKismetSystemLibrary::DrawDebugSphere(this, chunkLocation, 1000.0f, 12, FLinearColor::Red, 30.0f, 10.0f);
			//UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("Distance to chunk: %f"), distanceToChunk), false);
		}
	}

	SetActorScale3D(FVector(Configuration.MapScale));
	//StartJobQueueTimer();
	GetWorld()->GetTimerManager().SetTimer(TimerHandleUpdateChunkLOD, this, &ATerrainGenerator::EditorTick, 1 / 60.0f, true);
	//UpdateChunkLODThread = new FUpdateChunkLODThread(this, Chunks);
}

void ATerrainGenerator::UpdateMap()
{
	UpdateAllChunks();
	StartJobQueueTimer();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::CreateAndEnqueueMeshDataJob(UTerrainChunk* chunk, int32 levelOfDetail, int32 numVertices, bool bUpdateMeshSection /*= false*/, 
	UNoiseGeneratorInterface* noiseGenerator /*= nullptr*/, const FVector2D& noiseOffset /*= FVector2D::ZeroVector*/)
{
	FMeshDataJob newJob = FMeshDataJob(noiseGenerator, chunk, Configuration.Amplitude, levelOfDetail, numVertices, bUpdateMeshSection, noiseOffset, Configuration.HeightCurve);
	NumUnfinishedMeshDataJobs.Increment();
	
	const int32 numThreads = Configuration.GetNumberOfThreads();
	if (numThreads == 0)
	{
		FTerrainGeneratorWorker::DoWork(newJob);
	}
	else
	{
		static int32 i = 0;
		FTerrainGeneratorWorker* worker = WorkerThreads[i % numThreads];

		if(noiseGenerator)
		{
			/* When in multi-threading mode, we have to create one noise generator object per thread,
			* because they are not thread-safe. */
			newJob.NoiseGenerator = DuplicateObject<UNoiseGeneratorInterface>(noiseGenerator, nullptr);
		}

		worker->PendingJobs.Enqueue(newJob);
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
	const bool bHasJob = FinishedMeshDataJobs.Dequeue(job);
	if (bHasJob)
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
			const bool bCreateCollision = lod == 0; // Only create collision for LOD 0 mesh.
			chunk->CreateMeshSection(lod, meshData->Vertices, meshData->Triangles, meshData->Normals, meshData->UVs, meshData->VertexColors, meshData->Tangents, bCreateCollision);
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

void ATerrainGenerator::HandleRequestedMeshDataJobs()
{
	FMeshDataRequest meshDataRequest;
	const bool bMeshRequest = RequestedMeshDataJobs.Dequeue(meshDataRequest);
	if (bMeshRequest)
	{
		CreateAndEnqueueMeshDataJob(meshDataRequest.Chunk, meshDataRequest.LOD, false, meshDataRequest.Chunk->NoiseOffset);
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


/////////////////////////////////////////////////////
			/* Update chunk LOD thread */
/////////////////////////////////////////////////////
FUpdateChunkLODThread::FUpdateChunkLODThread(ATerrainGenerator* terrainGenerator, const TMap<FVector2D, UTerrainChunk*>& chunks)
	: TerrainGenerator(terrainGenerator), Chunks(chunks)
{
	Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	CameraLocation = terrainGenerator->GetCameraLocation();
	LastCameraLocation = CameraLocation;

	const FString threadName = TEXT("Update chunk LOD thread");
	Thread = FRunnableThread::Create(this, *threadName);
}

FUpdateChunkLODThread::~FUpdateChunkLODThread()
{
	delete Thread;
	Thread = nullptr;
}

uint32 FUpdateChunkLODThread::Run()
{
	while (bShouldRun)
	{
		bShouldRun = bShouldRun && IsValid(TerrainGenerator) && IsValid(TerrainGenerator->GetWorld());
		if (!bShouldRun)
		{
			break;
		}

		LastCameraLocation = CameraLocation;
		CameraLocation = UTerrainChunk::CameraLocation;

		const float movedDistanceSqrt = FVector::DistSquared(LastCameraLocation, CameraLocation);
		const bool bHasMoved = movedDistanceSqrt >= 1.0f/* && CameraLocation != FVector::ZeroVector && LastCameraLocation != FVector::ZeroVector*/;
		if (bHasMoved && movedDistanceSqrt > MoveThreshold * MoveThreshold)
		{
			for (const TPair<FVector2D, UTerrainChunk*>& pair : Chunks)
			{
				UTerrainChunk* chunk = pair.Value;
				if (IsValid(chunk))
				{
					chunk->UpdateChunk();
				}

				if (!bShouldRun)
				{
					break;
				}
			}
		}

		if (bShouldRun)	// Only wait if we should run at all.
		{
			Semaphore->Wait(1 / 60.0f);
		}
	}

	return 0;
}