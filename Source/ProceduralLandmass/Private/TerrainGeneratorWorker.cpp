// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainGeneratorWorker.h"
#include "NoiseGeneratorInterface.h"
#include "HAL/RunnableThread.h"
#include "TerrainGenerator.h"

int32 FTerrainGeneratorWorker::ThreadCounter = 0;

FTerrainGeneratorWorker::FTerrainGeneratorWorker()
{
	Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	bKill = false;
	bPause = false;

	const FString threadName = TEXT("Terrain Generator Worker Thread #") + FString::FromInt(GetNewThreadNumber());
	Thread = FRunnableThread::Create(this, *threadName);
}

FTerrainGeneratorWorker::~FTerrainGeneratorWorker()
{
	delete Thread;
	Thread = nullptr;
}

uint32 FTerrainGeneratorWorker::Run()
{
	while(!bKill)
	{
		if (bPause)
		{
			/* This thread will 'sleep' at this line until it is woken. */
			Semaphore->Wait();

			if (bKill) // Check if we should still run when woken up.
			{
				return 0;
			}

			continue;
		}

		FMeshDataJob currentJob;
		const bool bHasJob = PendingJobs.Dequeue(currentJob);
		if (!bHasJob)
		{
			/* No jobs left, we wait a little and then put our self to sleep if we still don't have any job. */
			Semaphore->Wait(1000/60);
			if (PendingJobs.IsEmpty())
			{
				Semaphore->Wait();
			}
			continue;;
		}

		const int32 size = currentJob.ChunkSize;
		const int32 offsetX = currentJob.Offset.X;
		const int32 offsetY = currentJob.Offset.Y;
		FArray2D* heightMap = new FArray2D(size, size);
		heightMap->ForEachWithIndex([&](float& value, int32 xIndex, int32 yIndex)
		{
			UObject* noiseGenerator = currentJob.NoiseGenerator.GetObject();
			value = INoiseGeneratorInterface::Execute_GetNoise2D(noiseGenerator, xIndex + offsetX, yIndex + offsetY);
		});

		currentJob.GeneratedMeshData = new FMeshData(*heightMap, currentJob.HeightMultiplier, currentJob.LevelOfDetail, currentJob.HeightCurve);
		currentJob.GeneratedHeightMap = heightMap;
		
		currentJob.MyGenerator->CriticalSectionMeshDataQueue.Lock();
		currentJob.MyGenerator->FinishedMeshDataJobs.Enqueue(currentJob);
		currentJob.MyGenerator->CriticalSectionMeshDataQueue.Unlock();
	}

	return 0;
}

void FTerrainGeneratorWorker::UnPause()
{
	bPause = false;
	Semaphore->Trigger();
}

void FTerrainGeneratorWorker::ClearJobQueue()
{
	FMeshDataJob job;
	while (PendingJobs.Dequeue(job))
	{
		delete job.GeneratedHeightMap;
		delete job.GeneratedMeshData;
	}
}
