// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NoiseGeneratorInterface.h"
#include "PerlinNoiseGenerator.generated.h"


/**
 * 
 */
UCLASS(BlueprintType)
class PROCEDURALLANDMASS_API UPerlinNoiseGenerator : public UObject, public INoiseGeneratorInterface
{
	GENERATED_BODY()

protected:
	int32 Seed = 42;
	float NoiseScale = 50.0f;
	int32 Octaves = 4;
	float Persistance = 0.5f;
	float Lacunarity = 2.0f;
	FVector2D Offset = FVector2D::ZeroVector;

	TArray<FVector2D> OctaveOffsets;
	float Limit = 0.0f;
	
public:
	UPerlinNoiseGenerator();
	void InitNoiseGenerator();
	UPerlinNoiseGenerator(int32 seed, float scale, int32 octaves, float persistance, float lacunarity, FVector2D offset = FVector2D::ZeroVector);

	virtual float GetNoise2D_Implementation(float x, float y) override;
};
