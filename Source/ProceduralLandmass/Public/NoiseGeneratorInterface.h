#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "NoiseGeneratorInterface.generated.h"


UCLASS(Abstract, Blueprintable)
class PROCEDURALLANDMASS_API UNoiseGenerator : public UObject
{
    GENERATED_BODY()

public:
    //UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Noise Generator")
    //UNoiseGenerator* CreateModule();
    
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Noise Generator")
    float GetNoise2D(float X, float Y) const;
    virtual float GetNoise2D_Implementation(float X, float Y) const { return 0.0f; };

    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Noise Generator")
    float GetNoise3D(float X, float Y, float Z) const;
    virtual float GetNoise3D_Implementation(float X, float Y, float Z) const { return 0.0f; };
};
