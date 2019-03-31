// Fill out your copyright notice in the Description page of Project Settings.

#include "PerlinNoiseGenerator.h"
#include "Kismet/KismetMathLibrary.h"

UPerlinNoiseGenerator::UPerlinNoiseGenerator(int32 seed, float scale, int32 octaves, float persistance, float lacunarity, FVector2D offset/* = FVector2D::ZeroVector*/)
{
	this->Seed;
	this->NoiseScale;
	this->Octaves;
	this->Persistance;
	this->Lacunarity;
	this->Offset = offset;

	/* Initialize a random number generator struct and seed it with the given value. */
	FRandomStream Random(Seed);

	OctaveOffsets.SetNum(Octaves);
	for (FVector2D& offsetVec : OctaveOffsets)
	{
		float offsetX = Random.FRandRange(-1000.0f, 1000.0f) + Offset.X;
		float offsetY = Random.FRandRange(-1000.0f, 1000.0f) + Offset.Y;
		offsetVec = FVector2D(offsetX, offsetY);
	}

	/* The calculated theoretical highest value. */
	for (int32 i = 0; i < Octaves; i++)
	{
		Limit += FMath::Pow(Persistance, i);
	}
}

float UPerlinNoiseGenerator::GetNoise2D_Implementation(float x, float y)
{
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float noiseHeight = 0.0f;

	for (const FVector2D& octaveOffset : OctaveOffsets)
	{
		const float sampleX = x / NoiseScale * frequency + octaveOffset.X;
		const float sampleY = y / NoiseScale * frequency + octaveOffset.Y;

		const float perlinValue = GetNoise2D(sampleX, sampleY);
		noiseHeight += perlinValue * amplitude;

		amplitude *= Persistance;
		frequency *= Lacunarity;
	}

	// Normalize the noise value to [0,1]
	return UKismetMathLibrary::NormalizeToRange(noiseHeight, -Limit, Limit);
}
