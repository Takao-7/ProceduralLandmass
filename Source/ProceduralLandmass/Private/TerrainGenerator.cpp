// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/TerrainGenerator.h"
#include "Array2D.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Public/UnityLibrary.h"
#include "PerlinNoiseGenerator.h"
#include "TerrainGeneratorWorker.h"
#include "Public/TerrainChunk.h"

/* Do we want to use threading for generating mesh data? */
#define NO_THREADS 1


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
	const int32 chunksPerDirection = Configuration.NumChunks;	
	const int32 chunkSize = Configuration.GetNumVertices();
	const int32 maxLOD = Configuration.LODs.Last().LOD;

	/* Create worker threads. */
	WorkerThreads.Empty();
	WorkerThreads.SetNum(numThreads);
	for (int32 i = 0; i < numThreads; i++)
	{
		WorkerThreads[i] = new FTerrainGeneratorWorker();
	}	

	const int32 totalNumberOfChunks = FMath::Pow(chunksPerDirection, 2);
	if (Chunks.Num() != totalNumberOfChunks)
	{
		Chunks.SetNum(totalNumberOfChunks);
	}

	/* The top positions for chunks. These are the chunk's relative positions to the terrain generator actor,
	 * measured from their centers. */
	const float topLeftChunkPositionX = (chunksPerDirection - 1) * chunkSize / -2.0f;
	const float topLeftChunkPositionY = (chunksPerDirection - 1) * chunkSize / 2.0f;

	/* The top positions. Used for noise generation. */
	const int32 totalHeight = chunkSize * chunksPerDirection;
	const float topLeftPositionX = totalHeight / -2.0f;
	const float topLeftPositionY = totalHeight / 2.0f;

	/* Create mesh data jobs and add them to the worker threads. */
	int32 i = 0;
	for (int32 y = 0; y < chunksPerDirection; ++y)
	{
		for (int32 x = 0; x < chunksPerDirection; ++x)
		{
			/* Spawn a new chunk if necessary. */
			if(Chunks[i] == nullptr)
			{
				const FName chunkName = *FString::Printf(TEXT("Terrain chunk %d"), i);
				UTerrainChunk* newChunk = NewObject<UTerrainChunk>(this, chunkName);
				newChunk->InitChunk(10000.0f, this, &Configuration.LODs, nullptr, 0.0f);
				Chunks[i] = newChunk;
			}

			const FVector2D positionOffset = FVector2D(topLeftChunkPositionX + (x * chunkSize), topLeftChunkPositionY - (y * chunkSize));	
			Chunks[i]->SetRelativeLocation(FVector(positionOffset, 0.0f));

			/* Create a new mesh data jump for the new chunk and assign it to a worker thread. */
			const FVector2D offset = FVector2D(topLeftPositionX + (x * chunkSize), topLeftPositionY + (y * chunkSize));
			FMeshDataJob newJob = FMeshDataJob(Configuration.NoiseGenerator, Chunks[i], Configuration.Amplitude, maxLOD, chunkSize, offset, Configuration.HeightCurve);

			#if NO_THREADS
				FTerrainGeneratorWorker::DoWork(newJob);
			#else
				FTerrainGeneratorWorker* worker = WorkerThreads[i % numThreads];
				worker->PendingJobs.Enqueue(newJob);
				worker->UnPause();
			#endif

			i++;
		}
	}

	SetActorScale3D(Configuration.MapScale);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::DrawMap(FArray2D& noiseMap, TArray<FLinearColor> colorMap)
{
	switch (DrawMode)
	{
	case EDrawMode::NoiseMap:
		DrawTexture(TextureFromHeightMap(noiseMap), Configuration.MapScale.X);
		break;
	case EDrawMode::ColorMap:
		DrawTexture(TextureFromColorMap(colorMap), Configuration.MapScale.X);
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
