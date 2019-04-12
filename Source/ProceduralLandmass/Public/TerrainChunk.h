#pragma once
#include "CoreMinimal.h"
#include "Structs/LODInfo.h"
#include "ProceduralMeshComponent.h"
#include "MeshData.h"
#include "TerrainChunk.generated.h"


class ATerrainGenerator;
class AActor;
struct FMeshData;


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
	/* This chunk's world position, measured from it's center point. */
	UPROPERTY(BlueprintReadWrite)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	AActor* Viewer;

	/* The mesh component that we hold data for. */
	UPROPERTY(BlueprintReadWrite)
	UProceduralMeshComponent* MeshComponent = nullptr;

	UPROPERTY(BlueprintReadWrite)
	float MaxViewDistance = 0.0f;

	/* Our status */
	UPROPERTY(BlueprintReadWrite)
	EChunkStatus Status = EChunkStatus::SPAWNED;

	/* Our terrain generator */
	UPROPERTY(BlueprintReadWrite)
	ATerrainGenerator* TerrainGenerator;

private:
	TArray<FLODInfo>* DetailLevels;
	TArray<FMeshData*> LODMeshes;

	int32 PreviousLOD = -1;
	int32 CurrentLOD = 0;


	/////////////////////////////////////////////////////
public:
	UTerrainChunk() {};

	void InitChunk(float viewDistance, ATerrainGenerator* parentTerrainGenerator, TArray<FLODInfo>* lodInfoArray, AActor* viewer, float zPosition = 0.0f);

	/* Updates this chunk's visibility based on the viewer position. If the distance to the given position is
	 * smaller than the MaxViewDistance, than this chunk will be set to visible.
	 * @return The new visibility. */
	bool UpdateTerrainChunk();

	bool IsChunkVisible() const { return MeshComponent && MeshComponent->IsMeshSectionVisible(CurrentLOD); };
	void SetIsVisible(bool bVisible);

	void DeleteMesh();
};