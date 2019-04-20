#pragma once
#include "Structs/LODInfo.h"
#include "NoiseGeneratorInterface.h"
#include "TerrainConfiguration.generated.h"


class UNoiseGeneratorInterface;


/* The number of vertices each chunk has per direction. */
UENUM(BlueprintType)
enum class ENumVertices : uint8
{
	x61 = 61,
	x121 = 121,
	x241 = 241
};


USTRUCT(BlueprintType)
struct FTerrainConfiguration
{
	GENERATED_BODY()


public:
	/* The number of threads we will use to generate the terrain.
	 * Settings this to 0 will use one thread per chunk to generate
	 * and a value of -1 will not use any threading. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = -1))
	int32 NumberOfThreads = 7;

	/* The noise generator to generate the terrain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNoiseGeneratorInterface* NoiseGenerator;

	/* Number of vertices per direction per chunk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ENumVertices NumVertices = ENumVertices::x61;

	/* The scale of each chunk. MapScale * MapChunkSize * NumberOfChunks = Total map width in cm. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1.0f, ClampMax = 100.0f))
	float MapScale = 100.0f;

	/** Number of chunks per axis we will generate. So the entire generated terrain will consist of 2 times this many chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 NumChunks = 20;

	/** Multiplier for height map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1.0f))
	float Amplitude = 17.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* HeightCurve = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TArray<FLODInfo> LODs;

	bool operator==(const FTerrainConfiguration& otherConfig) const
	{
		return
		(
			NumberOfThreads == otherConfig.NumberOfThreads &&
			NoiseGenerator == otherConfig.NoiseGenerator &&
			NumVertices == otherConfig.NumVertices &&
			MapScale == otherConfig.MapScale &&
			NumChunks == otherConfig.NumChunks &&
			Amplitude == otherConfig.Amplitude &&
			HeightCurve == otherConfig.HeightCurve
		);
	}

	FTerrainConfiguration()
	{
		FLODInfo newLOD;
		newLOD.LOD = 0;
		newLOD.VisibleDistanceThreshold = 7500.0f;
		LODs.Add(newLOD);			

		for (int32 i = 1; i <= 12; ++i)
		{
			if(GetChunkSize() % (2*i) != 0)
			{
				continue;
			}

			newLOD.LOD = i;
			newLOD.VisibleDistanceThreshold = 7500.0f * (i+1);
			LODs.Add(newLOD);
		}		
	}

	/**
	 * Returns the actual number of threads. When @see NumberOfThreads was set to 0, then we will use one thread
	 * per chunk so the actual number of threads can be 2x the number of chunks per line.
	 */
	FORCEINLINE int32 GetNumberOfThreads() const
	{
		return NumberOfThreads == -1 ? NumChunks * 2 : NumberOfThreads;
	}

	FORCEINLINE int32 GetNumVertices() const
	{
		return (int32)NumVertices;
	}

	FORCEINLINE int32 GetChunkSize() const
	{
		return GetNumVertices() - 1;
	}
};
