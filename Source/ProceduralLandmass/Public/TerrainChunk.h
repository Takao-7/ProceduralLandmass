#pragma once
#include "CoreMinimal.h"
#include "Structs/LODInfo.h"
#include "ProceduralMeshComponent.h"
#include "MeshData.h"
#include "TerrainChunk.generated.h"


class ATerrainGenerator;
class AActor;
struct FMeshData;
class UBoxComponent;


UENUM(BlueprintType)
enum class EChunkStatus : uint8
{
	/* We are only spawned and don't have mesh data and therefore no mesh. */
	SPAWNED,
	/* We have received our mesh data, but haven't generated a mesh from it. */
	MESH_DATA_REQUESTED,
	/* We have generated a mesh. */
	IDLE
};


UCLASS(BlueprintType)
class UTerrainChunk : public UProceduralMeshComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadWrite)
	float MaxViewDistance = 0.0f;

	/* Our terrain generator */
	UPROPERTY(BlueprintReadWrite)
	ATerrainGenerator* TerrainGenerator;

public:
	/* Our status */
	UPROPERTY(BlueprintReadWrite)
	EChunkStatus Status = EChunkStatus::SPAWNED;

	TArray<FMeshData*> LODMeshes;
	FArray2D* HeightMap;

private:
	TArray<FLODInfo>* DetailLevels;

	int32 PreviousLOD = -1;
	int32 CurrentLOD = 0;

	/////////////////////////////////////////////////////
public:
	UTerrainChunk();

	void InitChunk(ATerrainGenerator* parentTerrainGenerator, TArray<FLODInfo>* lodInfoArray);

protected:
	UFUNCTION()
	void HandleCameraOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};