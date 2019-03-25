// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PLG_Noise.generated.h"


/**
 * 
 */
UCLASS(Abstract)
class PROCEDURALLANDMASS_API UPLG_Noise : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @return 
	 */
	static TMap<int32, TArray<float>> GenerateNoiseMap(int mapSize, float scale);
	
};
