#pragma once
#include "CoreMinimal.h"
#include "MeshDataJob.generated.h"


class ATerrainGenerator;
class UCurveFloat;
struct FMeshData;
struct FArray2D;

/**
 * 
 */
USTRUCT()
struct FMeshDataJob
{
	GENERATED_BODY()

public:
	/* The terrain generator we are doing this job for. */
	ATerrainGenerator* MyGenerator;
	
	/* The generated mesh data. */
	FMeshData* MeshData = nullptr;

	/* The generated height map. */
	FArray2D* HeightMap = nullptr;
};