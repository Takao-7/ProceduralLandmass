// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainGeneratorWorker.h"
#include "NoiseGeneratorInterface.h"
#include "HAL/RunnableThread.h"
#include "TerrainGenerator.h"
#include <Kismet/KismetSystemLibrary.h>
#include "UnityLibrary.h"

int32 FTerrainGeneratorWorker::ThreadCounter = 0;


//////////////////////////////////////////////////////
FTerrainGeneratorWorker::FTerrainGeneratorWorker(const FTerrainConfiguration& configuration, UObject* outer)
{
	Configuration = FTerrainConfiguration(configuration, outer);

	Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	bWorkFinished = false;

	const FString threadName = TEXT("Terrain Generator Worker Thread #") + FString::FromInt(GetNewThreadNumber());
	Thread = FRunnableThread::Create(this, *threadName);	
}

FTerrainGeneratorWorker::~FTerrainGeneratorWorker()
{
	ClearJobQueue();
	delete Thread;
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
	bWorkFinished = true;
}

//////////////////////////////////////////////////////
void FTerrainGeneratorWorker::DoWork(FMeshDataJob& currentJob)
{
	const UTerrainChunk* chunk = currentJob.Chunk;
	const int32 levelOfDetail = currentJob.LevelOfDetail;
	const bool bUpdateSection = currentJob.bUpdateMeshSection;
	if (bUpdateSection && chunk->LODMeshes[levelOfDetail] == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("No mesh for requested LOD %d!"), levelOfDetail);
		return;
	}

	const int32 chunkSize = Configuration.GetChunkSize();
	const int32 numVertices = Configuration.GetNumVertices();
	const int32 topLeftX = currentJob.Offset.X - (chunkSize / 2.0f);
	const int32 topLeftY = currentJob.Offset.Y - (chunkSize / 2.0f);

	/* Generate a height map if we need one or update it. */
 	FArray2D* heightMap = bUpdateSection || chunk->HeightMap ? chunk->HeightMap : new FArray2D(numVertices, numVertices);
	UNoiseGeneratorInterface* noiseGenerator = Configuration.NoiseGenerator;
	if (IsValid(noiseGenerator) && (bUpdateSection || chunk->HeightMap == nullptr))
	{
		heightMap->ForEachWithIndex([&](float& value, int32 xIndex, int32 yIndex)
		{
			value = noiseGenerator->GetNoise2D(topLeftX + xIndex, topLeftY + yIndex);
		});
	}
	currentJob.GeneratedHeightMap = heightMap;

	const int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
	const int32 verticesPerLine = (numVertices - 1) / meshSimplificationIncrement + 1;
	TArray<float> borderHeightMap; borderHeightMap.SetNum(verticesPerLine * 4 + 4);

	/* Generate border height map */
	if(IsValid(noiseGenerator))
	{
		const int32 borderTopLeftX = topLeftX - meshSimplificationIncrement;
		const int32 borderTopLeftY = topLeftY - meshSimplificationIncrement;
		int32 vertexIndex = 0;

		/* Top row */
		for (int32 x = 0; x < numVertices + (meshSimplificationIncrement * 2); x += meshSimplificationIncrement)
		{
			borderHeightMap[vertexIndex] = noiseGenerator->GetNoise2D(borderTopLeftX + x, borderTopLeftY);
			vertexIndex++;
		}

		/* Sides */
		for (int32 y = meshSimplificationIncrement; y < numVertices + meshSimplificationIncrement; y += meshSimplificationIncrement)
		{
			borderHeightMap[vertexIndex] = noiseGenerator->GetNoise2D(borderTopLeftX, borderTopLeftY + y);
			vertexIndex++;

			borderHeightMap[vertexIndex] = noiseGenerator->GetNoise2D(borderTopLeftX + (2 * meshSimplificationIncrement) + chunkSize, borderTopLeftY + y);
			vertexIndex++;
		}

		/* Bottom row*/
		for (int32 x = 0; x < numVertices + (meshSimplificationIncrement * 2); x += meshSimplificationIncrement)
		{
			borderHeightMap[vertexIndex] = noiseGenerator->GetNoise2D(borderTopLeftX + x, borderTopLeftY + (chunkSize + (2 * meshSimplificationIncrement)));
			vertexIndex++;
		}
	}

	/* Generate or update mesh data. */
	if (bUpdateSection)
	{
		currentJob.GeneratedMeshData = chunk->LODMeshes[levelOfDetail];
		currentJob.GeneratedMeshData->UpdateMeshData(*heightMap, Configuration.Amplitude, borderHeightMap,Configuration.HeightCurve);
	}
	else
	{
		currentJob.GeneratedMeshData = new FTerrainMeshData(*heightMap, Configuration.Amplitude, levelOfDetail, borderHeightMap, Configuration.HeightCurve, Configuration.MapScale);
	}

	currentJob.DropOffQueue->Enqueue(currentJob);
}

//////////////////////////////////////////////////////
void FTerrainGeneratorWorker::UpdateConfiguration(const FTerrainConfiguration& newConfig)
{
	Configuration.CopyConfiguration(newConfig);
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
