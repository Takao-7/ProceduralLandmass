// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/PLG_Noise.h"
#include "Engine/Texture2D.h"

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

			/*const float noiseValue = FMath::PerlinNoise1D(sampleX + sampleY * mapSize);*/
			const float noiseValue = PerlinNoise(sampleX, sampleY);
			row[x] = noiseValue;
		}

		noiseMap.Add(y, row);
	}

	return noiseMap;
}

/////////////////////////////////////////////////////
float UPLG_Noise::PerlinNoise(float x, float y)
{
	return PerlinNoise(FVector2D(x, y));
}

#define B 0x100
#define BM 0xff

#define N 0x1000
#define NP 12   /* 2^N */
#define NM 0xfff

static int32 p[B + B + 2];
static TArray<FVector> g3 = TArray<FVector>(); //static float g3[B + B + 2][3];
static TArray<FVector2D> g2 = TArray<FVector2D>(); //static float g2[B + B + 2][2];
static float g1[B + B + 2];
static int32 start = 1;

static void init(void);

#define s_curve(t) ( t * t * (3. - 2. * t) )

#define lerp(t, a, b) ( a + t * (b - a) )

#define setup(i,b0,b1,r0,r1)\
	t = vec[i] + N;\
	b0 = ((int)t) & BM;\
	b1 = (b0+1) & BM;\
	r0 = t - (int)t;\
	r1 = r0 - 1.;

float UPLG_Noise::PerlinNoise(FVector2D vec)
{
	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, sx, sy, a, b, t, u, v;
	FVector2D q;
	register int i, j;

	if (start)
	{
		start = 0;
		g2.SetNum(B + B + 2);
		g3.SetNum(B + B + 2);
		init();
	}

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
	a = lerp(sx, u, v);

	q = g2[b01]; u = at2(rx0, ry1);
	q = g2[b11]; v = at2(rx1, ry1);
	b = lerp(sx, u, v);

	return lerp(sy, a, b);
}

static void init()
{
	int i, j, k;

	for (i = 0; i < B; i++)
	{
		p[i] = i;

		const int32 Random1 = FMath::RandRange(0, 0x3fffffff /* RAND_MAX */);
		const int32 Random2 = FMath::RandRange(0, 0x3fffffff /* RAND_MAX */);
		const int32 Random3 = FMath::RandRange(0, 0x3fffffff /* RAND_MAX */);

		g1[i] = (float)((Random1 % (B + B)) - B) / B;

		for (j = 0; j < 2; j++)
		{
			g2[i][j] = (float)((Random2 % (B + B)) - B) / B;
			g2[i].Normalize();
		}

		for (j = 0; j < 3; j++)
		{
			g3[i][j] = (float)((Random3 % (B + B)) - B) / B;
			g3[i].Normalize();
		}
	}

	while (--i)
	{
		k = p[i];
		p[i] = p[j = FMath::Rand() % B];
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