// Fill out your copyright notice in the Description page of Project Settings.

#include "EndlessTerrain.h"
#include "TerrainGenerator.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Containers/List.h"


UEndlessTerrain::UEndlessTerrain()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEndlessTerrain::UpdateVisibleChunks()
{
	for (FTerrainChunk* chunk : TerrainChunksVisibleLastFrame)
	{
		chunk->SetIsVisible(false);
	}	
	TerrainChunksVisibleLastFrame.Empty();	

	const int32 currentChunkCoordX = FMath::RoundToInt(ViewerPosition.X / ChunkSize);
	const int32 currentChunkCoordY = FMath::RoundToInt(ViewerPosition.Y / ChunkSize);

	for (int32 yOffset = -ChunksVisibleInViewDistance; yOffset <= ChunksVisibleInViewDistance; yOffset++)
	{
		for (int32 xOffset = -ChunksVisibleInViewDistance; xOffset <= ChunksVisibleInViewDistance; xOffset++)
		{
			FVector2D viewedChunkCoord = FVector2D(currentChunkCoordX + xOffset, currentChunkCoordY + yOffset);

			if (TerrainChunkDictionary.Contains(viewedChunkCoord))
			{
				FTerrainChunk* chunk = *TerrainChunkDictionary.Find(viewedChunkCoord);
				chunk->UpdateTerrainChunk(ViewerPosition);

				if (chunk->IsVisible())
				{
					TerrainChunksVisibleLastFrame.AddTail(chunk);
				}
			}
			else
			{
				FTerrainChunk* newChunk = new FTerrainChunk(viewedChunkCoord, MaxViewDistance, ChunkSize, GetOwner(), MeshClass);
				TerrainChunkDictionary.Add(viewedChunkCoord, newChunk);
			}
		}
	}
}

void UEndlessTerrain::BeginPlay()
{
	Super::BeginPlay();

	ChunkSize = ATerrainGenerator::MapChunkSize - 1;
	ChunksVisibleInViewDistance = FMath::RoundToInt(MaxViewDistance / ChunkSize);
}

void UEndlessTerrain::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Viewer)
	{
		ViewerPosition = Viewer->GetActorLocation();
	}
	else
	{
		APlayerController* pc = GetWorld()->GetFirstPlayerController();
		FRotator rotation;
		pc->GetPlayerViewPoint(ViewerPosition, rotation);
	}

	UpdateVisibleChunks();
}
