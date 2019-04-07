#pragma once
#include "CoreMinimal.h"
#include "TerrainChunk.h"
#include "MeshDataJob.generated.h"


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
	ATerrainGenerator* MyGenerator = nullptr;
	
	/* The chunk that the generated mesh data is for. */
	UTerrainChunk* Chunk = nullptr;

	/////////////////////////////////////////////////////
	/* The noise generator that will be used to generate the mesh data. */
	TScriptInterface<INoiseGeneratorInterface> NoiseGenerator = nullptr;

	/* Multiply each hight map value with this value. */
	float HeightMultiplier = 0.0f;

	/* For which level of detail the mesh data will be generated. */
	int32 LevelOfDetail = 0;

	/* Number of vertices per edge at LOD 0. So the mesh has 2x this many vertices. */
	int32 ChunkSize = 0;

	/* Add this offset to the noise generator input. */
	FVector2D Offset = FVector2D::ZeroVector;

	/* A height curve to multiply the vertex height with. Optional. */
	UCurveFloat* HeightCurve = nullptr;
	
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
	FMeshDataJob(TScriptInterface<INoiseGeneratorInterface> noiseGenerator, UTerrainChunk* chunk, float heightMultiplier, int32 levelOfDetail, int32 chunkSize, FVector2D offset, UCurveFloat* heightCurve = nullptr)
						: Chunk(chunk), NoiseGenerator(noiseGenerator), HeightMultiplier(heightMultiplier), LevelOfDetail(levelOfDetail), ChunkSize(chunkSize), Offset(offset), HeightCurve(heightCurve)
	{
		MyGenerator = reinterpret_cast<ATerrainGenerator*>(chunk->GetOwner());
	};
};