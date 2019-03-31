// Fill out your copyright notice in the Description page of Project Settings.

#include "EndlessTerrain.h"
#include "TerrainGenerator.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Containers/List.h"
#include "TerrainChunk.h"


UEndlessTerrain::UEndlessTerrain()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEndlessTerrain::UpdateVisibleChunks()
{
	for (UTerrainChunk* chunk : TerrainChunksVisible)
	{
		const bool bIsVisibleNow = chunk->UpdateTerrainChunk();
		if (!bIsVisibleNow)
		{
			TerrainChunksVisible.RemoveNode(chunk);
		}
	}

	const FVector viewerPosition = GetViewActor()->GetActorLocation();
	const int32 currentChunkCoordX = FMath::RoundToInt(viewerPosition.X / (ChunkSize * 100.0f));
	const int32 currentChunkCoordY = FMath::RoundToInt(viewerPosition.Y / (ChunkSize * 100.0f));

	for (int32 yOffset = -ChunksVisibleInViewDistance; yOffset <= ChunksVisibleInViewDistance; yOffset++)
	{
		for (int32 xOffset = -ChunksVisibleInViewDistance; xOffset <= ChunksVisibleInViewDistance; xOffset++)
		{
			const FVector2D viewedChunkCoord = FVector2D(currentChunkCoordX + xOffset, currentChunkCoordY + yOffset);
			if (TerrainChunkDictionary.Contains(viewedChunkCoord))
			{
				UTerrainChunk* chunk = *TerrainChunkDictionary.Find(viewedChunkCoord);
				chunk->UpdateTerrainChunk();
				if (chunk->IsVisible())
				{
					TerrainChunksVisible.AddTail(chunk);
				}
			}
			else
			{
				FName chunkName = *FString::Printf(TEXT("Chunk %s"), *viewedChunkCoord.ToString());
				UTerrainChunk* newChunk = NewObject<UTerrainChunk>(TerrainGenerator, chunkName);
				newChunk->InitChunk(viewedChunkCoord, MaxViewDistance, ChunkSize, 100.0f, TerrainGenerator, &DetailLevels, GetViewActor());
				TerrainChunkDictionary.Add(viewedChunkCoord, newChunk);
			}
		}
	}
}

void UEndlessTerrain::BeginPlay()
{
	Super::BeginPlay();

	ChunkSize = ATerrainGenerator::MapChunkSize - 1;
	MaxViewDistance = DetailLevels.Last().VisibleDistanceThreshold;
	ChunksVisibleInViewDistance = FMath::RoundToInt(MaxViewDistance / (ChunkSize * 100.0f));
	TerrainGenerator = Cast<ATerrainGenerator>(GetOwner());
}

void UEndlessTerrain::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateVisibleChunks();
}

AActor* UEndlessTerrain::GetViewActor() const
{
	return Viewer ? Viewer : GetWorld()->GetFirstPlayerController();
}
