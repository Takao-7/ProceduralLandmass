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
    UPerlinNoiseModule(int32 seed);
    
    virtual float GetNoise2D_Implementation(float X, float Y) override;

private:
    void Init(int32 seed = 5);

    int32 Seed = 5;
    bool bAlreadyInitilise = false;

    const int32 B = 256;
	const int32 N = 4096;
	const int32 BM = 255;

    TArray<int32> p;
    TArray<float> g;
    TArray<float> g1;
	TArray<FVector2D> g2;
	TArray<FVector> g3;
};
