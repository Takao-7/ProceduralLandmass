// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EndlessTerrain.generated.h"


USTRUCT(BlueprintType)
struct FTerrainChunk
{
	GENERATED_BODY()

public:
	FVector2D Position;

	FTerrainChunk() {};
	FTerrainChunk(FVector2D coord, int32 size)
	{
		Position = coord * size;
		FVector Position = FVector(Position.X, Position.Y, 0.0f);
	}
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROCEDURALLANDMASS_API UEndlessTerrain : public UActorComponent
{
	GENERATED_BODY()

protected:
	const float maxViewDistance = 300.0f;
	FTransform CameraTransform;
	FVector ViewerPosition;

	int32 ChunkSize;
	int32 ChunksVisibleInViewDistance;

	TMap<FVector2D, FTerrainChunk> TerrainChunkDictionary;

public:	
	// Sets default values for this component's properties
	UEndlessTerrain();

	void Start();
	void UpdateVisibleChunks();		
};
