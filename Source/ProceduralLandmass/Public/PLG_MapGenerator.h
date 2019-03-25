// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PLG_MapGenerator.generated.h"

class UPLG_MapDisplay;

UCLASS()
class PROCEDURALLANDMASS_API APLG_MapGenerator : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Size = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float NoiseScale = 0.3f;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	UPLG_MapDisplay* MapDisplay;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bGenerateNoiseMap;

public:
	UFUNCTION(BlueprintCallable)
	void GenerateMap();
	
public:	
	// Sets default values for this actor's properties
	APLG_MapGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
