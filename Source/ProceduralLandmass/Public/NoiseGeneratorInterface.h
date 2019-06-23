#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "NoiseGeneratorInterface.generated.h"


UCLASS(Abstract, BlueprintType)
class PROCEDURALLANDMASS_API UNoiseGenerator : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Noise Generator")
    virtual float GetNoise2D(float X, float Y);

    UFUNCTION(BlueprintCallable, Category = "Noise Generator")
    virtual float GetNoise3D(float X, float Y, float Z);

    /** Returns a copy of this noise generator. */
    UFUNCTION(BlueprintCallable, Category = "Noise Generator")
    virtual UNoiseGenerator* CopyGenerator();
};