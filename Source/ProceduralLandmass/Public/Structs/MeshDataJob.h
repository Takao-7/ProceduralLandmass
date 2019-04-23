#pragma once
#include "CoreMinimal.h"
#include "TerrainChunk.h"
#include "TerrainConfiguration.h"
#include "MeshDataJob.generated.h"


class UCurveFloat;
struct FMeshData;
struct FArray2D;
class UTerrainChunk;
class UNoiseGeneratorInterface;

/**
 * 
 */
USTRUCT()
struct FMeshDataJob
{
	GENERATED_BODY()


public:
	/////////////////////////////////////////////////////	
	/* The chunk that the generated mesh data is for. */
	UTerrainChunk* Chunk = nullptr;

	/* The queue where the finished job should be enqueued to. */
	TQueue<FMeshDataJob, EQueueMode::Mpsc>* DropOffQueue = nullptr;

	/////////////////////////////////////////////////////
	/* For which level of detail the mesh data will be generated. */
	int32 LevelOfDetail = 0;

	/* Do we want to update the mesh section or create a new one? */
	bool bUpdateMeshSection = false;

	/* Add this offset to the noise generator input and to the uv coordinates. */
	FVector2D Offset = FVector2D::ZeroVector;

	/////////////////////////////////////////////////////
	/* The generated mesh data. */
	FMeshData* GeneratedMeshData = nullptr;

	/* The generated height map. */
	FArray2D* GeneratedHeightMap = nullptr;

	/////////////////////////////////////////////////////
	FMeshDataJob() {}

	/**
	 * @param noiseGenerator Noise generator to be used for generating the height map
	 * @param chunk The chunk that we are creating the mesh data for.
	 * @param dropOffQueue When we are done, the finished job will be enqueued here.
	 * @param levelOfDetail The LOD for this mesh data.
	 * @param offset Add this offset to the noise generator input. This will shift the noise map by this value.
	 * @param heightCurve (Optional) Height curve to multiply the vertex height with. The X-axis represents the noise generator output (0..1) and the Y axis the modifier.
	 */
	FMeshDataJob(UTerrainChunk* chunk, TQueue<FMeshDataJob, EQueueMode::Mpsc>* dropOffQueue, 
		int32 levelOfDetail, bool bUpdateMeshSection = false, FVector2D offset = FVector2D::ZeroVector):
		Chunk(chunk), 
		DropOffQueue(dropOffQueue),
		LevelOfDetail(levelOfDetail),
		bUpdateMeshSection(bUpdateMeshSection), 
		Offset(offset)
	{}
};