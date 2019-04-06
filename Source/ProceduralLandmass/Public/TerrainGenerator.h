// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structs/MeshData.h"
#include "Structs/TerrainConfiguration.h"
#include "TerrainGenerator.generated.h"


class UTerrainChunk;
class UProceduralMeshComponent;
class UMaterial;
class UCurveFloat;
class USceneComponent;
class UTexture2D;
class AStaticMeshActor;
struct FMeshData;
struct FLinearColor;
class FTerrainGeneratorWorker;


USTRUCT(BlueprintType)
struct FTerrainType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	/* The minimum height that this layer will start at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float StartHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color;
};


template<typename T>
struct FMapThreadInfo
{
public:
	TFunction<void (T)> Callback;
	T Parameter;

	FMapThreadInfo() {};
	FMapThreadInfo(TFunction<void(T)> callback, T parameter) : Callback(callback), Parameter(parameter) {};
	~FMapThreadInfo() {};
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
					/* Parameters */
	/////////////////////////////////////////////////////
protected:
	/* Use this as a button to manually generate the map. 
	 * When @see bAutoUpdate is enabled, this has no effect. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bGenerateMap = false;

	/* If true we will generate the map each time a value changes. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bAutoUpdate = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	EDrawMode DrawMode;

	/* While in the editor override the LOD with this value for EVERY chunk. A value of -1 means no override. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = -1, ClampMax = 6), Category = "Map Generator|General")
	int32 EditorPreviewLevelOfDetail = -1;

	/* The scale of each chunk. MapScale * MapChunkSize * NumberOfChunks = Total map width in cm. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1.0f), Category = "Map Generator|General")
	float MapScale = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	FTerrainConfiguration Configuration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	TArray<FTerrainType> Regions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	UMaterial* MeshMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	UCurveFloat* MeshHeightCurve;

private:	
	FCriticalSection CriticalSectionMapDataQueue;
	FCriticalSection CriticalSectionMeshDataQueue;

	TArray<FTerrainGeneratorWorker*> WorkerThreads;
	TArray<UTerrainChunk*> Chunks;


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
	void GenerateMap();

	static FArray2D GenerateNoiseMap(int32 mapSize, float scale, float lacunarity, int32 octaves, float persistance = 0.5f, bool bOptimiseNormalization = false, const FVector2D& offset = FVector2D::ZeroVector, int32 seed = 42);

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void DrawMap(FArray2D &noiseMap, TArray<FLinearColor> colorMap);

public:	
	// Sets default values for this actor's properties
	ATerrainGenerator();

	/* Returns the terrain size (in cm) along one direction. So the total terrain area is 2x this value. */
	float GetTerrainSize() const { return MapScale * Configuration.ChunkSize * Configuration.NumChunksPerDirection; };

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	static UTexture2D* TextureFromColorMap(const TArray<FLinearColor>& colorMap);

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	static UTexture2D* TextureFromHeightMap(const FArray2D& heightMap);
	
	/*void RequestMapData(FVector2D center, TFunction<void (FMapData)> callback);
	void MapDataThread(FVector2D center, TFunction<void (FMapData)> callback);*/

	/*void RequestMeshData(FMapData mapData, int32 lod, TFunction<void(FMeshData)> callback);
	void MeshDataThread(FMapData mapData, int32 lod, TFunction<void(FMeshData)> callback);*/
};
