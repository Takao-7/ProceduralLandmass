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
	delete Thread;
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
	const UTerrainChunk* chunk = currentJob.Chunk;
	const int32 levelOfDetail = currentJob.LevelOfDetail;
	const bool bUpdateSection = currentJob.bUpdateMeshSection;
	if (bUpdateSection && chunk->LODMeshes[levelOfDetail] == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("No mesh for requested LOD %d!"), levelOfDetail);
		return;
	}

	const int32 numVertices = Configuration.GetNumVertices();
	const int32 offsetX = currentJob.Offset.X;
	const int32 offsetY = currentJob.Offset.Y;

	/* Generate a height map if we need one or update it. */
	FArray2D* heightMap = bUpdateSection || chunk->HeightMap ? chunk->HeightMap : new FArray2D(numVertices, numVertices);
	UNoiseGeneratorInterface* noiseGenerator = Configuration.NoiseGenerator;
	if (IsValid(noiseGenerator) && (bUpdateSection || chunk->HeightMap == nullptr))
	{
		const bool bUseFalloffPerChunk = Configuration.bFalloffMapPerChunk;
		const int32 falloffMapSize = bUseFalloffPerChunk ? numVertices : numVertices * Configuration.NumChunks;
		heightMap->ForEachWithIndex([&](float& value, int32 xIndex, int32 yIndex)
		{
			value = noiseGenerator->GetNoise2D(xIndex + offsetX, yIndex + offsetY);

			if (Configuration.bUseFalloffMap)
			{
				value -= UUnityLibrary::GetValueWithFalloff(xIndex + offsetX, yIndex + offsetY, falloffMapSize);
				value = FMath::Clamp(value, 0.0f, 1.0f);
			}
		});
	}
	currentJob.GeneratedHeightMap = heightMap;

	const int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
	const int32 verticesPerLine = (numVertices - 1) / meshSimplificationIncrement + 1;
	TArray<float> borderHeightMap; borderHeightMap.SetNum(verticesPerLine * 4 + 4);
	if (IsValid(noiseGenerator))
	{
		int32 vertexIndex = 0;
		const int32 chunkSize = Configuration.GetChunkSize();
		const int32 borderOffsetX = offsetX - meshSimplificationIncrement;
		const int32 borderOffsetY = offsetY + meshSimplificationIncrement;

		/* Top row */
		for (int32 x = 0; x < numVertices + meshSimplificationIncrement * 2; x += meshSimplificationIncrement)
		{
			borderHeightMap[vertexIndex] = noiseGenerator->GetNoise2D(x + borderOffsetX, borderOffsetY);
			++vertexIndex;
		}

		/* Sides */
		for (int32 y = meshSimplificationIncrement; y <= numVertices; y += meshSimplificationIncrement)
		{	
			borderHeightMap[vertexIndex++] = noiseGenerator->GetNoise2D(borderOffsetX, y + borderOffsetY);
			borderHeightMap[vertexIndex++] = noiseGenerator->GetNoise2D(chunkSize + (2 * meshSimplificationIncrement) + borderOffsetX, y + borderOffsetY);
		}

		/* Bottom row*/
		for (int32 x = 0; x < numVertices + meshSimplificationIncrement * 2; x += meshSimplificationIncrement)
		{
			borderHeightMap[vertexIndex] = noiseGenerator->GetNoise2D(x + borderOffsetX, borderOffsetY + chunkSize + (2 * meshSimplificationIncrement));
			++vertexIndex;
		}
	}

	/* Generate or update mesh data. */
	if (bUpdateSection)
	{
		currentJob.GeneratedMeshData = chunk->LODMeshes[levelOfDetail];
		currentJob.GeneratedMeshData->UpdateMeshData(*heightMap, Configuration.Amplitude, Configuration.HeightCurve);
	}
	else
	{
		currentJob.GeneratedMeshData = new FMeshData(*heightMap, Configuration.Amplitude, levelOfDetail, &borderHeightMap, Configuration.HeightCurve, Configuration.MapScale);
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
