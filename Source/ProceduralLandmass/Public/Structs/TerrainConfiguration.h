#pragma once
#include "LODInfo.h"
#include "NoiseGeneratorInterface.h"
#include "TerrainConfiguration.generated.h"


class INoiseGeneratorInterface;


/* The number of vertices each chunk has per direction. */
UENUM(BlueprintType)
enum class EChunkSize : uint8
{
	x241,
	x481
};


USTRUCT(BlueprintType)
struct FTerrainConfiguration
{
	GENERATED_BODY()

protected:
	/* The number of threads we will use to generate the terrain.
	 * Settings this to 0 will use one thread per chunk to generate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 NumberOfThreads = 1;

public:
	/* The noise generator to generate the terrain. If left blank, we will use the default Perlin noise generator. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TScriptInterface<INoiseGeneratorInterface> NoiseGenerator;

	/* Number of vertices per direction per chunk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EChunkSize ChunkSize = EChunkSize::x481;

	/* The scale of each chunk. MapScale * MapChunkSize * NumberOfChunks = Total map width in cm. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1.0f, AM_Clamp = 100.0f), Category = "Map Generator|General")
	FVector MapScale = FVector(100.0f);

	/** Number of chunks per axis we will generate. So the entire generated terrain will consist of 2 times this many chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 NumChunks = 8;

	/** Multiplier for height map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1.0f))
	float Amplitude = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* HeightCurve = nullptr;

	/** If checked and numLODs > 1, material will be instanced and TerrainOpacity parameters used to dither LOD transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool DitheringLODTransitions = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLODInfo> LODs;

	FTerrainConfiguration()
	{
		for (int32 i = 0; i < 6; ++i)
		{
			FLODInfo newLOD;
			newLOD.LOD = i;
			newLOD.VisibleDistanceThreshold = 1000.0f * (i+1);
			LODs.Add(newLOD);
		}

		
	}

	/**
	 * Returns the actual number of threads. When @see NumberOfThreads was set to 0, then we will use one thread
	 * per chunk so the actual number of threads can be 2x the number of chunks per line.
	 */
	int32 GetNumberOfThreads() const
	{
		return NumberOfThreads == 0 ? NumChunks * 2 : NumberOfThreads;
	}

	int32 GetNumVertices() const
	{
		switch (ChunkSize)
		{
		case EChunkSize::x241:
			return 241;
		case EChunkSize::x481:
			return 481;
		default:
			return 241;
		}
	}
};
