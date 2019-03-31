#pragma once
#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "TerrainGenerator.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TerrainChunk.generated.h"


USTRUCT()
struct FLODMesh
{
	GENERATED_BODY()

public:
	bool bHasRequestedMesh;
	bool bHasMesh;
	int32 LOD;
	FMeshData MeshData;

private:
	ATerrainGenerator* Generator;

public:
	FLODMesh() {};
	FLODMesh(int32 lod, ATerrainGenerator* generator)
	{
		this->LOD = lod;
		this->Generator = generator;
	};

	void RequestMesh(FMapData mapData)
	{
		bHasRequestedMesh = true;
		Generator->RequestMeshData(mapData, LOD, [&](FMeshData meshData) { OnMeshDataRecieved(meshData); });
	};

	void OnMeshDataRecieved(FMeshData meshData)
	{
		bHasMesh = true;
		this->MeshData = meshData;
	}
};


USTRUCT(BlueprintType)
struct FLODInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, ClampMax = 6))
	int32 LOD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0.0f))
	float VisibleDistanceThreshold;
};


USTRUCT(BlueprintType)
struct FTerrainChunk
{
	GENERATED_BODY()

public:
	/* This chunk's world position, measured from it's center point. */
	UPROPERTY(BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	AActor* Viewer;

protected:
	UPROPERTY(BlueprintReadWrite)
	UProceduralMeshComponent* MeshComponent = nullptr;

	UPROPERTY(BlueprintReadWrite)
	float MaxViewDistance = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<AActor> TerrainClass = nullptr;

	UPROPERTY(BlueprintReadWrite)
	UWorld* World = nullptr;

	/* The chunk size (edge length) in cm. */
	UPROPERTY(BlueprintReadWrite)
	float ChunkSize;

private:
	ATerrainGenerator* TerrainGenerator;	
	TArray<FLODInfo>* DetailLevels;
	TArray<FLODMesh> LODmeshes;

	FMapData MapData;
	int32 MeshSectionIndex = 0;
	bool bMapDataRecieved = false;
	int32 PreviousLODindex = -1;


	/////////////////////////////////////////////////////
public:
	FTerrainChunk() {};

	/**
	 * @param coordination The chunks coordination in relation to the terrain grid. The chunk at the center would be at 0:0.
	 */
	FTerrainChunk(const FVector2D& coordinates, float viewDistance, int32 size, float scale, AActor* parent, TSubclassOf<AActor> terrainClass, 
		ATerrainGenerator* terrainGenerator, TArray<FLODInfo>* lodInfoArray, AActor* viewer, float zPosition = 0.0f)
	{
		ChunkSize = (float)size * scale;
		Position = FVector(coordinates * ChunkSize, zPosition);
		MaxViewDistance = viewDistance;
		World = parent->GetWorld();
		TerrainClass = terrainClass;
		DetailLevels = lodInfoArray;
		TerrainGenerator = terrainGenerator;
		Viewer = viewer;
		MeshComponent = TerrainGenerator->GetMeshComponent();

		if (TerrainClass && World)
		{
			const FTransform spawnTransform = FTransform(FRotator::ZeroRotator, Position, FVector::OneVector);
			FActorSpawnParameters spawnParams;
			spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			spawnParams.Owner = parent;
			
			AActor* meshActor = World->SpawnActor<AActor>(TerrainClass, spawnTransform, spawnParams);
			MeshComponent = Cast<UProceduralMeshComponent>(meshActor->GetComponentByClass(UProceduralMeshComponent::StaticClass()));
			meshActor->AttachToActor(parent, FAttachmentTransformRules::KeepWorldTransform);
			MeshComponent->SetRelativeScale3D(FVector(scale));
		}
		
		SetIsVisible(false);
		LODmeshes.SetNum(DetailLevels->Num());
		for (int32 i = 0; i < LODmeshes.Num(); i++)
		{
			LODmeshes[i] = FLODMesh((*DetailLevels)[i].LOD, TerrainGenerator);
		}

		TerrainGenerator->RequestMapData(FVector2D(Position),[&](FMapData mapData) { OnMapDataRecieved(mapData); });
	};

	/* Updates this chunk's visibility based on the viewer position. If the distance to the given position is
	 * smaller than the MaxViewDistance, than this chunk will be set to visible.
	 * @return The new visibility. */
	bool UpdateTerrainChunk()
	{
		const FVector viewerPosition = Viewer ? Viewer->GetActorLocation() : FVector::ZeroVector;
		const float viewerDistanceToCenter = FVector::Distance(Position, viewerPosition);

		const bool bIsVisible = (viewerDistanceToCenter <= MaxViewDistance) || viewerDistanceToCenter < ChunkSize;
		SetIsVisible(bIsVisible);
		if (bIsVisible && bMapDataRecieved)
		{
			int32 lodIndex = 0;
			for (int32 i = 0; i < DetailLevels->Num() - 1; i++)
			{
				if (viewerDistanceToCenter > (*DetailLevels)[i].VisibleDistanceThreshold)
				{
					lodIndex = i + 1;
				}
				else
				{
					break;
				}
			}

			if (lodIndex != PreviousLODindex)
			{
				FLODMesh& lodMesh = LODmeshes[lodIndex];
				if (lodMesh.bHasMesh)
				{
					PreviousLODindex = lodIndex;
					lodMesh.MeshData.CreateMesh(MeshComponent);
				}
				else if (!lodMesh.bHasRequestedMesh)
				{
					lodMesh.RequestMesh(MapData);
				}
			}
		}

		return bIsVisible;
	};

	void SetIsVisible(bool bVisible)
	{
		if (MeshComponentIsValid())
		{
			MeshComponent->SetMeshSectionVisible(MeshSectionIndex, bVisible);
			//MeshComponent->SetVisibility(bVisible);
		}
	};

	bool IsVisible() const
	{
		return MeshComponentIsValid() && MeshComponent->IsMeshSectionVisible(MeshSectionIndex);
		//return MeshComponentIsValid() && MeshComponent->IsVisible();
	};

	void OnMapDataRecieved(FMapData mapData)
	{
		bMapDataRecieved = true;
		this->MapData = mapData;
		UpdateTerrainChunk();
	}

	void OnMeshDataRevieved(FMeshData meshData)
	{
		MeshSectionIndex = meshData.MeshSectionIndex;
		meshData.CreateMesh(MeshComponent);
		UpdateTerrainChunk();
	}

private:
	FORCEINLINE bool MeshComponentIsValid() const
	{
		if (!IsValid(MeshComponent))
		{
			UE_LOG(LogTemp, Error, TEXT("Mesh actor was null on chunk at position %s!"), *Position.ToCompactString());
			return false;
		}
		return true;
	}
};