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
#include "TerrainGeneratorWorker.h"


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

	/*if (!MeshDataThreadInfoQueue.IsEmpty() && !CriticalSectionMeshDataQueue.TryLock())
	{
		FMapThreadInfo<FMeshData> threadInfo;
		MeshDataThreadInfoQueue.Dequeue(threadInfo);
		threadInfo.Callback(threadInfo.Parameter);
	}*/
}

/////////////////////////////////////////////////////
void ATerrainGenerator::GenerateMap(bool bCreateNewMesh /*= false*/)
{
	const int32 numThreads = Configuration.NumberOfThreads;
	if (WorkerThreads.Num() != 0)
	{
		/* Clean all current worker threads. */
		for (FTerrainGeneratorWorker* worker : WorkerThreads)
		{
			worker->bKill = true;
		}

		WorkerThreads.Empty(numThreads);
	}

	for (int32 i = 0; i < numThreads; i++)
	{
		WorkerThreads[i] = new FTerrainGeneratorWorker();
	}

	/* Clean up chunks. */
	const int32 numberOfChunks = Configuration.NumChunksPerDirection * 2;
	if (Chunks.Num() != numberOfChunks)
	{
		for (UTerrainChunk* chunk : Chunks)
		{
			chunk->DestroyComponent();
		}

		Chunks.SetNum(0);
	}

	for (int32 i = 0; i < numberOfChunks; ++i)
	{
		Chunks[i]->ClearAllMeshSections();
		
	}
	
	const int32 maxLOD = Configuration.LODs.Last().LOD;
	for (int32 i = 0; i < numberOfChunks; i++)
	{
		FMeshDataJob newJob = FMeshDataJob(Configuration.NoiseGenerator, , Configuration.Amplitude, maxLOD, Configuration.ChunkSize, Configuration.HeightCurve);
		WorkerThreads[i % numThreads]->PendingJobs.Enqueue(newJob);
	}
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
