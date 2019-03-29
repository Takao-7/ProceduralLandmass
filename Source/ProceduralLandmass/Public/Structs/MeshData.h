#pragma once
#include "CoreMinimal.h"
#include "Array2D.h"
#include "Curves/CurveFloat.h"
#include "ProceduralMeshComponent.h"
#include "MeshData.generated.h"


struct FProcMeshTangent;
struct FLinearColor;


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
	int32 MeshSectionIndex = 0;


	/////////////////////////////////////////////////////
	FMeshData() {};
	FMeshData(int32 meshSize)
	{
		Vertices.SetNum(meshSize * meshSize);
		Triangles.SetNum((meshSize - 1) * (meshSize - 1) * 6);
		UVs.SetNum(meshSize * meshSize);
	};
	~FMeshData() {};

	/////////////////////////////////////////////////////
	static FMeshData GenerateMeshData(FArray2D& heightMap, float heightMultiplier, int32 levelOfDetail, int32 meshSectionIndex, const FVector& offset = FVector::ZeroVector, const UCurveFloat* heightCurve = nullptr)
	{
		const int32 heightMapSize = heightMap.GetWidth();
		const float topLeftX = (heightMapSize - 1) / -2.0f;
		const float topLeftY = (heightMapSize - 1) / 2.0f;

		const int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
		const int32 verticesPerLine = (heightMapSize - 1) / meshSimplificationIncrement + 1;

		FMeshData meshData = FMeshData(verticesPerLine);
		meshData.MeshSectionIndex = meshSectionIndex;

		int32 vertexIndex = 0;
		for (int32 y = 0; y < heightMapSize; y += meshSimplificationIncrement)
		{
			for (int32 x = 0; x < heightMapSize; x += meshSimplificationIncrement)
			{
				const float height = heightMap[y][x];
				const float curveValue = heightCurve ? heightCurve->GetFloatValue(height) : 1.0f;
				meshData.Vertices[vertexIndex] = FVector((topLeftX + x), (topLeftY - y), height * heightMultiplier * curveValue) + offset;
				meshData.UVs[vertexIndex] = FVector2D(x / (float)heightMapSize, y / (float)heightMapSize);

				if (x < heightMapSize - 1 && y < heightMapSize - 1)
				{	
					meshData.AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
					meshData.AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
				}

				vertexIndex++;
			}
		}

		return meshData;
	};

	/////////////////////////////////////////////////////
	void AddTriangle(int32 a, int32 b, int32 c)
	{
		Triangles[triangleIndex] = a;
		Triangles[triangleIndex + 1] = b;
		Triangles[triangleIndex + 2] = c;

		triangleIndex += 3;
	};

	/////////////////////////////////////////////////////
	/* Creates a mesh section in the given procedural mesh from this mesh data.
	 * Clears the given mesh sections before that. */
	void CreateMesh(UProceduralMeshComponent* mesh)
	{
		mesh->bUseAsyncCooking = true;
		mesh->CreateMeshSection_LinearColor(MeshSectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
	};

	/* Updates the given mesh section with this mesh data. */
	void UpdateMesh(UProceduralMeshComponent* mesh)
	{
		mesh->UpdateMeshSection_LinearColor(MeshSectionIndex, Vertices, Normals, UVs, VertexColors, Tangents);
	};
};