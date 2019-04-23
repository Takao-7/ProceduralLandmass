// Fill out your copyright notice in the Description page of Project Settings.

#include "PerlinNoiseGenerator.h"
#include "Kismet/KismetMathLibrary.h"
#include "UnityLibrary.h"

UPerlinNoiseGenerator::UPerlinNoiseGenerator()
{
	InitNoiseGenerator();
}

UPerlinNoiseGenerator::UPerlinNoiseGenerator(int32 seed, float scale, int32 octaves, float persistance, float lacunarity, FVector2D offset/* = FVector2D::ZeroVector*/)
	: Seed(seed), NoiseScale(scale), Octaves(octaves), Persistance(persistance), Lacunarity(lacunarity), Offset(offset)
{
	InitNoiseGenerator();
}

UPerlinNoiseGenerator::UPerlinNoiseGenerator(const UPerlinNoiseGenerator* other)
{
	CopySettings(other);
}

/////////////////////////////////////////////////
void UPerlinNoiseGenerator::CopySettings(const UNoiseGeneratorInterface* other)
{
	const UPerlinNoiseGenerator* otherGenerator = Cast<UPerlinNoiseGenerator>(other);
	if (otherGenerator == nullptr)
	{
		return;
	}

	Seed = otherGenerator->Seed;
	NoiseScale = otherGenerator->NoiseScale;
	Octaves = otherGenerator->Octaves;
	Persistance = otherGenerator->Persistance;
	Lacunarity = otherGenerator->Lacunarity;
	Offset = otherGenerator->Offset;
	OctaveOffsets = otherGenerator->OctaveOffsets;
	Limit = otherGenerator->Limit;

	otherGenerator->CopyArrays(P, G, G1, G2, G3);
}

/////////////////////////////////////////////////
void UPerlinNoiseGenerator::InitNoiseGenerator()
{
	/* Initialize a random number generator struct and seed it with the given value. */
	FRandomStream random(Seed);

	OctaveOffsets.SetNum(Octaves);
	for (FVector2D& offsetVec : OctaveOffsets)
	{
		const float offsetX = random.FRandRange(-1000.0f, 1000.0f) + Offset.X;
		const float offsetY = random.FRandRange(-1000.0f, 1000.0f) + Offset.Y;
		offsetVec = FVector2D(offsetX, offsetY);
	}

	/* The calculated theoretical highest value. */
	for (int32 i = 0; i < Octaves; i++)
	{
		Limit += FMath::Pow(Persistance, i);
	}

	P.SetNum(B + B + 2);
	G.SetNum(B + B + 2);
	G1.SetNum(B + B + 2);
	G2.SetNum(B + B + 2);
	G3.SetNum(B + B + 2);

	//random = FRandomStream(5);

	int i, j, k;
	for (i = 0; i < B; i++)
	{
		P[i] = i;

		G1[i] = (random.FRand() * 2) - 1;

		for (j = 0; j < 2; j++)
		{
			G2[i][j] = (random.FRand() * 2) - 1;
			G2[i].Normalize();
		}

		for (j = 0; j < 3; j++)
		{
			G3[i][j] = (random.FRand() * 2) - 1;
			G3[i].Normalize();
		}
	}

	while (--i)
	{
		k = P[i];
		P[i] = P[j = random.RandRange(0, 255)];
		P[j] = k;
	}

	for (i = 0; i < B + 2; i++)
	{
		P[B + i] = P[i];
		G1[B + i] = G1[i];
		for (j = 0; j < 2; j++)
		{
			G2[B + i][j] = G2[i][j];
		}
		for (j = 0; j < 3; j++)
		{
			G3[B + i][j] = G3[i][j];
		}
	}
}

float UPerlinNoiseGenerator::GetNoise2D(float x, float y)
{
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float noiseHeight = 0.0f;

	for (const FVector2D& octaveOffset : OctaveOffsets)
	{
		const float sampleX = x / NoiseScale * frequency + octaveOffset.X;
		const float sampleY = y / NoiseScale * frequency + octaveOffset.Y;

		const float perlinValue = PerlinNoise(FVector2D(sampleX, sampleY));
		noiseHeight += perlinValue * amplitude;

		amplitude *= Persistance;
		frequency *= Lacunarity;
	}

	// Normalize the noise value to [0,1]
	const float normalizedValues = UKismetMathLibrary::NormalizeToRange(noiseHeight, -Limit, Limit);
	return normalizedValues;
}

float UPerlinNoiseGenerator::PerlinNoise(const FVector2D& vec)
{
	auto sCurve = [](float t) -> float
	{
		return t * t * (3.0f - 2.0f * t);
	};

	auto setup = [=](int32 i, int32& b0, int32& b1, float& r0, float& r1, float& t) -> void
	{
		t = vec[i] + N;
		b0 = ((int32)t) & BM;
		b1 = (b0 + 1) & BM;
		r0 = t - (int32)t;
		r1 = r0 - 1.;
	};

	auto at2 = [](float rx, float ry, const FVector2D& q) -> float
	{
		return rx * q[0] + ry * q[1];
	};

	int32 bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, sx, sy, a, b, t, u, v;
	FVector2D q;
	int32 i, j;

	setup(0, bx0, bx1, rx0, rx1, t);
	setup(1, by0, by1, ry0, ry1, t);

	i = P[bx0];
	j = P[bx1];

	b00 = P[i + by0];
	b10 = P[j + by0];
	b01 = P[i + by1];
	b11 = P[j + by1];

	sx = sCurve(rx0);
	sy = sCurve(ry0);

	q = G2[b00]; u = at2(rx0, ry0, q);
	q = G2[b10]; v = at2(rx1, ry0, q);
	a = FMath::Lerp(u, v, sx);

	q = G2[b01]; u = at2(rx0, ry1, q);
	q = G2[b11]; v = at2(rx1, ry1, q);
	b = FMath::Lerp(u, v, sx);

	return FMath::Lerp(a, b, sy);
}

void UPerlinNoiseGenerator::CopyArrays(TArray<int32>& p, TArray<float>& g, TArray<float>& g1, TArray<FVector2D>& g2, TArray<FVector>& g3) const
{
	p = TArray<int32>(P);
	g = TArray<float>(G);
	g1 = TArray<float>(G1);
	g2 = TArray<FVector2D>(G2);
	g3 = TArray<FVector>(G3);
}
