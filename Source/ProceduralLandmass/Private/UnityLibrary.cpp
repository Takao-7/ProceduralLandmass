// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/UnityLibrary.h"
#include "Engine/Texture2D.h"
#include <ThreadSafeBool.h>


/////////////////////////////////////////////////////
			/* Noise functions */
/////////////////////////////////////////////////////
float UUnityLibrary::PerlinNoise(float x, float y)
{
	return PerlinNoise(FVector2D(x, y));
}

static FCriticalSection SemaphoreG1;
static FCriticalSection SemaphoreG2;
static FCriticalSection SemaphoreG3;
static FCriticalSection SemaphoreP;
static FCriticalSection SemaphoreG;

float UUnityLibrary::PerlinNoise(const FVector2D& vec)
{
	const int32 B = 256;
	const int32 N = 4096;
	static int32 p[B + B + 2];
	static float g[B + B + 2];
	const int32 BM = 255;

	static float g1[B + B + 2];
	static TArray<FVector2D> g2;
	static TArray<FVector> g3;
	
	const auto sCurve = [](float t) -> float
	{
		return t * t * (3.0f - 2.0f * t);
	};

	const auto setup = [&](int32 i, int32& b0, int32& b1, float& r0, float& r1, float& t) -> void
	{
		t = vec[i] + N;
		b0 = ((int32)t) & BM;
		b1 = (b0 + 1) & BM;
		r0 = t - (int32)t;
		r1 = r0 - 1.;
	};

	const auto at2 = [](float rx, float ry, const FVector2D& q) -> float
	{
		return rx * q[0] + ry * q[1];
	};

	static FThreadSafeBool bIsFirstCall = true;
	if (bIsFirstCall)
	{
		bIsFirstCall = false;

		FRandomStream rand(5);

		SemaphoreG.Lock();
		SemaphoreG1.Lock();
		SemaphoreG2.Lock();
		SemaphoreG3.Lock();
		SemaphoreP.Lock();

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

		SemaphoreG.Unlock();
		SemaphoreG1.Unlock();
		SemaphoreG2.Unlock();
		SemaphoreG3.Unlock();
		SemaphoreP.Unlock();
	}

	int32 bx0, bx1, by0, by1, b00, b10, b01, b11;
	float rx0, rx1, ry0, ry1, sx, sy, a, b, t, u, v;
	FVector2D q;
	int32 i, j;

	setup(0, bx0, bx1, rx0, rx1, t);
	setup(1, by0, by1, ry0, ry1, t);

	SemaphoreP.Lock();
	i = p[bx0];
	j = p[bx1];

	b00 = p[i + by0];
	b10 = p[j + by0];
	b01 = p[i + by1];
	b11 = p[j + by1];
	SemaphoreP.Unlock();

	sx = sCurve(rx0);
	sy = sCurve(ry0);

	SemaphoreG2.Lock();
	q = g2[b00]; u = at2(rx0, ry0, q);
	q = g2[b10]; v = at2(rx1, ry0, q);
	a = FMath::Lerp(u, v, sx);

	q = g2[b01]; u = at2(rx0, ry1, q);
	q = g2[b11]; v = at2(rx1, ry1, q);
	b = FMath::Lerp(u, v, sx);
	SemaphoreG2.Unlock();

	return FMath::Lerp(a, b, sy);
}


/////////////////////////////////////////////////////
					/* Texture */
/////////////////////////////////////////////////////
void UUnityLibrary::ReplaceTextureData(UTexture2D* texture, const TArray<FLinearColor>& newData, int32 mipToUpdate /*= 0*/)
{
	ReplaceTextureData(texture, (void*)newData.GetData(), mipToUpdate);
}

void UUnityLibrary::ReplaceTextureData(UTexture2D* texture, const void* newData, int32 mipToUpdate /*= 0*/)
{
	FTexture2DMipMap& Mip = texture->PlatformData->Mips[mipToUpdate]; /* A texture has several mips, which are basically their level of detail. Mip 0 is the highest detail. */
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE); /* Get the bulk (raw) data from the mip and lock it for read/write access. */
	FMemory::Memcpy(Data, newData, Mip.BulkData.GetBulkDataSize()); /* Copy the color map's raw data (the internal array) to the location of the texture's bulk data location. */
	Mip.BulkData.Unlock(); /* Unlock the bulk data. */
	texture->UpdateResource(); /* Update the texture, so that it's ready to be used. */
}


/////////////////////////////////////////////////////
				/* Multi-Threading */
/////////////////////////////////////////////////////
FAutoDeleteAsyncTask<UUnityLibrary::MyAsyncTask>* UUnityLibrary::CreateTask(TFunction<void(void)> workCallback, TFunction<void(void)> abandonCallback /*= nullptr*/, TFunction<bool(void)> canAbandonCallback /*= nullptr*/)
{
	FAutoDeleteAsyncTask<MyAsyncTask>* newTask = new FAutoDeleteAsyncTask<MyAsyncTask>(workCallback, abandonCallback, canAbandonCallback);
	return newTask;
}

FAutoDeleteAsyncTask<UUnityLibrary::MyAsyncTask>* UUnityLibrary::CreateTask(TFunction<void(void)> workCallback)
{
	FAutoDeleteAsyncTask<MyAsyncTask>* newTask = new FAutoDeleteAsyncTask<MyAsyncTask>(workCallback);
	return newTask;
}
