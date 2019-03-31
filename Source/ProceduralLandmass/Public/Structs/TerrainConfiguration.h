#pragma once
#include "LODInfo.h"
#include "NoiseGeneratorInterface.h"
#include "TerrainConfiguration.generated.h"


class INoiseGeneratorInterface;


USTRUCT(BlueprintType)
struct FTerrainConfiguration
{
	GENERATED_BODY()

public:	
	/* The noise generator to generate the terrain. If left blank, we will use the default Perlin noise generator. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TScriptInterface<INoiseGeneratorInterface> NoiseGenerator;

	/* The number of threads we will use to generate the terrain.
	 * Settings this to 0 will use one thread per chunk to generate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	uint8 NumberOfThreads = 1;
		
	/** Number of chunks per axis we will generate. So the entire generated terrain will consist of 2 times this many chunks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 NumChunksPerDirection = 32;
	
	/** Multiplier for height map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1.0f))
	float Amplitude = 5000.0f;
	
	/** If checked and numLODs > 1, material will be instanced and TerrainOpacity parameters used to dither LOD transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool  DitheringLODTransitions = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLODInfo> LODs;
};
