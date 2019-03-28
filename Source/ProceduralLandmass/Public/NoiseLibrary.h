// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Array2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NoiseLibrary.generated.h"


/**
 * 
 */
UCLASS()
class PROCEDURALLANDMASS_API UNoiseLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Noise Library")
	static FArray2D& GenerateNoiseMap(int32 mapSize, float scale, float lacunarity, int32 octaves, float persistance = 0.5f, const FVector2D& offset = FVector2D::ZeroVector, int32 seed = 42);

	/* Implementation of 2D Perlin noise based on Ken Perlin's original version (http://mrl.nyu.edu/~perlin/doc/oscar.html)
	 * (See Random1.tps for additional third party software info.)
	 * @return A value between -1 and 1. */
	static float PerlinNoise(float x, float y);

	UFUNCTION(BlueprintCallable, Category = "Noise Library")
	static float PerlinNoise(const FVector2D& vec);
};
