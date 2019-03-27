// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/PLG_NoiseLibrary.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

/* If set to true, we will optimize the normalization by not normalizing between the actual highest and lowest value,
 * but between the theoretically maximum values. The means that we don't have to iterate over the entire noise map again
 * to normalize the value, hover the resulting noise map will not have pure blacks and whites because it's very unlikely
 * that these value are generated. */
static bool bOptimiseNormalization = false;


FArray2D& UNoiseLibrary::GenerateNoiseMap(int32 mapSize, float scale, float lacunarity, int32 octaves, float persistance /*= 0.5f*/, const FVector2D& offset /*= FVector2D::ZeroVector*/, int32 seed /*= 42*/)
{
	FArray2D* noiseMap = new FArray2D(mapSize, mapSize);

	/* Initialize a random number generator struct and seed it with the given value. */
	FRandomStream random(seed);

	TArray<FVector2D> octaveOffsets; octaveOffsets.SetNum(octaves);
	for (FVector2D& offsetVec : octaveOffsets)
	{
		float offsetX = random.FRandRange(-1000.0f, 1000.0f) + offset.X;
		float offsetY = random.FRandRange(-1000.0f, 1000.0f) + offset.Y;
		offsetVec = FVector2D(offsetX, offsetY);
	}

	const float halfSize = mapSize / 2.0;

	/* The actual min value in the noise map. */
	float minValue = 1.0f / SMALL_NUMBER;

	/* The actual max value in the noise map. */
	float maxValue = SMALL_NUMBER;

	/* The calculated theoretical highest value. */
	float limit = 0.0f;
	for (int32 i = 0; i < octaves; i++)
	{
		limit += FMath::Pow(persistance, i);
	}

	const auto generateNoise = [&](float& value, int32 x, int32 y)
	{
		float amplitude = 1.0f;
		float frequency = 1.0f;
		float noiseHeight = 0.0f;

		for (const FVector2D& octaveOffset : octaveOffsets)
		{
			const float sampleX = (x - halfSize) / scale * frequency + octaveOffset.X;
			const float sampleY = (y - halfSize) / scale * frequency + octaveOffset.Y;

			const float perlinValue = PerlinNoise(sampleX, sampleY);
			noiseHeight += perlinValue * amplitude;

			amplitude *= persistance;
			frequency *= lacunarity;
		}

		if (noiseHeight > maxValue)
		{
			maxValue = noiseHeight;
		}
		else if (noiseHeight < minValue)
		{
			minValue = noiseHeight;
		}

		value = bOptimiseNormalization ? UKismetMathLibrary::NormalizeToRange(noiseHeight, -limit, limit) : noiseHeight;
	};
	noiseMap->ForEachWithIndex(generateNoise);

	if (!bOptimiseNormalization)
	{
		const auto normalizeValue = [minValue, maxValue](float& value) { value = UKismetMathLibrary::NormalizeToRange(value, minValue, maxValue); };
		noiseMap->ForEach(normalizeValue);
	}
	
	return *noiseMap;
}

/////////////////////////////////////////////////////
float UNoiseLibrary::PerlinNoise(float x, float y)
{
	return PerlinNoise(FVector2D(x, y));
}

#define s_curve(t) ( t * t * (3.0f - 2.0f * t) )
#define N 0x1000
#define setup(i,b0,b1,r0,r1)\
	t = vec[i] + N;\
	b0 = ((int)t) & BM;\
	b1 = (b0+1) & BM;\
	r0 = t - (int)t;\
	r1 = r0 - 1.;

float UNoiseLibrary::PerlinNoise(const FVector2D& vec)
{
	const int32 B = 256;
	static int32 p[B + B + 2];
	static float g[B + B + 2];
	const int32 BM = 255;

	static float g1[B + B + 2];
	static TArray<FVector2D> g2;
	static TArray<FVector> g3;

	static bool bIsFirstCall = true;
	if (bIsFirstCall)
	{
		bIsFirstCall = false;

		FRandomStream rand(5);

		g2.SetNum(B + B + 2);
		g3.SetNum(B + B + 2);

		int i, j, k;
		for (i = 0; i < B; i++)
		{
			p[i] = i;

			g1[i] = (rand.FRand() * 2) - 1;

			for (j = 0; j < 2; j++)
			{
				g2[i][j] = (rand.FRand() * 2) - 1;
				g2[i].Normalize();
			}

			for (j = 0; j < 3; j++)
			{
				g3[i][j] = (rand.FRand() * 2) - 1;
				g3[i].Normalize();
			}
		}

		while (--i)
		{
			k = p[i];
			p[i] = p[j = rand.RandRange(0, 255)];
			p[j] = k;
		}

		for (i = 0; i < B + 2; i++)
		{
			p[B + i] = p[i];
			g1[B + i] = g1[i];
			for (j = 0; j < 2; j++)
			{
				g2[B + i][j] = g2[i][j];
			}
			for (j = 0; j < 3; j++)
			{
				g3[B + i][j] = g3[i][j];
			}
		}		
	}

	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, sx, sy, a, b, t, u, v;
	FVector2D q;
	register int i, j;

	setup(0, bx0, bx1, rx0, rx1);
	setup(1, by0, by1, ry0, ry1);

	i = p[bx0];
	j = p[bx1];

	b00 = p[i + by0];
	b10 = p[j + by0];
	b01 = p[i + by1];
	b11 = p[j + by1];

	sx = s_curve(rx0);
	sy = s_curve(ry0);

	#define at2(rx,ry) ( rx * q[0] + ry * q[1] )

	q = g2[b00]; u = at2(rx0, ry0);
	q = g2[b10]; v = at2(rx1, ry0);
	a = FMath::Lerp(u, v, sx);

	q = g2[b01]; u = at2(rx0, ry1);
	q = g2[b11]; v = at2(rx1, ry1);
	b = FMath::Lerp(u, v, sx);

	return FMath::Lerp(a, b, sy);
}