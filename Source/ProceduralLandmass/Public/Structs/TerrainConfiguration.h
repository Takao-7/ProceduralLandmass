#pragma once
#include "Structs/LODInfo.h"
#include "NoiseGeneratorInterface.h"
#include "TerrainConfiguration.generated.h"


class UNoiseGeneratorInterface;
struct FArray2D;


/* The number of vertices each chunk has per direction. */
UENUM(BlueprintType)
enum class ENumVertices : uint8
{
	x61 = 61,
	x121 = 121,
	x241 = 241
};

UENUM(BlueprintType)
enum class ECollisonMode : uint8
{
	NoCollision
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
	int32 NumberOfThreads = 4;

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
	bool bUseFalloffMap = false;

	/* When using a falloff map, should it's size be equal to one chunk (true) or
	 * the entire terrain (false)? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bFalloffMapPerChunk = false;

	FArray2D* FalloffMap = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECollisonMode Collision = ECollisonMode::NoCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* HeightCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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

	void InitLODs()
	{
		LODs.Empty();

		const float distance = 10000.0f;

		FLODInfo newLOD;
		newLOD.LOD = 0;
		newLOD.VisibleDistanceThreshold = distance;
		LODs.Add(newLOD);

		int32 i = 1;
		while (LODs.Num() <= 6)
		{
			if (GetChunkSize() % (2 * i) != 0)
			{
				continue;
			}

			newLOD.LOD = i;
			newLOD.VisibleDistanceThreshold = distance * (i + 1);
			LODs.Add(newLOD);
			++i;
		}

		/*for (int32 i = 1; i <= GetNumVertices(); ++i)
		{
			if (GetChunkSize() % (2 * i) != 0)
			{
				continue;
			}

			newLOD.LOD = i;
			newLOD.VisibleDistanceThreshold = 5000.0f * (i + 1);
			LODs.Add(newLOD);
		}*/
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
