// Fill out your copyright notice in the Description page of Project Settings.


#include "PerlinNoiseModule.h"

UPerlinNoiseModule::UPerlinNoiseModule()
{
   
}

UPerlinNoiseModule::UPerlinNoiseModule(int32 seed)
{
   this->Seed = seed;
}

void UPerlinNoiseModule::Init(int32 seed /* = 5 */)
{
    p.SetNum(B + B + 2);
    g.SetNum(B + B + 2);
    g1.SetNum(B + B + 2);
    g2.SetNum(B + B + 2);
    g3.SetNum(B + B + 2);

    FRandomStream rand(5);

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

float UPerlinNoiseModule::GetNoise2D_Implementation(float X, float Y)
{
    if(!bAlreadyInitilise)
    {
        bAlreadyInitilise = true;
        Init(Seed);
    }
    
    const auto sCurve = [](float t) -> float
	{
		return t * t * (3.0f - 2.0f * t);
	};

	const auto setup = [&](float c, int32& b0, int32& b1, float& r0, float& r1, float& t) -> void
	{
		t = c + N;
		b0 = ((int32)t) & BM;
		b1 = (b0 + 1) & BM;
		r0 = t - (int32)t;
		r1 = r0 - 1.;
	};

    const auto at2 = [](float rx, float ry, const FVector2D& q) -> float
	{
		return rx * q[0] + ry * q[1];
	};

    int32 bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, sx, sy, a, b, t, u, v;
	FVector2D q;
	int32 i, j;

	setup(X, bx0, bx1, rx0, rx1, t);
	setup(Y, by0, by1, ry0, ry1, t);

	i = p[bx0];
	j = p[bx1];

	b00 = p[i + by0];
	b10 = p[j + by0];
	b01 = p[i + by1];
	b11 = p[j + by1];

	sx = sCurve(rx0);
	sy = sCurve(ry0);

	q = g2[b00]; u = at2(rx0, ry0, q);
	q = g2[b10]; v = at2(rx1, ry0, q);
	a = FMath::Lerp(u, v, sx);

	q = g2[b01]; u = at2(rx0, ry1, q);
	q = g2[b11]; v = at2(rx1, ry1, q);
	b = FMath::Lerp(u, v, sx);

	return FMath::Lerp(a, b, sy);
}