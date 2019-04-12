// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Interface.h"
#include "NoiseGeneratorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType, meta = (DisplayName = "Noise Generator"))
class UNoiseGeneratorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for noise generators.
 */
class PROCEDURALLANDMASS_API INoiseGeneratorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Noise Generator Interface")
	float GetNoise1D(float x);
	virtual float GetNoise1D_Implementation(float x);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Noise Generator Interface")
	float GetNoise2D(float x, float y);
	virtual float GetNoise2D_Implementation(float x, float y);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Noise Generator Interface")
	float GetNoise3D(float x, float y, float z);
	virtual float GetNoise3D_Implementation(float x, float y, float z);
};
