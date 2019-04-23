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
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (ExposeOnSpawn = true))
	int32 Seed = 42;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (ExposeOnSpawn = true))
	float NoiseScale = 50.0f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (ExposeOnSpawn = true))
	int32 Octaves = 4;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (ExposeOnSpawn = true))
	float Persistance = 0.5f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (ExposeOnSpawn = true))
	float Lacunarity = 2.0f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (ExposeOnSpawn = true))
	FVector2D Offset = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TArray<FVector2D> OctaveOffsets;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	float Limit = 0.0f;
	
	UPerlinNoiseGenerator();
	UPerlinNoiseGenerator(int32 seed, float scale, int32 octaves, float persistance, float lacunarity, FVector2D offset = FVector2D::ZeroVector);
	UPerlinNoiseGenerator(const UPerlinNoiseGenerator* other);
	
	virtual void CopySettings(const UNoiseGeneratorInterface* other) override;
	virtual float GetNoise2D(float x, float y) override;

	void CopyArrays(TArray<int32>& p, TArray<float>& g, TArray<float>& g1, TArray<FVector2D>& g2, TArray<FVector>& g3) const;

private:
	void InitNoiseGenerator();
	float PerlinNoise(const FVector2D& vec);

	const int32 B = 256;
	const int32 N = 4096;
	const int32 BM = 255;
	TArray<int32> P;
	TArray<float> G;
	TArray<float> G1;
	TArray<FVector2D> G2;
	TArray<FVector> G3;
};
