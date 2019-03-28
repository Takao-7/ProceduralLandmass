// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Structs/TerrainChunk.h"
#include "EndlessTerrain.generated.h"


class AStaticMeshActor;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROCEDURALLANDMASS_API UEndlessTerrain : public UActorComponent
{
	GENERATED_BODY()


protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Endless Terrain|Settings")
	float MaxViewDistance = 300.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Endless Terrain|View Target")
	AActor* Viewer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Endless Terrain|Settings")
	TSubclassOf<AStaticMeshActor> MeshClass;

	FVector ViewerPosition = FVector::ZeroVector;
	int32 ChunkSize = 240;
	int32 ChunksVisibleInViewDistance = 1;

	TMap<FVector2D, FTerrainChunk*> TerrainChunkDictionary;
	TDoubleLinkedList<FTerrainChunk*> TerrainChunksVisibleLastFrame;

public:	
	// Sets default values for this component's properties
	UEndlessTerrain();

	void UpdateVisibleChunks();
	FVector GetViewerPosition() { return ViewerPosition; };
	float GetMaxViewDistance() { return MaxViewDistance; };

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
