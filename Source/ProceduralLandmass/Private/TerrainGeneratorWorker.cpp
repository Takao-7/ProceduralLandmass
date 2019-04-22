// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainGeneratorWorker.h"
#include "NoiseGeneratorInterface.h"
#include "HAL/RunnableThread.h"
#include "TerrainGenerator.h"
#include <Kismet/KismetSystemLibrary.h>
#include "UnityLibrary.h"

int32 FTerrainGeneratorWorker::ThreadCounter = 0;

//////////////////////////////////////////////////////
FTerrainGeneratorWorker::FTerrainGeneratorWorker()
{
	Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	bWorkFinished = false;

	const FString threadName = TEXT("Terrain Generator Worker Thread #") + FString::FromInt(GetNewThreadNumber());
	Thread = FRunnableThread::Create(this, *threadName);
}

FTerrainGeneratorWorker::~FTerrainGeneratorWorker()
{
	delete Thread;
	Thread = nullptr;
	ClearJobQueue();
}

//////////////////////////////////////////////////////
uint32 FTerrainGeneratorWorker::Run()
{
	while(!bWorkFinished)
	{
		FMeshDataJob currentJob;
		while (!bWorkFinished && !bPause && (PriorityJobs.Dequeue(currentJob) || PendingJobs.Dequeue(currentJob)))
		{
			DoWork(currentJob);
		}

		if(!bWorkFinished)
		{
			Semaphore->Wait();
		}
	}

	return 0;
}

void FTerrainGeneratorWorker::Stop()
{
	bPause = true;
	bWorkFinished = true;
}

//////////////////////////////////////////////////////
void FTerrainGeneratorWorker::DoWork(FMeshDataJob& currentJob)
{
	UTerrainChunk* chunk = currentJob.Chunk;
	const int32 levelOfDetail = currentJob.LevelOfDetail;
	const bool bUpdateSection = currentJob.bUpdateMeshSection;
	if (bUpdateSection && chunk->LODMeshes[levelOfDetail] == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("No mesh for requested LOD %d!"), levelOfDetail);
		return;
	}

	const int32 size = currentJob.NumVertices;
	const int32 offsetX = currentJob.Offset.X;
	const int32 offsetY = currentJob.Offset.Y;

	/* Generate height map */
	FArray2D* heightMap = bUpdateSection ? chunk->HeightMap : new FArray2D(size, size);
	UNoiseGeneratorInterface* noiseGenerator = currentJob.NoiseGenerator;
	if (IsValid(noiseGenerator))
	{
		const bool bUseFalloffPerChunk = currentJob.MyGenerator->Configuration.bFalloffMapPerChunk;
		const int32 falloffMapSize = bUseFalloffPerChunk ? currentJob.NumVertices : currentJob.NumVertices * currentJob.MyGenerator->Configuration.NumChunks;
		heightMap->ForEachWithIndex([&](float& value, int32 xIndex, int32 yIndex)
		{
			if (IsValid(noiseGenerator))
			{
				float fallOff = 0;
				if (currentJob.bUseFalloffMap)
				{
					fallOff = UUnityLibrary::GetValueWithFalloff(xIndex + offsetX, yIndex + offsetY, falloffMapSize);
				}

				value = noiseGenerator->GetNoise2D(xIndex + offsetX, yIndex + offsetY) - fallOff;
				value = FMath::Clamp(value, 0.0f, 1.0f);
			}
		});
	}
	currentJob.GeneratedHeightMap = heightMap;

	/* Generate or update mesh data. */
	if (bUpdateSection)
	{
		currentJob.GeneratedMeshData = chunk->LODMeshes[levelOfDetail];
		currentJob.GeneratedMeshData->UpdateMeshData(*heightMap, currentJob.HeightMultiplier, currentJob.HeightCurve);
	}
	else
	{
		const float mapScale = currentJob.MyGenerator->Configuration.MapScale;
		currentJob.GeneratedMeshData = new FMeshData(*heightMap, currentJob.HeightMultiplier, levelOfDetail, currentJob.HeightCurve, mapScale);
	}

	currentJob.MyGenerator->FinishedMeshDataJobs.Enqueue(currentJob);
}

//////////////////////////////////////////////////////
void FTerrainGeneratorWorker::ClearJobQueue()
{
	FMeshDataJob job;
	while (PendingJobs.Dequeue(job) || PriorityJobs.Dequeue(job))
	{
		delete job.GeneratedHeightMap;
		delete job.GeneratedMeshData;
	}
}
