// Fill out your copyright notice in the Description page of Project Settings.

#include "NoiseGeneratorInterface.h"
#include "UnityLibrary.h"


float INoiseGeneratorInterface::GetNoise1D_Implementation(float x)
{
	return 0.0f;
	//return FMath::PerlinNoise1D(x);
}

float INoiseGeneratorInterface::GetNoise2D_Implementation(float x, float y)
{
	return UUnityLibrary::PerlinNoise(x, y);
}

float INoiseGeneratorInterface::GetNoise3D_Implementation(float x, float y, float z)
{
	return GetNoise1D(x) * GetNoise2D(x, y);
}