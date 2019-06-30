// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/TerrainGenerator.h"
#include "Array2D.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Public/UnityLibrary.h"
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

	ClearThreads();
	ClearTimers();
}
	
/////////////////////////////////////////////////////
void ATerrainGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);	
	UpdateChunkLOD();
}
	
void ATerrainGenerator::EditorTick()
{
	UpdateChunkLOD();			
	HandleFinishedMeshDataJobs();
}

void ATerrainGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bGenerateTerrain)
	{
		bGenerateTerrain = false;
		GenerateTerrain();
	}
	else if (bClearTerrain)
	{
		bClearTerrain = false;
		ClearTerrain();
	}
	else if (bUpdateTerrain || bAutoUpdate)
	{
		bUpdateTerrain = false;
		UpdateTerrain();
	}
}

/////////////////////////////////////////////////////
void ATerrainGenerator::UpdateChunkLOD()
{
	UTerrainChunk::LastCameraLocation = UTerrainChunk::CameraLocation;
	UTerrainChunk::CameraLocation = UUnityLibrary::GetCameraLocation(this);

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
}

/////////////////////////////////////////////////////
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

void ATerrainGenerator::ClearThreads()
{
	for (int32 i = 0; i < WorkerThreads.Num(); i++)
	{
		if (WorkerThreads[i])
		{
			WorkerThreads[i]->Stop();
			WorkerThreads[i] = nullptr;
		}
	}
}
	
void ATerrainGenerator::ClearTimers()
{
	UWorld* world = GetWorld();
	if (world)
	{
		world->GetTimerManager().ClearAllTimersForObject(this);
	}
}

/////////////////////////////////////////////////////
void ATerrainGenerator::GenerateTerrain()
{
	ClearTerrain();
	
	SetActorScale3D(FVector(Configuration.MapScale));
	Configuration.InitLODs();
	if (Configuration.NoiseGeneratorClass)
	{
		Configuration.NoiseGenerator = NewObject<UNoiseGenerator>(this, Configuration.NoiseGeneratorClass);
	}
	
	const int32 numThreads = Configuration.GetNumberOfThreads();
	const int32 chunksPerDirection = Configuration.NumChunks;	
	const int32 chunkSize = Configuration.GetChunkSize();
		
	/* Create worker threads. */	
	WorkerThreads.SetNum(numThreads);
	for (int32 i = 0; i < numThreads; i++)
	{
		WorkerThreads[i] = new FTerrainGeneratorWorker(Configuration, GetWorld());
	}	
		
	/* The top positions for chunks. These are the chunk's relative positions to the terrain generator actor,
	 * measured from their centers. */
	const float topLeftChunkPositionX = ((chunksPerDirection - 1) * chunkSize) / -2.0f;
	const float topLeftChunkPositionY = ((chunksPerDirection - 1) * chunkSize) / -2.0f;
	
	/* Create mesh data jobs and add them to the worker threads. */
	const FVector cameraLocation = UUnityLibrary::GetCameraLocation(this);
	int32 i = 0;
	for (int32 y = 0; y < chunksPerDirection; ++y)
	{
		for (int32 x = 0; x < chunksPerDirection; ++x)
		{
			/* Chunk position is relative to the whole terrain actor. */
			const FVector chunkPosition = FVector(topLeftChunkPositionX + (x * chunkSize), topLeftChunkPositionY + (y * chunkSize), 0.0f);
			const FVector2D noiseOffset = FVector2D(chunkPosition);
	
			const FName chunkName = *FString::Printf(TEXT("Terrain chunk %d"), i);
			UTerrainChunk* newChunk = NewObject<UTerrainChunk>(this, chunkName);
			newChunk->InitChunk(this, &Configuration.LODs);
			newChunk->bEnableAutoLODGeneration = true;
			newChunk->bUseAsyncCooking = true;
			newChunk->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			newChunk->SetCollisionResponseToAllChannels(ECR_Block);
				
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
	GetWorld()->GetTimerManager().SetTimer(THEditorTick, this, &ATerrainGenerator::EditorTick, 1 / 60.0f, true);
}
	
void ATerrainGenerator::UpdateTerrain()
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
	
	/* Update the configurations for all worker threads. */
	for (FTerrainGeneratorWorker* worker : WorkerThreads)
	{
		if (worker)
		{
			worker->UpdateConfiguration(Configuration, this);
		}
	}
	
	const int32 chunksPerDirection = Configuration.NumChunks;
	const int32 chunkSize = Configuration.GetChunkSize();
	const FVector cameraLocation = UUnityLibrary::GetCameraLocation(this);

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

			/* Update all LOD meshes. */
			for (int32 lod = 0; lod < chunk->LODMeshes.Num(); ++lod)
			{
				const FTerrainMeshData* data = chunk->LODMeshes[lod];
				if (data)
				{
					CreateAndEnqueueMeshDataJob(chunk, lod, true, chunkPosition);
				}
			}
			++i;
		}
	}
	
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
		FTerrainMeshData* meshData = job.GeneratedMeshData;
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
	
		chunk->SetMaterial(lod, TerrainMaterial);
		chunk->SetNewLOD(lod);
		chunk->Status = EChunkStatus::IDLE;
	}
}
