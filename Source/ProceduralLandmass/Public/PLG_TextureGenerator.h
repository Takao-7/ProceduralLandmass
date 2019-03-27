// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PLG_TextureGenerator.generated.h"

class UTexture2D;
struct FLinearColor;
struct FArray2D;

/**
 * 
 */
UCLASS()
class PROCEDURALLANDMASS_API UPLG_TextureGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static UTexture2D* TextureFromColorMap(const TArray<FLinearColor>& colorMap);
	static UTexture2D* TextureFromHeightMap(const FArray2D& heightMap);
	
};
