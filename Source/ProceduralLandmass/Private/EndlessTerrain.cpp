// Fill out your copyright notice in the Description page of Project Settings.

#include "EndlessTerrain.h"
#include "PLG_MapGenerator.h"

// Sets default values for this component's properties
UEndlessTerrain::UEndlessTerrain()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UEndlessTerrain::Start()
{
	ChunkSize = APLG_MapGenerator::MapChunkSize;
	ChunksVisibleInViewDistance = FMath::RoundToInt(maxViewDistance / ChunkSize);

}

void UEndlessTerrain::UpdateVisibleChunks()
{
	int32 currentChunkCoordX = FMath::RoundToInt(ViewerPosition.X / ChunkSize);
	int32 currentChunkCoordY = FMath::RoundToInt(ViewerPosition.Y / ChunkSize);

	for (int32 yOffset = -ChunksVisibleInViewDistance; yOffset < ChunksVisibleInViewDistance; yOffset++)
	{
		for (int32 xOffset = -ChunksVisibleInViewDistance; xOffset < ChunksVisibleInViewDistance; xOffset++)
		{
			FVector2D viewedChunkCoord = FVector2D(currentChunkCoordX + xOffset, currentChunkCoordY + yOffset);
			if (TerrainChunkDictionary.Contains(viewedChunkCoord))
			{

			}
			else
			{
				TerrainChunkDictionary.Add(viewedChunkCoord, FTerrainChunk());
			}
		}
	}
}
