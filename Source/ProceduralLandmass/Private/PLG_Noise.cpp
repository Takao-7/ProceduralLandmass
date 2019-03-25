// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/PLG_Noise.h"


TMap<int32, TArray<float>> UPLG_Noise::GenerateNoiseMap(int mapSize, float scale)
{
	TMap<int32, TArray<float>> noiseMap = TMap<int32, TArray<float>>();
	
	if (scale <= 0)
	{
		scale = 0.0001f;
	}

	for (int32 y = 0; y < mapSize; ++y)
	{
		TArray<float> row = TArray<float>();
		row.SetNum(mapSize);

		for (int32 x = 0; x < mapSize; ++x)
		{
			const float sampleX = x / scale;
			const float sampleY = y / scale;

			const float noiseX = FMath::PerlinNoise1D(sampleX);
			const float noiseY = FMath::PerlinNoise1D(sampleY);
			const FVector2D perlinValue = FVector2D(noiseX, noiseY);
			row[x] = perlinValue.Size();
		}

		noiseMap.Add(y, row);
	}

	return noiseMap;
}
