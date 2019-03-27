// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PLG_MeshGenerator.h"
#include "PLG_MapGenerator.generated.h"


USTRUCT(BlueprintType)
struct FTerrainType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Height;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color;
};


UENUM(BlueprintType)
enum class EDrawMode : uint8
{
	NoiseMap,
	ColorMap,
	Mesh
};


class UProceduralMeshComponent;
class UMaterial;
class UCurveFloat;
class USceneComponent;
class UTexture2D;
class AStaticMeshActor;


UCLASS()
class PROCEDURALLANDMASS_API APLG_MapGenerator : public AActor
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

	/* The target map size, in cm. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 100.0f), Category = "Map Generator|General")
	float TargetMapSize = 1000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 0.001f), Category = "Map Generator|Settings")
	float NoiseScale = 0.3f;

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
	static const int32 MapChunkSize = 241;


	/////////////////////////////////////////////////////
					/* Functions */
	/////////////////////////////////////////////////////
protected:
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void DrawTexture(UTexture2D* texture, float targetSize);
	
	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void DrawMesh(FMeshData& meshData, UTexture2D* texture, UMaterial* material, UProceduralMeshComponent* mesh, float targetSize);

	UFUNCTION(BlueprintPure, Category = "Map Generator")
	bool ShouldGenerateMap() const { return bAutoUpdate || bGenerateMap; };

	UFUNCTION(BlueprintCallable, Category = "Map Generator")
	void GenerateMap(bool bCreateNewMesh = false);

private:
	void SetComponentScaleToTextureSize(USceneComponent* component, const UTexture2D* texture, int32 initialSize = 1);
	
public:	
	// Sets default values for this actor's properties
	APLG_MapGenerator();

	void SetPlaneVisibility(bool bIsVisible);

};
