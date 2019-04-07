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
#include "Public/TerrainChunk.h"


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	bRunConstructionScriptOnDrag = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

/////////////////////////////////////////////////////
void ATerrainGenerator::GenerateMap()
{
	const int32 numThreads = Configuration.GetNumberOfThreads();
	const int32 chunksPerDirection = Configuration.NumChunksPerDirection;	
	const int32 chunkSize = Configuration.ChunkSize;
	const int32 maxLOD = Configuration.LODs.Last().LOD;
	
	
	/* Clean all current worker threads. */
	if (WorkerThreads.Num() != 0)
	{
		WorkerThreads.Empty(numThreads);
	}
	WorkerThreads.SetNum(numThreads);		

	/* Create new worker threads. */
	for (int32 i = 0; i < numThreads; i++)
	{
		WorkerThreads[i] = new FTerrainGeneratorWorker();
	}

	/* Clean up chunks. */
	const int32 totalNumberOfChunks = chunksPerDirection * 2;
	if (Chunks.Num() != 0)
	{
		Chunks.Empty(totalNumberOfChunks);
	}
	Chunks.SetNum(totalNumberOfChunks);

	/* Calculate the top positions for chunks. */
	const int32 totalHeight = chunkSize * chunksPerDirection;
	const float topLeftPositionX = (totalHeight - chunkSize) / -2.0f;
	const float topLeftPositionY = (totalHeight - chunkSize) / 2.0f;

	/* Create chunks, mesh data jobs and add them to the worker threads. */
	for (int32 y = 0; y < chunksPerDirection; ++y)
	{
		for (int32 x = 0; x < chunksPerDirection; ++x)
		{
			const int32 i = y + x;
			const FVector2D offset = FVector2D(topLeftPositionX + (x * chunkSize), topLeftPositionY + (y * chunkSize));
			
			const FName chunkName = *FString::Printf(TEXT("Terrain chunk %d"), i);
			UTerrainChunk* newChunk = NewObject<UTerrainChunk>(this, chunkName);
			newChunk->InitChunk(10000.0f, this, &Configuration.LODs, nullptr, 0.0f);
			newChunk->SetRelativeLocation(FVector(offset, 0.0f));
			
			FMeshDataJob newJob = FMeshDataJob(Configuration.NoiseGenerator, newChunk, Configuration.Amplitude, maxLOD, chunkSize, offset, Configuration.HeightCurve);
			WorkerThreads[i % numThreads]->PendingJobs.Enqueue(newJob);

			Chunks[i] = newChunk;
		}
	}

	SetActorScale3D(FVector(MapScale));
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

void ATerrainGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FMeshDataJob job;
	if(FinishedMeshDataJobs.Dequeue(job))
	{
		FMeshData* meshData = job.GeneratedMeshData;
		job.Chunk->bUseAsyncCooking = true;
		job.Chunk->CreateMeshSection(0, meshData->Vertices, meshData->Triangles, meshData->Normals, meshData->UVs, meshData->VertexColors, meshData->Tangents, true);

		//delete job.GeneratedMeshData;
		//delete job.GeneratedHeightMap;
	}
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
