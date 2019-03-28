#pragma once
#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "TerrainChunk.generated.h"


USTRUCT(BlueprintType)
struct FTerrainChunk
{
	GENERATED_BODY()

public:
	/* This chunk's world position, measured from it's center point. */
	UPROPERTY(BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;

protected:
	UPROPERTY(BlueprintReadWrite)
	AStaticMeshActor* MeshActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	float MaxViewDistance = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<AStaticMeshActor> PlaneClass = nullptr;

	UPROPERTY(BlueprintReadWrite)
	UWorld* World = nullptr;

	UPROPERTY(BlueprintReadWrite)
	float Size = 0.0f;


	/////////////////////////////////////////////////////
public:
	FTerrainChunk() {};

	/**
	 * @param coordination The chunks coordination in relation to the terrain grid. The chunk at the center would be at 0:0.
	 */
	FTerrainChunk(const FVector2D& coordination, float viewDistance, int32 size, AActor* parent, TSubclassOf<AStaticMeshActor> planeClass, float zPosition = 0.0f)
	{
		Position = FVector(coordination * size, zPosition);
		MaxViewDistance = viewDistance;
		World = parent->GetWorld();
		PlaneClass = planeClass;
		Size = size;

		if (PlaneClass && World)
		{
			const FTransform spawnTransform = FTransform(FRotator::ZeroRotator, Position, FVector(size) / 100.0f);
			FActorSpawnParameters spawnParams;
			spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			spawnParams.Owner = parent;
			MeshActor = World->SpawnActor<AStaticMeshActor>(PlaneClass, spawnTransform, spawnParams);
			MeshActor->AttachToActor(parent, FAttachmentTransformRules::KeepWorldTransform);
		}

		SetIsVisible(false);
	};

	void UpdateTerrainChunk(FVector viewerPosition)
	{
		const float viewerDistanceFromCenter = (FVector2D(Position) - FVector2D(viewerPosition)).Size() - Size;
		const bool bIsVisible = viewerDistanceFromCenter <= MaxViewDistance;
		SetIsVisible(bIsVisible);
	};

	void SetIsVisible(bool bVisible)
	{
		if (MeshComponentIsValid())
		{
			MeshActor->GetStaticMeshComponent()->SetVisibility(bVisible);
		}
	};

	bool IsVisible() const
	{
		return MeshComponentIsValid() && MeshActor->GetStaticMeshComponent()->IsVisible();
	};

private:
	bool MeshComponentIsValid() const
	{
		if (!IsValid(MeshActor) || !IsValid(MeshActor->GetStaticMeshComponent()))
		{
			UE_LOG(LogTemp, Error, TEXT("Mesh actor was null on chunk at position %s!"), *Position.ToCompactString());
			return false;
		}
		return true;
	}
};