// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/PLG_MapDisplay.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

void UPLG_MapDisplay::DrawNoiseMap(TMap<int32, TArray<float>> noiseMap)
{
	int32 height = noiseMap.Num();
	int32 width = height;
	
	TArray<FColor> colourMap = TArray<FColor>();
	colourMap.SetNum(width * height);

	for (int32 y = 0; y < height; y++)
	{
		for (int32 x = 0; x < width; x++)
		{
			TArray<float> row = *noiseMap.Find(y);
			FLinearColor color = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor::White, row[x]);
			colourMap[y * width + x] = color.ToRGBE();
		}
	}

	UTexture2D* texture = UTexture2D::CreateTransient(width, height);
	FTexture2DMipMap& Mip = texture->PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, (void*)colourMap.GetData(), Mip.BulkData.GetBulkDataSize());
	Mip.BulkData.Unlock();
	texture->UpdateResource();
	
	if(RenderTarget)
	{
		UStaticMeshComponent* mesh = Cast<UStaticMeshComponent>(RenderTarget->GetComponentByClass(UStaticMeshComponent::StaticClass()));
		if(mesh)
		{
			UMaterialInstanceDynamic* material = mesh->CreateDynamicMaterialInstance(0);
			material->SetTextureParameterValue("Texture", texture);
		}
	}
}

// Sets default values for this component's properties
UPLG_MapDisplay::UPLG_MapDisplay()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPLG_MapDisplay::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UPLG_MapDisplay::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

