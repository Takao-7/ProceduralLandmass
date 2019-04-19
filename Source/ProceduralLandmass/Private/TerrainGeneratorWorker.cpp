// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainGeneratorWorker.h"
#include "NoiseGeneratorInterface.h"
#include "HAL/RunnableThread.h"
#include "TerrainGenerator.h"

int32 FTerrainGeneratorWorker::ThreadCounter = 0;

//////////////////////////////////////////////////////
FTerrainGeneratorWorker::FTerrainGeneratorWorker()
{
	Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	bWorkFinished = false;
	bPause = true;

	const FString threadName = TEXT("Terrain Generator Worker Thread #") + FString::FromInt(GetNewThreadNumber());
	Thread = FRunnableThread::Create(this, *threadName);
}

FTerrainGeneratorWorker::~FTerrainGeneratorWorker()
{
	delete Thread;
	Thread = nullptr;
}

//////////////////////////////////////////////////////
uint32 FTerrainGeneratorWorker::Run()
{
	while(!bWorkFinished)
	{
		if (bPause)
		{
			/* This thread will 'sleep' at this line until it is woken. */
			Semaphore->Wait();
			continue;
		}

		FMeshDataJob currentJob;
		const bool bHasJob = PendingJobs.Dequeue(currentJob);
		if (!bHasJob)
		{
			/* No jobs left, we wait a little and then go into sleep if there are still no jobs. */
			Semaphore->Wait(1000);
			if (PendingJobs.IsEmpty())
			{
				Semaphore->Wait();
			}

			continue;
		}

		DoWork(currentJob);
	}

	return 0;
}

void FTerrainGeneratorWorker::Stop()
{
	ClearJobQueue();
}

void FTerrainGeneratorWorker::Exit()
{
	ClearJobQueue();
}

//////////////////////////////////////////////////////
void FTerrainGeneratorWorker::DoWork(FMeshDataJob& currentJob)
{
	const int32 size = currentJob.NumVertices;
	const int32 offsetX = currentJob.Offset.X;
	const int32 offsetY = currentJob.Offset.Y;
	const bool bUpdateSection = currentJob.bUpdateMeshSection;
	const int32 levelOfDetail = currentJob.LevelOfDetail;
	UTerrainChunk* chunk = currentJob.Chunk;

	/* Generate height map */
	FArray2D* heightMap = bUpdateSection ? chunk->HeightMap : new FArray2D(size, size);
	UNoiseGeneratorInterface* noiseGenerator = currentJob.NoiseGenerator;
	if (noiseGenerator)
	{
		heightMap->ForEachWithIndex([&](float& value, int32 xIndex, int32 yIndex)
		{
			value = noiseGenerator->GetNoise2D(xIndex + offsetX, yIndex + offsetY);
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
		currentJob.GeneratedMeshData = new FMeshData(*heightMap, currentJob.HeightMultiplier, levelOfDetail, currentJob.HeightCurve, currentJob.Offset);
		chunk->LODMeshes[levelOfDetail] = currentJob.GeneratedMeshData;
		chunk->HeightMap = heightMap;
		chunk->Status = EChunkStatus::MESH_DATA_REQUESTED;
	}

	currentJob.MyGenerator->FinishedMeshDataJobs.Enqueue(currentJob);
}

//////////////////////////////////////////////////////
void FTerrainGeneratorWorker::UnPause()
{
	bPause = false;
	Semaphore->Trigger();
}

//////////////////////////////////////////////////////
void FTerrainGeneratorWorker::ClearJobQueue()
{
	FMeshDataJob job;
	while (PendingJobs.Dequeue(job))
	{
		delete job.GeneratedHeightMap;
		delete job.GeneratedMeshData;
	}
}
