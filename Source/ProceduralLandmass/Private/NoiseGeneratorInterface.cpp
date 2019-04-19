// Fill out your copyright notice in the Description page of Project Settings.

#include "NoiseGeneratorInterface.h"
#include "UnityLibrary.h"


float UNoiseGeneratorInterface::GetNoise1D(float x)
{
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION > 20)
	return FMath::PerlinNoise1D(x);
#else
	return UUnityLibrary::PerlinNoise(x, x);
#endif
}

float UNoiseGeneratorInterface::GetNoise2D(float x, float y)
{
	return UUnityLibrary::PerlinNoise(x, y);
}

float UNoiseGeneratorInterface::GetNoise3D(float x, float y, float z)
{
	return GetNoise1D_K2(x) * GetNoise2D(x, y);
}