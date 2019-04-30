#pragma once
#include "CoreMinimal.h"
#include "Structs/LODInfo.h"
#include "ProceduralMeshComponent.h"
#include "MeshData.h"
#include "TerrainChunk.generated.h"


class ATerrainGenerator;
class AActor;
struct FTerrainMeshData;
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
	/* Our terrain generator */
	UPROPERTY(BlueprintReadWrite)
	ATerrainGenerator* TerrainGenerator;

public:
	/* Our status */
	UPROPERTY(BlueprintReadWrite)
	EChunkStatus Status = EChunkStatus::SPAWNED;

	TArray<FTerrainMeshData*> LODMeshes;
	FArray2D* HeightMap;

	/* The player's camera location. Used for level of detail.
	 * This location is updated in the Terrain generator's tick functions. */
	static FVector CameraLocation;

	/* The player's camera location in the last frame. */
	static FVector LastCameraLocation;

private:
	int32 CurrentLOD = 0;

	TArray<bool> RequestedMeshData;

	TArray<FLODInfo>* DetailLevels;

	/* This chunk's size, including it's parent terrain generator scale. */
	int32 TotalChunkSize = 0;

	FBox Box;

	/////////////////////////////////////////////////////
public:
	~UTerrainChunk();

	void SetNewLOD(int32 newLOD);
	void InitChunk(ATerrainGenerator* parentTerrainGenerator, TArray<FLODInfo>* lodInfoArray);

	void SetChunkBoundingBox();

	void UpdateChunk(FVector cameraLocation);
	void UpdateChunk();

	int32 GetOptimalLOD(FVector cameraLocation);
};