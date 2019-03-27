// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "PLG_MeshGenerator.generated.h"


struct FArray2D;
struct FProcMeshTangent;
struct FMeshData;
class UCurveFloat;

/**
 * 
 */
UCLASS()
class PROCEDURALLANDMASS_API UPLG_MeshGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FMeshData& GenerateMeshData(FArray2D& heightMap, float heightMultiplier, int32 levelOfDetail, float targetSize, const UCurveFloat* heightCurve = nullptr);
	
};

USTRUCT(BlueprintType)
struct FMeshData
{
	GENERATED_BODY()

private:
	int32 triangleIndex = 0;

public:
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;

	TArray<FVector> Normals;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	FMeshData() {};
	FMeshData(int32 meshSize)
	{
		Vertices.SetNum(meshSize * meshSize);
		Triangles.SetNum((meshSize - 1) * (meshSize - 1) * 6);
		UVs.SetNum(meshSize * meshSize);
	};

	void AddTriangle(int32 a, int32 b, int32 c)
	{
		Triangles[triangleIndex] = a;
		Triangles[triangleIndex+1] = b;
		Triangles[triangleIndex+2] = c;

		triangleIndex += 3;
	}

	/* Creates a mesh section in the given procedural mesh from this mesh data.
	 * Clears the given mesh sections before that. */
	void CreateMesh(UProceduralMeshComponent* mesh, int32 meshSectionIndex = 0)
	{
		mesh->ClearMeshSection(meshSectionIndex);
		mesh->CreateMeshSection_LinearColor(meshSectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
	}

	/* Updates the given mesh section with this mesh data. */
	void UpdateMesh(UProceduralMeshComponent* mesh, int32 meshSectionIndex = 0)
	{
		mesh->UpdateMeshSection_LinearColor(meshSectionIndex, Vertices, Normals, UVs, VertexColors, Tangents);
	}
};