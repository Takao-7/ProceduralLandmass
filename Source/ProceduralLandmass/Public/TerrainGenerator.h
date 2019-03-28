// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structs/MeshData.h"
#include "TerrainGenerator.generated.h"


class UProceduralMeshComponent;
class UMaterial;
class UCurveFloat;
class USceneComponent;
class UTexture2D;
class AStaticMeshActor;
struct FMeshData;
struct FLinearColor;


USTRUCT(BlueprintType)
struct FTerrainType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color;
};


USTRUCT(BlueprintType)
struct FMapData
{
	GENERATED_BODY()

public:
	FArray2D HeightMap;
	TArray<FLinearColor> ColorMap;

	FMapData() {};
	FMapData(FArray2D noiseMap, TArray<FLinearColor> colorMap)
	{
		HeightMap = noiseMap;
		ColorMap = colorMap;
	};
	~FMapData() {};
};


UENUM(BlueprintType)
enum class EDrawMode : uint8
{
	NoiseMap,
	ColorMap,
	Mesh
};


UCLASS()
class PROCEDURALLANDMASS_API ATerrainGenerator : public AActor
{
	GENERATED_BODY()


	/////////////////////////////////////////////////////
					/* Components */
	/////////////////////////////////////////////////////
protected:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Map Generator")
	UProceduralMeshComponent* MeshComponent;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Map Generator")
	UStaticMeshComponent* PreviewPlane;


	/////////////////////////////////////////////////////
					/* Parameters */
	/////////////////////////////////////////////////////
protected:
	/* Use this as a button to manually generate the map. 
	 * When @see bAutoUpdate is enabled, this has no effect. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bGenerateMap = false;

	/* If true we will generate the noise map each time a value changes. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bAutoUpdate = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	EDrawMode DrawMode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 0, ClampMax = 6), Category = "Map Generator|General")
	int32 LevelOfDetail = 0;

	/* The overall map scale. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1.0f), Category = "Map Generator|General")
	float MapScale = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 0.001f), Category = "Map Generator|Settings")
	float NoiseScale = 30.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 0), Category = "Map Generator|Settings")
	int32 Octaves = 4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 0.0f, ClampMax = 1.0f), Category = "Map Generator|Settings")
	float Persistance = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1.0f), Category = "Map Generator|Settings")
	float Lacunarity = 2.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|Settings")
	int32 Seed = 42;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|Settings")
	FVector2D Offset = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|Settings")
	float MeshHeightMultiplier = 10.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	TArray<FTerrainType> Regions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	UMaterial* MeshMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	UCurveFloat* MeshHeightCurve;

public:
	/* The map chunk size. This is the number of vertices per line, per chunk.
	 * Change witch caution. Default is 241. */
	static const int32 MapChunkSize = 241;


	/////////////////////////////////////////////////////
					/* Functions */
	/////////////////////////////////////////////////////
protected:
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void DrawTexture(UTexture2D* texture, float targetScale);
	
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void DrawMesh(FMeshData& meshData, UTexture2D* texture, UMaterial* material, UProceduralMeshComponent* mesh, float targetScale);

	UFUNCTION(BlueprintPure, Category = "Map Generator")
	bool ShouldGenerateMap() const { return bAutoUpdate || bGenerateMap; };

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void GenerateMap(bool bCreateNewMesh = false);

	void DrawMap(FArray2D &noiseMap, TArray<FLinearColor> colorMap);

	virtual void BeginPlay() override;
	
public:	
	// Sets default values for this actor's properties
	ATerrainGenerator();

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	static UTexture2D* TextureFromColorMap(const TArray<FLinearColor>& colorMap);

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	static UTexture2D* TextureFromHeightMap(const FArray2D& heightMap);

	UFUNCTION(BlueprintPure, Category = "Map Generator")
	static int32 GetMapChunkSize() { return MapChunkSize; };

	/* Lambda type: (FMapData)->void */	
	template<typename Lambda>
	void RequestMapData(Lambda callback);

	void MapDataThread();

private:
	void SetPlaneVisibility(bool bIsVisible);
};
