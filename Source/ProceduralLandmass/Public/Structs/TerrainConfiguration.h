#pragma once
#include "Structs/LODInfo.h"
#include "Public/NoiseGeneratorInterface.h"
#include "TerrainConfiguration.generated.h"


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
	/* Don't generate any collision */
	NoCollision
};


USTRUCT(BlueprintType)
struct FTerrainConfiguration
{
	GENERATED_BODY()


public:
	/* The number of threads we will use to generate the terrain.
	 * Settings this to 0 will use one thread per chunk to generate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 NumberOfThreads = 7;

	/* The noise generator class to generate the terrain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UNoiseGenerator> NoiseGeneratorClass = nullptr;

	/* The noise generator object. */
	UPROPERTY(BlueprintReadOnly)
	UNoiseGenerator* NoiseGenerator;

	/* Number of vertices per direction per chunk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ENumVertices NumVertices = ENumVertices::x61;

	/* The scale of each chunk. MapScale * MapChunkSize * NumberOfChunks = Total map width in cm. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1.0f, ClampMax = 100.0f))
	float MapScale = 100.0f;

	/** Number of chunks per axis we will generate. So the entire generated terrain will consist of 2 times this many chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1))
	int32 NumChunks = 20;

	/** Multiplier for height map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1.0f))
	float Amplitude = 17.5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECollisonMode Collision = ECollisonMode::NoCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* HeightCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLODInfo> LODs = TArray<FLODInfo>();

	///////////////////////////////////////////////////////
	FTerrainConfiguration()
	{
		if (NoiseGeneratorClass != nullptr)
		{
			NoiseGenerator = NewObject<UNoiseGenerator>((UObject*)GetTransientPackage(), NoiseGeneratorClass);
		}
	}

	FTerrainConfiguration(const FTerrainConfiguration& reference)
	{
		CopyConfiguration(reference);
	}

	///////////////////////////////////////////////////////
	bool operator==(const FTerrainConfiguration& other) const
	{
		return
		(
			NumberOfThreads == other.NumberOfThreads &&
			NoiseGeneratorClass == other.NoiseGeneratorClass &&
			NumVertices == other.NumVertices &&
			MapScale == other.MapScale &&
			NumChunks == other.NumChunks &&
			Amplitude == other.Amplitude &&
			HeightCurve == other.HeightCurve
		);
	}

	///////////////////////////////////////////////////////
	void CopyConfiguration(const FTerrainConfiguration& reference)
	{
		NumberOfThreads = reference.NumberOfThreads;
		NumVertices = reference.NumVertices;
		MapScale = reference.MapScale;
		NumChunks = reference.NumChunks;
		Amplitude = reference.Amplitude;
		Collision = reference.Collision;
		LODs = reference.LODs;
		NoiseGeneratorClass = reference.NoiseGeneratorClass;

		if (reference.HeightCurve)
		{
			HeightCurve = DuplicateObject<UCurveFloat>(reference.HeightCurve, nullptr);
		}

		UNoiseGenerator* otherNoiseGenerator = reference.NoiseGenerator;
		if (otherNoiseGenerator)
		{
			NoiseGenerator = DuplicateObject<UNoiseGenerator>(otherNoiseGenerator, nullptr);
			NoiseGenerator->CopyGenerator(otherNoiseGenerator);
		}
	}

    ///////////////////////////////////////////////////////
	void InitLODs()
	{
		LODs.Empty();

		const float distance = 7500.0f;

		FLODInfo newLOD;
		newLOD.LOD = 0;
		newLOD.VisibleDistanceThreshold = distance;
		LODs.Add(newLOD);

		int32 i = 1;
		while (LODs.Num() <= 15 && i < GetChunkSize() / 2 && LODs.Last().LOD <= 15)
		{
			if (GetChunkSize() % (2 * i) != 0)
			{
				++i;
				continue;
			}

			newLOD.LOD = i;
			newLOD.VisibleDistanceThreshold = distance * (i + 1);
			LODs.Add(newLOD);
			++i;
		}
	}

	///////////////////////////////////////////////////////
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
