#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "NoiseGeneratorInterface.generated.h"


UCLASS(Abstract, Blueprintable)
class PROCEDURALLANDMASS_API UNoiseGenerator : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Noise Generator")
	virtual float GetNoise2D(float X, float Y) { return 0.0f; };

    UFUNCTION(BlueprintCallable, Category = "Noise Generator")
    virtual float GetNoise3D(float X, float Y, float Z) { return 0.0f; };

    /** Returns a copy of this noise generator. */
    UFUNCTION(BlueprintCallable, Category = "Noise Generator")
    virtual UNoiseGenerator* CopyGenerator() { return NewObject<UNoiseGenerator>(); };
};
