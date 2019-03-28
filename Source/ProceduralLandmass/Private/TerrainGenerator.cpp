// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/TerrainGenerator.h"
#include "Public/NoiseLibrary.h"
#include "Array2D.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "RunnableThread.h"
#include "Runnable.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AsyncWork.h"


ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	bRunConstructionScriptOnDrag = false;

	RootComponent = Cast<USceneComponent>(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	MeshComponent = Cast<UProceduralMeshComponent>(CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh")));
	MeshComponent->SetupAttachment(RootComponent);

	PreviewPlane = Cast<UStaticMeshComponent>(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plane")));
	PreviewPlane->SetupAttachment(RootComponent);
	PreviewPlane->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	SetPlaneVisibility(false);
}

void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	PreviewPlane->SetMaterial(0, MeshMaterial);
	MeshComponent->ClearAllMeshSections();

	RequestMapData(nullptr);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::SetPlaneVisibility(bool bIsVisible)
{
	PreviewPlane->SetVisibility(bIsVisible);
	PreviewPlane->SetHiddenInGame(!bIsVisible);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::GenerateMap(bool bCreateNewMesh /*= false*/)
{
	FArray2D& noiseMap = UNoiseLibrary::GenerateNoiseMap(MapChunkSize, NoiseScale, Lacunarity, Octaves, Persistance, Offset, Seed);	
	
	TArray<FLinearColor> colorMap = TArray<FLinearColor>(); colorMap.SetNum(MapChunkSize * MapChunkSize);
	if(Regions.Num() > 0)
	{
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
	}
	else
	{
		const auto setColorMapToNoiseMap = [&](float& height, int32 x, int32 y)
		{
			colorMap[y * MapChunkSize + x] = FLinearColor(height, height, height);
		};
		noiseMap.ForEachWithIndex(setColorMapToNoiseMap);
	}

	if (bCreateNewMesh)
	{
		MeshComponent->ClearAllMeshSections();
	}

	DrawMap(noiseMap, colorMap);
}

/////////////////////////////////////////////////////
void ATerrainGenerator::DrawMap(FArray2D& noiseMap, TArray<FLinearColor> colorMap)
{
	switch (DrawMode)
	{
	case EDrawMode::NoiseMap:
		MeshComponent->ClearAllMeshSections();
		DrawTexture(TextureFromHeightMap(noiseMap), MapScale);
		break;
	case EDrawMode::ColorMap:
		MeshComponent->ClearAllMeshSections();
		DrawTexture(TextureFromColorMap(colorMap), MapScale);
		break;
	case EDrawMode::Mesh:
	{
		SetPlaneVisibility(false);
		FMeshData& meshData = FMeshData::GenerateMeshData(noiseMap, MeshHeightMultiplier, LevelOfDetail, MeshHeightCurve);
		UTexture2D* texture = TextureFromColorMap(colorMap);
		DrawMesh(meshData, texture, MeshMaterial, MeshComponent, MapScale);
		meshData.~FMeshData();
		break;
	}
	}

	noiseMap.~FArray2D();
}

/////////////////////////////////////////////////////
void ATerrainGenerator::DrawTexture(UTexture2D* texture, float targetScale)
{
	PreviewPlane->SetMaterial(0, MeshMaterial);
	UMaterialInstanceDynamic* material = PreviewPlane->CreateDynamicMaterialInstance(0);
	if(material)
	{
		material->SetTextureParameterValue("Texture", texture);
		
		const float textureSize = texture->GetSizeX();
		const float scaleFactor = textureSize / 100.0f;
		PreviewPlane->SetRelativeScale3D(FVector(scaleFactor * targetScale));

		SetPlaneVisibility(true);
	}
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

	FTexture2DMipMap& Mip = texture->PlatformData->Mips[0]; /* A texture has several mips, which are basically their level of detail. Mip 0 is the highest detail. */
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE); /* Get the bulk (raw) data from the mip and lock it for read/write access. */
	FMemory::Memcpy(Data, (void*)colorMap.GetData(), Mip.BulkData.GetBulkDataSize()); /* Copy the color map's raw data (the internal array) to the location of the texture's bulk data location. */
	Mip.BulkData.Unlock(); /* Unlock the bulk data. */
	texture->UpdateResource(); /* Update the texture, so that it's ready to be used. */

	return texture;
}

UTexture2D* ATerrainGenerator::TextureFromHeightMap(const FArray2D& heightMap)
{
	const int32 size = heightMap[0].Num();

	TArray<FLinearColor> colorMap; colorMap.SetNum(size * size);
	for (int32 y = 0; y < size; y++)
	{
		for (int32 x = 0; x < size; x++)
		{
			FLinearColor color = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor::White, heightMap[x][y]);
			colorMap[y * size + x] = color;
		}
	}

	return TextureFromColorMap(colorMap);
}

/////////////////////////////////////////////////////
template <typename Lambda>
void ATerrainGenerator::RequestMapData(Lambda callback)
{
	//FMapBuilderRunnable* runnable = new FMapBuilderRunnable(this);
	//FRunnableThread::Create(runnable, TEXT("Test"));

	(new FAutoDeleteAsyncTask<ExampleAutoDeleteAsyncTask>(this))->StartBackgroundTask();
}

void ATerrainGenerator::MapDataThread()
{
	UKismetSystemLibrary::PrintString(this, TEXT("Thread is running. "));
}


class FMapBuilderRunnable : public FRunnable
{
private:
	ATerrainGenerator* Generator;

public:
	FMapBuilderRunnable () {};
	FMapBuilderRunnable(ATerrainGenerator* callBackObject)
	{
		Generator = callBackObject;
	};
	~FMapBuilderRunnable() {};


	virtual bool Init() override
	{
		return true;
	}

	virtual uint32 Run() override
	{
		return 0;
	}

	virtual void Stop() override
	{
		
	}

	virtual void Exit() override
	{
		
	}

	virtual class FSingleThreadRunnable* GetSingleThreadInterface() override
	{
		return nullptr;
	}
};


class ExampleAutoDeleteAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<ExampleAutoDeleteAsyncTask>;

	ATerrainGenerator* ExampleData;

	ExampleAutoDeleteAsyncTask(ATerrainGenerator* InExampleData)
	{
		ExampleData = InExampleData;
	};

	void DoWork()
	{
		// Do work here.
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(ExampleAutoDeleteAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};

