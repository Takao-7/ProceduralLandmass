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
struct FTerrainMeshData;
struct FLinearColor;
class FTerrainGeneratorWorker;


UENUM(BlueprintType)
enum class EDrawMode : uint8
{
	/* Only display the noise map on a flat plane. */
	NoiseMap,
	/* Display the texture on a flat plane. */
	ColorMap,
	/* Generate and display the whole mesh. */
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
	/** 
	 * Use this as a button to (re-) generate the map.
	 * This will clear the entire map and all of it's chunks. 
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bGenerateMap = false;
	
	/**
	 * Just update the terrain. This will not clear the entire map and is identically
	 * if @see bAutoUpdate is set to true and a value was changed.
	 * Use this if you have updated the float curve.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bUpdateTerrain = false;

	/**
	 * Destroy all chunks and terrain workers. After doing this, the terrain
	 * has the same state as when it was spawned.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bClearTerrain = false;
	
	/* If true we will generate the map each time a value changes. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	bool bAutoUpdate = true;
		
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator|General")
	EDrawMode DrawMode = EDrawMode::Mesh;

	/* Array of all worker threads. Pointers can be null. */
	TArray<FTerrainGeneratorWorker*> WorkerThreads;

	/* All chunks that belong to this terrain. */
	TMap<FVector2D, UTerrainChunk*> Chunks;

	/* Timer handle for @see FinishedMeshDataJobs */
	FTimerHandle THFinishedJobsQueue;

	/* Timer handle for @see EditorTick() */
	FTimerHandle THEditorTick;

	/* The configuration before it changed. Used to determent which values did change. */
	FTerrainConfiguration OldConfiguration;
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	FTerrainConfiguration Configuration;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Map Generator")
	UMaterial* TerrainMaterial;
	
	/* Queue for finished jobs */
	TQueue<FMeshDataJob, EQueueMode::Mpsc> FinishedMeshDataJobs;
		
	
	/////////////////////////////////////////////////////
					/* Functions */
	/////////////////////////////////////////////////////
public:
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void CreateAndEnqueueMeshDataJob(UTerrainChunk* chunk, int32 levelOfDetail, bool bUpdateMeshSection = false, const FVector2D& offset = FVector2D::ZeroVector);

	/** Returns the actual terrain size (in cm) along one direction (= edge length).
	 * Takes the map scale into account! */
	UFUNCTION(BlueprintPure, Category = "Map Generator")
	FORCEINLINE float GetTerrainSize() const
	{
		return Configuration.MapScale * Configuration.GetChunkSize() * Configuration.NumChunks;
	};

	ATerrainGenerator();
	~ATerrainGenerator();

protected:
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void OnGenerateMapClicked();

	/* Generates the entire terrain. Clears the current terrain chunks if existing (@see ClearTerrain). */
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void GenerateTerrain();
	
	/* Updates the terrain, if only the noise generator has changed. */
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void UpdateMap();
	
	/* Removes all timers for this object, clears all threads and destroys all chunks. */
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void ClearTerrain();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/**
	 * Custom "Tick" function that runs in the editor.
	 * Updates the chunk LOD (@see UpdateChunkLOD) and handles
	 * finished mesh data jobs (@see HandleFinishedMeshDataJobs).
	 */ 
	void EditorTick();

	void UpdateChunkLOD();
	
	void ClearThreads();
	void ClearTimers();
	
	void HandleFinishedMeshDataJobs();
};
