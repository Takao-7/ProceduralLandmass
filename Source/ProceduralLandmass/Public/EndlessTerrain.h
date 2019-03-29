// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Structs/TerrainChunk.h"
#include "MeshData.h"
#include "EndlessTerrain.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROCEDURALLANDMASS_API UEndlessTerrain : public UActorComponent
{
	GENERATED_BODY()


protected:
	UPROPERTY(BlueprintReadWrite, Category = "Endless Terrain|Settings")
	float MaxViewDistance;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Endless Terrain|View Target")
	AActor* Viewer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Endless Terrain|Settings")
	TSubclassOf<AActor> MeshClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Endless Terrain|Settings")
	TArray<FLODInfo> DetailLevels;

private:
	ATerrainGenerator* TerrainGenerator;	
	int32 ChunkSize = 240;
	int32 ChunksVisibleInViewDistance = 1;

	TMap<FVector2D, FTerrainChunk*> TerrainChunkDictionary;
	TDoubleLinkedList<FTerrainChunk*> TerrainChunksVisible;

public:	
	// Sets default values for this component's properties
	UEndlessTerrain();

	TSubclassOf<AActor> GetMeshClass() const { return MeshClass; };

	void UpdateVisibleChunks();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	AActor* GetViewActor() const;

};
