// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/TerrainGenerator.h"
#include "Array2D.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Public/UnityLibrary.h"
#include "PerlinNoiseGenerator.h"


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	bRunConstructionScriptOnDrag = false;

	RootComponent = Cast<USceneComponent>(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
}

void ATerrainGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	/*if (!MapDataThreadInfoQueue.IsEmpty() && !CriticalSectionMapDataQueue.TryLock())
	{
		FMapThreadInfo<FMapData> threadInfo;
		MapDataThreadInfoQueue.Dequeue(threadInfo);
		threadInfo.Callback(threadInfo.Parameter);
	}*/

	if (!MeshDataThreadInfoQueue.IsEmpty() && !CriticalSectionMeshDataQueue.TryLock())
	{
		FMapThreadInfo<FMeshData> threadInfo;
		MeshDataThreadInfoQueue.Dequeue(threadInfo);
		threadInfo.Callback(threadInfo.Parameter);
	}
}

/////////////////////////////////////////////////////
void ATerrainGenerator::GenerateMap(bool bCreateNewMesh /*= false*/)
{
	
	//GenerateMapData(FVector2D::ZeroVector);

	//DrawMap(mapData.HeightMap, mapData.ColorMap);
}

//FMapData ATerrainGenerator::GenerateMapData(FVector2D center)
//{
//	FArray2D noiseMap = GenerateNoiseMap(MapChunkSize, NoiseScale, Lacunarity, Octaves, Persistance, false, center + Offset, Seed);
//
//	TArray<FLinearColor> colorMap = TArray<FLinearColor>(); colorMap.SetNum(MapChunkSize * MapChunkSize);
//	if (Regions.Num() > 0)
//	{
//		/* Iterate over the noise map and based of the height value set the region color to the color map. */
//		noiseMap.ForEachWithIndex([&](float& height, int32 x, int32 y)
//		{
//			for (const FTerrainType& region : Regions)
//			{
//				if (height <= region.StartHeight)
//				{
//					colorMap[y * MapChunkSize + x] = region.Color;
//					break;
//				}
//			}
//		});
//	}
//	else
//	{
//		noiseMap.ForEachWithIndex([&](float& height, int32 x, int32 y)
//		{
//			colorMap[y * MapChunkSize + x] = FLinearColor(height, height, height);
//		});
//	}
//
//	return FMapData(noiseMap, colorMap, FVector(center + Offset, 0.0f) * 100.0f);
//}

/////////////////////////////////////////////////////
FArray2D ATerrainGenerator::GenerateNoiseMap(int32 mapSize, float scale, float lacunarity, int32 octaves, float persistance /*= 0.5f*/, bool bOptimiseNormalization /*= false*/, const FVector2D& offset /*= FVector2D::ZeroVector*/, int32 seed /*= 42*/)
{
	FArray2D noiseMap = FArray2D(mapSize, mapSize);

	/* Initialize a random number generator struct and seed it with the given value. */
	FRandomStream random(seed);

	TArray<FVector2D> octaveOffsets; octaveOffsets.SetNum(octaves);
	for (FVector2D& offsetVec : octaveOffsets)
	{
		float offsetX = random.FRandRange(-1000.0f, 1000.0f) + offset.X;
		float offsetY = random.FRandRange(-1000.0f, 1000.0f) + offset.Y;
		offsetVec = FVector2D(offsetX, offsetY);
	}

	const float halfSize = mapSize / 2.0;

	/* The actual min value in the noise map. */
	float minValue = 1.0f / SMALL_NUMBER;

	/* The actual max value in the noise map. */
	float maxValue = SMALL_NUMBER;

	/* The calculated theoretical highest value. */
	float limit = 0.0f;
	for (int32 i = 0; i < octaves; i++)
	{
		limit += FMath::Pow(persistance, i);
	}

	const auto generateNoise = [&](float& value, int32 x, int32 y)
	{
		float amplitude = 1.0f;
		float frequency = 1.0f;
		float noiseHeight = 0.0f;

		for (const FVector2D& octaveOffset : octaveOffsets)
		{
			const float sampleX = (x - halfSize) / scale * frequency + octaveOffset.X;
			const float sampleY = (y - halfSize) / scale * frequency + octaveOffset.Y;

			const float perlinValue = UUnityLibrary::PerlinNoise(sampleX, sampleY);
			noiseHeight += perlinValue * amplitude;

			amplitude *= persistance;
			frequency *= lacunarity;
		}

		if (noiseHeight > maxValue)
		{
			maxValue = noiseHeight;
		}
		else if (noiseHeight < minValue)
		{
			minValue = noiseHeight;
		}

		value = bOptimiseNormalization ? UKismetMathLibrary::NormalizeToRange(noiseHeight, -limit, limit) : noiseHeight;
	};
	noiseMap.ForEachWithIndex(generateNoise);

	if (!bOptimiseNormalization)
	{
		const auto normalizeValue = [minValue, maxValue](float& value) { value = UKismetMathLibrary::NormalizeToRange(value, minValue, maxValue); };
		noiseMap.ForEach(normalizeValue);
	}

	return noiseMap;
}

/////////////////////////////////////////////////////
void ATerrainGenerator::DrawMap(FArray2D& noiseMap, TArray<FLinearColor> colorMap)
{
	switch (DrawMode)
	{
	case EDrawMode::NoiseMap:
		DrawTexture(TextureFromHeightMap(noiseMap), MapScale);
		break;
	case EDrawMode::ColorMap:
		DrawTexture(TextureFromColorMap(colorMap), MapScale);
		break;
	case EDrawMode::Mesh:
	{
		/*SetPlaneVisibility(false);
		FMeshData meshData = FMeshData::GenerateMeshData(noiseMap, MeshHeightMultiplier, EditorPreviewLevelOfDetail, 0, FVector::ZeroVector, MeshHeightCurve);
		UTexture2D* texture = TextureFromColorMap(colorMap);
		DrawMesh(meshData, texture, MeshMaterial, MeshComponent, MapScale);*/
		break;
	}
	}

	noiseMap.~FArray2D();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::DrawTexture(UTexture2D* texture, float targetScale)
{
	/*PreviewPlane->SetMaterial(0, MeshMaterial);
	UMaterialInstanceDynamic* material = PreviewPlane->CreateDynamicMaterialInstance(0);
	if(material)
	{
		material->SetTextureParameterValue("Texture", texture);

		const float textureSize = texture->GetSizeX();
		const float scaleFactor = textureSize / 100.0f;
		PreviewPlane->SetRelativeScale3D(FVector(scaleFactor * targetScale));

		SetPlaneVisibility(true);
	}*/
}

void ATerrainGenerator::DrawMesh(FMeshData& meshData, UTexture2D* texture, UMaterial* material, UProceduralMeshComponent* mesh, float targetScale)
{
	meshData.CreateMesh(mesh);
	UMaterialInstanceDynamic* materialInstance = mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0, material);
	materialInstance->SetTextureParameterValue("Texture", texture);
	
	mesh->SetRelativeScale3D(FVector(targetScale));
}

/////////////////////////////////////////////////////
UTexture2D* ATerrainGenerator::TextureFromColorMap(const TArray<FLinearColor>& colorMap)
{
	const int32 size = FMath::Sqrt(colorMap.Num());

	UTexture2D* texture = UTexture2D::CreateTransient(size, size, EPixelFormat::PF_A32B32G32R32F); /* Create a texture with a 32 bit pixel format for the Linear Color. */
	texture->Filter = TextureFilter::TF_Nearest;
	texture->AddressX = TextureAddress::TA_Clamp;
	texture->AddressY = TextureAddress::TA_Clamp;
	
	UUnityLibrary::ReplaceTextureData(texture, colorMap, 0);

	return texture;
}

UTexture2D* ATerrainGenerator::TextureFromHeightMap(const FArray2D& heightMap)
{
	const int32 size = heightMap.GetHeight();

	TArray<FLinearColor> colorMap; colorMap.SetNum(size * size);
	for (int32 y = 0; y < size; y++)
	{
		for (int32 x = 0; x < size; x++)
		{
			FLinearColor color = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor::White, heightMap.GetValue(x, y));
			colorMap[y * size + x] = color;
		}
	}

	return TextureFromColorMap(colorMap);
}

/////////////////////////////////////////////////////
//void ATerrainGenerator::RequestMapData(FVector2D center, TFunction<void (FMapData)> callback)
//{
//	UUnityLibrary::CreateTask([=]() { MapDataThread(center, callback); })->StartBackgroundTask();
//}
//
//void ATerrainGenerator::MapDataThread(FVector2D center, TFunction<void (FMapData)> callback)
//{
//	FMapData mapData = GenerateMapData(center);
//	FMapThreadInfo<FMapData> threadInfo = FMapThreadInfo<FMapData>(callback, mapData);
//
//	{
//		FScopeLock ScopeLockMapData(&CriticalSectionMapDataQueue);
//		MapDataThreadInfoQueue.Enqueue(threadInfo);
//	}
//}

/////////////////////////////////////////////////////
//void ATerrainGenerator::RequestMeshData(FMapData mapData, int32 lod, TFunction<void(FMeshData)> callback)
//{
//	UUnityLibrary::CreateTask([=]() { MeshDataThread(mapData, lod, callback); })->StartBackgroundTask();
//}

//void ATerrainGenerator::MeshDataThread(FMapData mapData, int32 lod, TFunction<void(FMeshData)> callback)
//{
//	FMeshData meshData = FMeshData(mapData.HeightMap, MeshHeightMultiplier, lod, MeshHeightCurve);
//	FMapThreadInfo<FMeshData> threadInfo = FMapThreadInfo<FMeshData>(callback, meshData);
//
//	{
//		FScopeLock ScopeLockMeshData(&CriticalSectionMeshDataQueue);
//		MeshDataThreadInfoQueue.Enqueue(threadInfo);
//	}
//}
