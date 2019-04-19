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

	FVector2D UVOffset;

	/////////////////////////////////////////////////////
	FMeshData() {}
	/**
	 * Creates a mesh data struct with the given data.
	 * Safes the height map in the red vertex color channel. This version of the height map is compressed, because the vertex color is only 8 bit (Values in range 0-255).
	 * The generated mesh data is centered, so the mesh component's central location will be at the mesh's center.
	 * @param heightMap The height to generate the mesh from. When @see levelOfDetail is not zero, than this must be the height map at LOD 0.
	 */
	FMeshData(const FArray2D& heightMap, float heightMultiplier, int32 levelOfDetail, const UCurveFloat* heightCurve = nullptr, FVector2D uvOffset = FVector2D::ZeroVector)
		: LOD(levelOfDetail), UVOffset(uvOffset)
	{
		const int32 meshSize = heightMap.GetWidth();
		const int32 meshSimplificationIncrement = LOD == 0 ? 1 : LOD * 2;
		const int32 verticesPerLine = (meshSize - 1) / meshSimplificationIncrement + 1;
		const int32 numVertices = FMath::Pow(verticesPerLine, 2);

		Vertices.SetNum(numVertices);
		Triangles.SetNum((verticesPerLine - 1) * (verticesPerLine - 1) * 6);
		UVs.SetNum(numVertices);
		VertexColors.SetNum(numVertices);

		int32 vertexIndex = 0;
		for (int32 y = 0; y < meshSize; y += meshSimplificationIncrement)
		{
			for (int32 x = 0; x < meshSize; x += meshSimplificationIncrement)
			{
				SetVertexAndUV(x, y, vertexIndex, heightMap, heightMultiplier, heightCurve);

				if (x < meshSize - 1 && y < meshSize - 1)
				{
					AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
					AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
				}

				++vertexIndex;
			}
		}
	}

	~FMeshData() {}

	void SetVertexAndUV(int32 x, int32 y, int32 vertexIndex, const FArray2D& heightMap, float heightMultiplier, const UCurveFloat* heightCurve = nullptr)
	{
		const int32 meshSize = heightMap.GetWidth();
		const float topLeftX = (meshSize - 1) / -2.0f;
		const float topLeftY = (meshSize - 1) / 2.0f;

		const float height = heightMap[x + y * meshSize];
		const float curveValue = heightCurve ? heightCurve->GetFloatValue(height) : 1.0f;
		const float totalHeight = height * heightMultiplier * curveValue;
		Vertices[vertexIndex] = FVector((topLeftX + x), (topLeftY - y), totalHeight);

		UVs[vertexIndex] = FVector2D(x + UVOffset.X, y + UVOffset.Y) / (float)meshSize;

		/* Safe the height map to the red vertex color channel. */
		float mappedHeight = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.0f, 255.0f), height * curveValue);
		VertexColors[vertexIndex] = FColor(FMath::RoundToInt(mappedHeight), 0, 0);
	}

	void UpdateMeshData(const FArray2D& heightMap, float heightMultiplier, const UCurveFloat* heightCurve = nullptr)
	{
		const int32 meshSize = heightMap.GetWidth();
		const int32 meshSimplificationIncrement = LOD == 0 ? 1 : LOD * 2;

		int32 vertexIndex = 0;
		for (int32 y = 0; y < meshSize; y += meshSimplificationIncrement)
		{
			for (int32 x = 0; x < meshSize; x += meshSimplificationIncrement)
			{
				SetVertexAndUV(x, y, vertexIndex, heightMap, heightMultiplier, heightCurve);
				++vertexIndex;
			}
		}
	}

private:
	int32 triangleIndex = 0;

	void AddTriangle(int32 a, int32 b, int32 c)
	{
		if(!Triangles.IsValidIndex(triangleIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Triangle index '%d' invalid! LOD: %d"), triangleIndex, LOD);
			return;
		}

		Triangles[triangleIndex] = a;
		Triangles[triangleIndex + 1] = b;
		Triangles[triangleIndex + 2] = c;

		triangleIndex += 3;
	}
};