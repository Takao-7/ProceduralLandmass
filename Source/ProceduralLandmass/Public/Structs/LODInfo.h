#pragma once
#include "LODInfo.generated.h"


USTRUCT(BlueprintType)
struct FLODInfo
{
	GENERATED_BODY()

public:
	FLODInfo() {}
	FLODInfo(int32 lod, float distance) : LOD(lod), VisibleDistanceThreshold(distance) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, ClampMax = 8))
	int32 LOD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.0f))
	float VisibleDistanceThreshold;

	/* Finds the optimal level of detail for the given distance. */
	static int32 FindLOD(const TArray<FLODInfo>& lods, float distance)
	{
		int32 optimalLOD = 0;
		for (const FLODInfo& lodInfo : lods)
		{
			if (lodInfo.VisibleDistanceThreshold > distance)
			{
				break;
			}

			optimalLOD = lodInfo.LOD;
		}

		return optimalLOD;
	}
};
