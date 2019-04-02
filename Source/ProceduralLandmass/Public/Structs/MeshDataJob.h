#pragma once
#include "CoreMinimal.h"
#include "TerrainChunk.h"
#include "MeshDataJob.generated.h"


class ATerrainGenerator;
class UCurveFloat;
struct FMeshData;
struct FArray2D;
class UTerrainChunk;
class INoiseGeneratorInterface;

/**
 * 
 */
USTRUCT()
struct FMeshDataJob
{
	GENERATED_BODY()


public:
	/////////////////////////////////////////////////////
	/* The terrain generator we are doing this job for. */
	ATerrainGenerator* MyGenerator;
	
	/* The chunk that the generated mesh data is for. */
	UTerrainChunk* Chunk;

	/////////////////////////////////////////////////////
	/* The noise generator that will be used to generate the mesh data. */
	INoiseGeneratorInterface* NoiseGenerator;

	/* Multiply each hight map value with this value. */
	float HeightMultiplier;

	/* For which level of detail the mesh data will be generated. */
	int32 LevelOfDetail;

	/* Number of vertices per edge at LOD 0. So the mesh has 2x this many vertices. */
	int32 ChunkSize;

	/* Add this offset to the noise generator input. */
	FVector2D Offset;

	/* A height curve to multiply the vertex height with. Optional. */
	UCurveFloat* HeightCurve;
	
	/////////////////////////////////////////////////////
	/* The generated mesh data. */
	FMeshData* GeneratedMeshData = nullptr;

	/* The generated height map. */
	FArray2D* GeneratedHeightMap = nullptr;

	/////////////////////////////////////////////////////
	FMeshDataJob() {};

	/**
	 * @param noiseGenerator Noise generator to be used for generating the height map
	 * @param chunk The chunk that we are creating the mesh data for.
	 * @param heightMultiplier Multiply each hight map Z-value with this value.
	 * @param levelOfDetail The LOD for this mesh data.
	 * @param chunkSize Number of vertices per edge at LOD 0. So the mesh has 2x this many vertices. This value is regardless of @see levelOfDetail!
	 * @param offset Add this offset to the noise generator input. This will shift the noise map by this value.
	 * @param heightCurve (Optional) Height curve to multiply the vertex height with. The X-axis represents the noise generator output (0..1) and the Y axis the modifier.
	 */
	FMeshDataJob(INoiseGeneratorInterface* noiseGenerator, UTerrainChunk* chunk, float heightMultiplier, int32 levelOfDetail, int32 targetMeshSize, int32 chunkSize, FVector2D offset, UCurveFloat* heightCurve = nullptr)
						: NoiseGenerator(noiseGenerator), Chunk(chunk), HeightMultiplier(heightMultiplier), LevelOfDetail(levelOfDetail), ChunkSize(chunkSize), Offset(offset), HeightCurve(heightCurve)
	{
		MyGenerator = Cast<ATerrainGenerator>(chunk->GetOwner());
	};
};