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
class PROCEDURALLANDMASS_API UPerlinNoiseGenerator : public UNoiseGeneratorInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	int32 Seed = 42;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float NoiseScale = 50.0f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	int32 Octaves = 4;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float Persistance = 0.5f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float Lacunarity = 2.0f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	FVector2D Offset = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TArray<FVector2D> OctaveOffsets;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float Limit = 0.0f;
	
	UPerlinNoiseGenerator();
	UPerlinNoiseGenerator(int32 seed, float scale, int32 octaves, float persistance, float lacunarity, FVector2D offset = FVector2D::ZeroVector);
	void InitNoiseGenerator();

	virtual float GetNoise2D(float x, float y) override;
};
