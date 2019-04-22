// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structs/MeshData.h"
#include "Structs/TerrainConfiguration.h"
#include "MeshDataJob.h"
#include "Queue.h"
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


/* Delegate for when the mesh data queue is finished. */
DECLARE_MULTICAST_DELEGATE(FOnMeshDataQueueFinished);


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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bClearTerrain = false;

	/* If true we will generate the map each time a value changes. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bAutoUpdate = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bVisualizeNormals = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	EDrawMode DrawMode;

	/* While in the editor override the LOD with this value for EVERY chunk. A value of -1 means no override. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = -1, ClampMax = 6), Category = "Map Generator|General")
	int32 EditorPreviewLevelOfDetail = -1;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	FTerrainConfiguration Configuration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	UMaterial* MeshMaterial;

	/* Queue for finished jobs */
	TQueue<FMeshDataJob, EQueueMode::Mpsc> FinishedMeshDataJobs;

	/* The number of unfinished mesh data jobs. */
	FThreadSafeCounter NumUnfinishedMeshDataJobs;

	/* Event when all mesh data jobs are completed. */
	FOnMeshDataQueueFinished MeshDataQueueFinishedDelegate;

private:
	/* Array of all worker threads. */
	TArray<FTerrainGeneratorWorker*> WorkerThreads;

	/* All chunks that belong to this terrain. */
	TMap<FVector2D, UTerrainChunk*> Chunks;
	
	FTimerHandle TimerHandleFinishedJobsQueue;
	FTimerHandle TimerHandleEditorTick;

protected:
	/* The configuration before it changed. Used to determent which values did change. */
	UPROPERTY(BlueprintReadWrite, Category = "Map Generator")
	FTerrainConfiguration OldConfiguration;


	/////////////////////////////////////////////////////
					/* Functions */
	/////////////////////////////////////////////////////
protected:
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void DrawTexture(UTexture2D* texture, float targetScale);
	
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void DrawMesh(FMeshData& meshData, UTexture2D* texture, UMaterial* material, UProceduralMeshComponent* mesh, float targetScale);

	/* Generates the entire terrain. Clears the current terrain chunks if existing. */
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void GenerateTerrain();

	/* Updates the terrain, if only the noise generator has changed. */
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void UpdateMap();

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void ClearTerrain();

	void CreateAndEnqueueMeshDataJob(UTerrainChunk* chunk, int32 levelOfDetail, int32 numVertices, bool bUpdateMeshSection = false, UNoiseGeneratorInterface* noiseGenerator = nullptr, const FVector2D& noiseOffset = FVector2D::ZeroVector);

	static FArray2D GenerateNoiseMap(int32 mapSize, float scale, float lacunarity, int32 octaves, float persistance = 0.5f, bool bOptimiseNormalization = false, const FVector2D& offset = FVector2D::ZeroVector, int32 seed = 42);

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void DrawMap(FArray2D &noiseMap, TArray<FLinearColor> colorMap);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ClearThreads();
	void ClearTimers();

public:	
	// Sets default values for this actor's properties
	ATerrainGenerator();
	~ATerrainGenerator();

	void CreateAndEnqueueMeshDataJob(UTerrainChunk* chunk, int32 levelOfDetail, bool bUpdateMeshSection = false, const FVector2D& offset = FVector2D::ZeroVector);
	
	virtual void Tick(float DeltaSeconds) override;
	
	/* Custom "Tick" function that runs in the editor. */
	void EditorTick();

	/* Returns the terrain size (in cm) along one direction. So the total terrain area is 2x this value.
	 * Takes the map scale into account! */
	float GetTerrainSize() const
	{
		return Configuration.MapScale * Configuration.GetChunkSize() * Configuration.NumChunks;
	};

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	static UTexture2D* TextureFromColorMap(const TArray<FLinearColor>& colorMap);

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	static UTexture2D* TextureFromHeightMap(const FArray2D& heightMap);
	
	void HandleFinishedMeshDataJobs();

	void UpdateAllChunks(int32 levelOfDetail = 0);

	/**
	 * Calculates the noise offset for the given x and y position in the chunk grid.
	 * 
	 * @param x Horizontal index 
	 * @param y Vertical index
	 */
	FVector2D CalculateNoiseOffset(int32 x, int32 y);

	FVector GetCameraLocation();
};
