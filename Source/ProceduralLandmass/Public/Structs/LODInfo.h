#pragma once
#include "LODInfo.generated.h"


USTRUCT(BlueprintType)
struct FLODInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, ClampMax = 8))
	int32 LOD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.0f))
	float VisibleDistanceThreshold;
};
