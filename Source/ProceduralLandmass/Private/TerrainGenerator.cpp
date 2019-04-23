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
	
	const float distanceMoved = FVector::Dist(UTerrainChunk::LastCameraLocation, UTerrainChunk::CameraLocation);

	if (!FMath::IsNearlyZero(distanceMoved, 1.0f))
	{
		/* Update chunk LOD */
		for (const TPair<FVector2D, UTerrainChunk*>& pair : Chunks)
		{
			UTerrainChunk* chunk = pair.Value;
			if (IsValid(chunk))
			{
				chunk->UpdateChunk();
			}
		}
	}

	/* Update chunk LOD */
	/*for (const TPair<FVector2D, UTerrainChunk*>& pair : Chunks)
	{
		UTerrainChunk* chunk = pair.Value;
		if (IsValid(chunk))
		{
			chunk->UpdateChunk();
		}
	}*/
		
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
	
	/* Create worker threads. */	
	WorkerThreads.SetNum(numThreads);
	for (int32 i = 0; i < numThreads; i++)
	{
		WorkerThreads[i] = new FTerrainGeneratorWorker(Configuration, this);
	}	
	
	/* The top positions for chunks. These are the chunk's relative positions to the terrain generator actor,
	 * measured from their centers. */
	const float topLeftChunkPositionX = ((chunksPerDirection - 1) * chunkSize) / -2.0f;
	const float topLeftChunkPositionY = ((chunksPerDirection - 1) * chunkSize) / 2.0f;

	/* Create mesh data jobs and add them to the worker threads. */
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
			
			const int32 levelOfDetail = newChunk->GetOptimalLOD(cameraLocation);			
			CreateAndEnqueueMeshDataJob(newChunk, levelOfDetail, false, noiseOffset);
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
void ATerrainGenerator::CreateAndEnqueueMeshDataJob(UTerrainChunk* chunk, int32 levelOfDetail, bool bUpdateMeshSection /*= false*/, const FVector2D& noiseOffset /*= FVector2D::ZeroVector*/)
{
	FMeshDataJob newJob = FMeshDataJob(chunk, &FinishedMeshDataJobs, levelOfDetail, bUpdateMeshSection, noiseOffset);

	static int32 i = 0;
	FTerrainGeneratorWorker* worker = WorkerThreads[i % Configuration.GetNumberOfThreads()];
	levelOfDetail == 0 ? worker->PriorityJobs.Enqueue(newJob) : worker->PendingJobs.Enqueue(newJob);
	worker->UnPause();
	++i;
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
		}

		chunk->SetMaterial(lod, MeshMaterial);
		chunk->SetNewLOD(lod);
		chunk->Status = EChunkStatus::IDLE;
	}
}

/////////////////////////////////////////////////////
void ATerrainGenerator::UpdateAllChunks()
{
	for (FTerrainGeneratorWorker* worker : WorkerThreads)
	{
		if (worker)
		{
			worker->UpdateConfiguration(Configuration);
		}
	}
	
	const int32 chunksPerDirection = Configuration.NumChunks;
	const int32 chunkSize = Configuration.GetChunkSize();
	const FVector cameraLocation = GetCameraLocation();

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
			
			/* Update all LOD meshes. */
			for (int32 lod = 0; lod < chunk->LODMeshes.Num(); ++lod)
			{
				const FMeshData* data = chunk->LODMeshes[lod];
				if (data)
				{
					CreateAndEnqueueMeshDataJob(chunk, lod, true, offset);
				}
			}
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
