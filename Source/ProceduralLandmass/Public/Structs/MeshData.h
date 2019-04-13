#pragma once
#include "CoreMinimal.h"
#include "Array2D.h"
#include "Curves/CurveFloat.h"
#include "ProceduralMeshComponent.h"
#include "MeshData.generated.h"


struct FProcMeshTangent;
struct FLinearColor;

/**
 * This struct contains the data needed to generate a mesh.
 */
USTRUCT(BlueprintType)
struct FMeshData
{
	GENERATED_BODY()

public:
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;

	TArray<FVector> Normals;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	/* The level of detail this mesh data represents. */
	int32 LOD = 0;

	/////////////////////////////////////////////////////
	FMeshData() {}
	/**
	 * Creates a mesh data struct with the given data.
	 * Safes the height map in the red vertex color channel. This version of the height map is compressed, because the vertex color is only 8 bit (Values in range 0-255).
	 * The generated mesh data is centered, so the mesh component's central location will be at the mesh's center.
	 * @param heightMap The height to generate the mesh from. When @see levelOfDetail is not zero, than this must be the height map at LOD 0.
	 */
	FMeshData(const FArray2D& heightMap, float heightMultiplier, int32 levelOfDetail, const UCurveFloat* heightCurve = nullptr)
	{
		LOD = levelOfDetail;

		const int32 meshSize = heightMap.GetWidth();
		const float topLeftX = (meshSize - 1) / -2.0f;
		const float topLeftY = (meshSize - 1) / 2.0f;

		const int32 meshSimplificationIncrement = LOD == 0 ? 1 : LOD * 2;
		const int32 verticesPerLine = (meshSize - 1) / meshSimplificationIncrement + 1;

		Vertices.SetNum(verticesPerLine * verticesPerLine);
		Triangles.SetNum((verticesPerLine - 1) * (verticesPerLine - 1) * 6);
		UVs.SetNum(verticesPerLine * verticesPerLine);
		VertexColors.SetNum(verticesPerLine * verticesPerLine);

		int32 vertexIndex = 0;
		for (int32 y = 0; y < meshSize; y += meshSimplificationIncrement)
		{
			for (int32 x = 0; x < meshSize; x += meshSimplificationIncrement)
			{
				const float height = heightMap[x + y*meshSize];
				const float curveValue = heightCurve ? heightCurve->GetFloatValue(height) : 1.0f;
				Vertices[vertexIndex] = FVector((topLeftX + x), (topLeftY - y), height * heightMultiplier * curveValue);
				UVs[vertexIndex] = FVector2D(x / (float)meshSize, y / (float)meshSize);
				
				/* Safe the height map to the red vertex color channel. */
				const FColor color = FColor((uint8)FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.0f, 255.0f), height));
				VertexColors[vertexIndex] = color;

				if (x < meshSize - 1 && y < meshSize - 1)
				{
					AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
					AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
				}

				vertexIndex++;
			}
		}
	}
	~FMeshData() {}

private:
	/////////////////////////////////////////////////////
	int32 triangleIndex = 0;

	void AddTriangle(int32 a, int32 b, int32 c)
	{
		if(!Triangles.IsValidIndex(triangleIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Triangle index '%d' invalid!"), triangleIndex);
			return;
		}

		Triangles[triangleIndex] = a;
		Triangles[triangleIndex + 1] = b;
		Triangles[triangleIndex + 2] = c;

		triangleIndex += 3;
	}

public:
	/////////////////////////////////////////////////////
	/* Creates a mesh section in the given procedural mesh from this mesh data. */
	void CreateMesh(UProceduralMeshComponent* mesh, int32 sectionIndex = 0) const
	{
		mesh->CreateMeshSection(sectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
	}

	/* Updates the given mesh section with this mesh data. */
	void UpdateMesh(UProceduralMeshComponent* mesh, int32 sectionIndex = 0) const
	{
		mesh->UpdateMeshSection(sectionIndex, Vertices, Normals, UVs, VertexColors, Tangents);
	}
};