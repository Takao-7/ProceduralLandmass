// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/PLG_MapGenerator.h"
#include "Public/PLG_NoiseLibrary.h"
#include "Array2D.h"
#include "Public/PLG_TextureGenerator.h"
#include "ProceduralMeshComponent.h"
#include "PLG_MeshGenerator.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"


APLG_MapGenerator::APLG_MapGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	bRunConstructionScriptOnDrag = false;

	RootComponent = Cast<USceneComponent>(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	MeshComponent = Cast<UProceduralMeshComponent>(CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh")));
	MeshComponent->SetupAttachment(RootComponent);

	PreviewPlane = Cast<UStaticMeshComponent>(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane")));
	PreviewPlane->SetupAttachment(RootComponent);
	SetPlaneVisibility(false);
}

void APLG_MapGenerator::SetPlaneVisibility(bool bIsVisible)
{
	PreviewPlane->SetVisibility(bIsVisible);
	PreviewPlane->SetHiddenInGame(!bIsVisible);
}

void APLG_MapGenerator::GenerateMap(bool bCreateNewMesh /*= false*/)
{
	FArray2D& noiseMap = UNoiseLibrary::GenerateNoiseMap(MapChunkSize, NoiseScale, Lacunarity, Octaves, Persistance, Offset, Seed);	
	
	TArray<FLinearColor> colorMap = TArray<FLinearColor>(); colorMap.SetNum(MapChunkSize * MapChunkSize);
	/* Iterate over the noise map and based of the height value set the region color to the color map. */
	const auto setRegionToColorMap = [&](float& height, int32 x, int32 y)
	{
		for (const FTerrainType& region : Regions)
		{
			if (height <= region.Height)
			{
				colorMap[y * MapChunkSize + x] = region.Color;
				break;
			}
		}
	};
	noiseMap.ForEachWithIndex(setRegionToColorMap);

	if (bCreateNewMesh)
	{
		MeshComponent->ClearAllMeshSections();
	}

	switch (DrawMode)
	{
	case EDrawMode::NoiseMap:
		MeshComponent->ClearAllMeshSections();
		DrawTexture(UPLG_TextureGenerator::TextureFromHeightMap(noiseMap), TargetMapSize);
		break;
	case EDrawMode::ColorMap:
		MeshComponent->ClearAllMeshSections();
		DrawTexture(UPLG_TextureGenerator::TextureFromColorMap(colorMap), TargetMapSize);
		break;
	case EDrawMode::Mesh:
	{
		SetPlaneVisibility(false);
		FMeshData& meshData = UPLG_MeshGenerator::GenerateMeshData(noiseMap, MeshHeightMultiplier, LevelOfDetail, TargetMapSize, MeshHeightCurve);
		UTexture2D* texture = UPLG_TextureGenerator::TextureFromColorMap(colorMap);
		DrawMesh(meshData, texture, MeshMaterial, MeshComponent, TargetMapSize);
		break;
	}
	}
}

void APLG_MapGenerator::DrawTexture(UTexture2D* texture, float targetSize)
{
	UMaterialInstanceDynamic* material = PreviewPlane->CreateDynamicMaterialInstance(0);
	if(material)
	{
		material->SetTextureParameterValue("Texture", texture);

		const int32 textureSize = texture->GetSizeX();
		const float scaleFactor = targetSize / textureSize;
		PreviewPlane->SetRelativeScale3D(FVector(scaleFactor));

		SetPlaneVisibility(true);
	}
}

void APLG_MapGenerator::DrawMesh(FMeshData& meshData, UTexture2D* texture, UMaterial* material, UProceduralMeshComponent* mesh, float targetSize)
{
	meshData.CreateMesh(mesh);

	const int32 textureSize = texture->GetSizeX();
	const float scaleFactor = targetSize / textureSize;
	mesh->SetRelativeScale3D(FVector(scaleFactor));

	UMaterialInstanceDynamic* materialInstance = mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, material);
	materialInstance->SetTextureParameterValue("Texture", texture);
}

void APLG_MapGenerator::SetComponentScaleToTextureSize(USceneComponent* component, const UTexture2D* texture, int32 initialSize /*= 1*/)
{
	const int32 size = texture->GetSizeX();
	FVector newScale = FVector(size) / initialSize;
	component->SetRelativeScale3D(newScale);
}
