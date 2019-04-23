// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "NoiseGeneratorInterface.generated.h"


UCLASS(BlueprintType, meta = (DisplayName = "Noise Generator"))
class UNoiseGeneratorInterface : public UObject
{
	GENERATED_BODY()

public:
	virtual void CopySettings(const UNoiseGeneratorInterface* other) {};

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Noise Generator Interface", meta = (DisplayName = "GetNoise 1D"))
	float GetNoise1D_K2(float x);
	
	UFUNCTION()
	virtual float GetNoise1D(float x);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Noise Generator Interface", meta = (DisplayName = "GetNoise 2D"))
	float GetNoise2D_K2(float x, float y);

	UFUNCTION()
	virtual float GetNoise2D(float x, float y);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Noise Generator Interface", meta = (DisplayName = "GetNoise 3D"))
	float GetNoise3D_K2(float x, float y, float z);
	
	UFUNCTION()
	virtual float GetNoise3D(float x, float y, float z);
};