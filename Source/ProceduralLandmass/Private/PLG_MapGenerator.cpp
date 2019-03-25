// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/PLG_MapGenerator.h"
#include "Public/PLG_Noise.h"
#include "Public/PLG_MapDisplay.h"

void APLG_MapGenerator::GenerateMap()
{
	TMap<int32, TArray<float>> noiseMap = UPLG_Noise::GenerateNoiseMap(Size, NoiseScale);
	MapDisplay->DrawNoiseMap(noiseMap);
}

// Sets default values
APLG_MapGenerator::APLG_MapGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MapDisplay = Cast<UPLG_MapDisplay>(CreateDefaultSubobject<UPLG_MapDisplay>(TEXT("Map display")));
	AddOwnedComponent(MapDisplay);
}

// Called when the game starts or when spawned
void APLG_MapGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APLG_MapGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

