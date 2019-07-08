// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NoiseGeneratorInterface.h"
#include "PerlinNoiseModule.generated.h"

/**
 * 
 */
UCLASS()
class PROCEDURALLANDMASS_API UPerlinNoiseModule : public UNoiseGenerator
{
	GENERATED_BODY()
	
public:
    UPerlinNoiseModule();
	UPerlinNoiseModule(float noiseScale, int32 seed, float persistence, float lacunarity, int32 octaves);
    
    virtual float GetNoise2D_Implementation(float X, float Y) const override;
	virtual void CopyGenerator_Implementation(const UNoiseGenerator* otherGenerator) override;

    TArray<int32> p;
    TArray<float> g1;
	TArray<FVector2D> g2;
	TArray<FVector> g3;

private:
    void Init();
	float PerlinNoise(FVector2D vec) const;

    const int32 B = 256;
    const int32 N = 4096;
    const int32 BM = 255;
};
