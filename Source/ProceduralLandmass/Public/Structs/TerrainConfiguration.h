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
	 * Settings this to 0 will use one thread per chunk to generate
	 * and a value of -1 will not use any threading. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = -1))
	int32 NumberOfThreads = 7;

public:
	/* The noise generator to generate the terrain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TScriptInterface<INoiseGeneratorInterface> NoiseGenerator;

	/* Number of vertices per direction per chunk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EChunkSize ChunkSize = EChunkSize::x481;

	/* The scale of each chunk. MapScale * MapChunkSize * NumberOfChunks = Total map width in cm. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1.0f, ClampMax = 100.0f))
	FVector MapScale = FVector(100.0f);

	/** Number of chunks per axis we will generate. So the entire generated terrain will consist of 2 times this many chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 NumChunks = 3;

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
		{
			FLODInfo newLOD;
			newLOD.LOD = 0;
			newLOD.VisibleDistanceThreshold = 1000.0f;
			LODs.Add(newLOD);			
		}

		for (int32 i = 1; i <= 12; ++i)
		{
			if((GetNumVertices()-1) % i != 0)
			{
				continue;
			}

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
		return NumberOfThreads == -1 ? NumChunks * 2 : NumberOfThreads;
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
