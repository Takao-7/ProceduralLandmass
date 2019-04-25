#pragma once
#include "CoreMinimal.h"
#include "Array2D.h"
#include "Curves/CurveFloat.h"
#include "ProceduralMeshComponent.h"
#include <Kismet/KismetSystemLibrary.h>
#include <KismetProceduralMeshLibrary.h>
#include "MeshData.generated.h"


struct FProcMeshTangent;
struct FLinearColor;


#define PROFILE_CPU 0

#if PROFILE_CPU
	DECLARE_STATS_GROUP(TEXT("MeshData"), STATGROUP_MeshData, STATCAT_Advanced);
	
	DECLARE_CYCLE_STAT(TEXT("GenerateTriangles"), STAT_GenerateTriangles, STATGROUP_MeshData);
	DECLARE_CYCLE_STAT(TEXT("CalculateNormals"), STAT_CalculateNormals, STATGROUP_MeshData);
	DECLARE_CYCLE_STAT(TEXT("UpdateMeshData"), STAT_UpdateMeshData, STATGROUP_MeshData);
#endif // PROFILE_CPU


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

	TArray<int32> VerticesIndexMap;

	/* Border vertices are counted from -1 downwards. 
	 * Beginning at the top left. */
	TArray<FVector> BorderVertices;
	TArray<int32> BorderTriangles;

	/* The level of detail this mesh data represents. */
	int32 LOD = 0;

	/* The terrain scale. Needed for setting the UV's in a way that compensates the terrain scale, so that materials
	 * are not stretched. */
	float MapScale = 100.0f;

	/////////////////////////////////////////////////////
	FMeshData() {}
	/**
	 * Creates a mesh data struct with the given data.
	 * Safes the height map in the red vertex color channel. This version of the height map is compressed, because the vertex color is only 8 bit (Values in range 0-255).
	 * The generated mesh data is centered, so the mesh component's central location will be at the mesh's center.
	 * @param heightMap The height to generate the mesh from. When @see levelOfDetail is not zero, than this must be the height map at LOD 0.
	 * @param borderHeightMap If provided, these height values form a ring around the height map, containing the height values for the vertices that would be at a 
	 * distance of LOD * 2.
	 */
	FMeshData(const FArray2D& heightMap, float heightMultiplier, int32 levelOfDetail, const TArray<float>& borderHeightMap, const UCurveFloat* heightCurve = nullptr, float mapScale = 100.0f)
		: LOD(levelOfDetail), MapScale(mapScale)
	{
		int32 triangleIndex = 0;
		int32 borderTriangleIndex = 0;
		auto AddTriangle = [&](int32 a, int32 b, int32 c) -> void
		{
			if (a >= 0 && b >= 0 && c >= 0)
			{
				Triangles[triangleIndex] = a;
				Triangles[triangleIndex + 1] = b;
				Triangles[triangleIndex + 2] = c;
				triangleIndex += 3;
			}
			else
			{
				BorderTriangles[borderTriangleIndex] = a;
				BorderTriangles[borderTriangleIndex + 1] = b;
				BorderTriangles[borderTriangleIndex + 2] = c;
				borderTriangleIndex += 3;
			}
		};

		const int32 meshSize = heightMap.GetWidth();
		const int32 meshSimplificationIncrement = LOD == 0 ? 1 : LOD * 2;
		const int32 verticesPerLine = (meshSize - 1) / meshSimplificationIncrement + 1;
		const int32 borderedSize = verticesPerLine + 2;
		const int32 numVertices = FMath::Pow(verticesPerLine, 2);
		const int32 numBorderVertices = (verticesPerLine * 4 + 4);

		Vertices.SetNum(numVertices);
		Normals.SetNum(numVertices);
		Tangents.SetNum(numVertices);
		Triangles.SetNum((verticesPerLine - 1) * (verticesPerLine - 1) * 6);
		UVs.SetNum(numVertices);
		VertexColors.SetNum(numVertices);
		VerticesIndexMap.SetNum(borderedSize * borderedSize);
		BorderVertices.SetNum(numBorderVertices);
		BorderTriangles.SetNum(24 * borderedSize);
		
		int32 meshVertexIndex = 0;
		int32 borderVertexIndex = -1;
		for (int32 y = 0; y < borderedSize; ++y)
		{
			for (int32 x = 0; x < borderedSize; x++)
			{
				const bool bIsBorderVertex = y == 0 || y == borderedSize - 1 || x == 0 || x == borderedSize - 1;
				if (bIsBorderVertex)
				{
					VerticesIndexMap[x + y * borderedSize] = borderVertexIndex;
					borderVertexIndex--;
				}
				else
				{
					VerticesIndexMap[x + y * borderedSize] = meshVertexIndex;
					meshVertexIndex++;
				}
			}
		}

		for (int32 y = 0; y < borderedSize; ++y)
		{
			#if PROFILE_CPU
			SCOPE_CYCLE_COUNTER(STAT_GenerateTriangles);
			#endif // PROFILE_CPU

			for (int32 x = 0; x < borderedSize; ++x)
			{
				const int32 vertexIndex = VerticesIndexMap[x + y * borderedSize];
				SetVertexAndUV(x, y, vertexIndex, heightMap, heightMultiplier, borderHeightMap, heightCurve);

				if (vertexIndex >= 0)
				{
					/* We need to initialize the normals array, because all normal vectors has to be exactly 0,
					 * when we start to calculate them. */
					Normals[vertexIndex] = FVector::ZeroVector;
				}

				if (x < borderedSize - 1 && y < borderedSize - 1)
				{
					const int32 a = VerticesIndexMap[x + y * borderedSize];
					const int32 b = VerticesIndexMap[(x + 1) + (y * borderedSize)];
					const int32 c = VerticesIndexMap[x + ((y + 1) * borderedSize)];
					const int32 d = VerticesIndexMap[(x + 1) + ((y + 1) * borderedSize)];
					AddTriangle(a, c, d);
					AddTriangle(a, d, b);
				}
			}
		}
		
		/* Calculate normals. */
		{
			#if PROFILE_CPU
			SCOPE_CYCLE_COUNTER(STAT_GenerateNormals);
			#endif // PROFILE_CPU

			auto NormalFromVertices = [&](int32 indexA, int32 indexB, int32 indexC) -> FVector
			{
				const FVector pointA = indexA >= 0 ? Vertices[indexA] : BorderVertices[-indexA - 1];
				const FVector pointB = indexB >= 0 ? Vertices[indexB] : BorderVertices[-indexB - 1];
				const FVector pointC = indexC >= 0 ? Vertices[indexC] : BorderVertices[-indexC - 1];

				const FVector sideAC = pointC - pointA;
				const FVector sideAB = pointB - pointA;

				FVector triangleNormal = FVector::CrossProduct(sideAC, sideAB);
				triangleNormal.Normalize();
				return triangleNormal;
			};

			const int32 triangleCount = Triangles.Num() / 3;
			for (int32 i = 0; i < triangleCount; i++)
			{
				const int32 normalTriangleIndex = i * 3;
				const int32 vertexIndexA = Triangles[normalTriangleIndex];
				const int32 vertexIndexB = Triangles[normalTriangleIndex + 1];
				const int32 vertexIndexC = Triangles[normalTriangleIndex + 2];

				FVector triangleNormal = NormalFromVertices(vertexIndexA, vertexIndexB, vertexIndexC);

				// We add the normals together, because a vertex normal is the average
				// normal across it's connected faces.
				Normals[vertexIndexA] += triangleNormal;
				Normals[vertexIndexB] += triangleNormal;
				Normals[vertexIndexC] += triangleNormal;
			}

			const int32 borderTrianglesCount = BorderTriangles.Num() / 3;
			for (int32 i = 0; i < borderTrianglesCount; i++)
			{
				const int32 normalTriangleIndex = i * 3;
				const int32 vertexIndexA = BorderTriangles[normalTriangleIndex];
				const int32 vertexIndexB = BorderTriangles[normalTriangleIndex + 1];
				const int32 vertexIndexC = BorderTriangles[normalTriangleIndex + 2];

				FVector triangleNormal = NormalFromVertices(vertexIndexA, vertexIndexB, vertexIndexC);

				// We add the normals together, because a vertex normal is the average
				// normal across it's connected faces.
				if (vertexIndexA >= 0)
				{
					Normals[vertexIndexA] += triangleNormal;
				}
				if (vertexIndexB >= 0)
				{
					Normals[vertexIndexB] += triangleNormal;
				}
				if (vertexIndexC >= 0)
				{
					Normals[vertexIndexC] += triangleNormal;
				}
			}

			for (int32 i = 0; i < numVertices; i++)
			{
				Normals[i].Normalize();

				const bool bFlipBitangent = Normals[i].Z < 0.0f;
				Tangents[i] = FProcMeshTangent(Normals[i], bFlipBitangent);
			}
		}
	}

	~FMeshData() {}

	void SetVertexAndUV(int32 x, int32 y, int32 vertexIndex, const FArray2D& heightMap, float heightMultiplier, const TArray<float>& borderHeightMap, const UCurveFloat* heightCurve = nullptr)
	{
		const int32 meshSize = heightMap.GetWidth();
		const float topLeftX = (meshSize - 1) / -2.0f;
		const float topLeftY = (meshSize - 1) / -2.0f;

		const int32 meshSimplificationIncrement = LOD == 0 ? 1 : LOD * 2;
		const int32 xPos = (x - 1) * meshSimplificationIncrement;
		const int32 yPos = (y - 1) * meshSimplificationIncrement;

		float height = 0;
		if (vertexIndex >= 0)
		{
			height = heightMap.GetValue(xPos, yPos);
		}
		else
		{
			height = borderHeightMap[-vertexIndex - 1];
		}

		const float curveValue = heightCurve ? heightCurve->GetFloatValue(height) : 1.0f;
		const float totalHeight = height * heightMultiplier * curveValue;
		
		const FVector vertexPosition = FVector((topLeftX + xPos), (topLeftY + yPos), totalHeight);

		if(vertexIndex >= 0)
		{
			Vertices[vertexIndex] = vertexPosition;
			UVs[vertexIndex] = (FVector2D(topLeftX + xPos, topLeftY + yPos) * MapScale) / (float)(meshSize);

			/* Safe the height map to the red vertex color channel. */
			float mappedHeight = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.0f, 255.0f), height * curveValue);
			VertexColors[vertexIndex] = FColor(FMath::RoundToInt(mappedHeight), 0, 0);
		}
		else
		{
			BorderVertices[-vertexIndex-1] = vertexPosition;
		}
	}

	void UpdateMeshData(const FArray2D& heightMap, float heightMultiplier, const TArray<float>& borderHeightMap, const UCurveFloat* heightCurve = nullptr)
	{
		#if PROFILE_CPU
		SCOPE_CYCLE_COUNTER(STAT_UpdateMeshData);
		#endif // PROFILE_CPU

		const int32 meshSize = heightMap.GetWidth();
		const int32 meshSimplificationIncrement = LOD == 0 ? 1 : LOD * 2;

		int32 vertexIndex = 0;
		for (int32 y = 0; y < meshSize; y += meshSimplificationIncrement)
		{
			for (int32 x = 0; x < meshSize; x += meshSimplificationIncrement)
			{
				SetVertexAndUV(x, y, vertexIndex, heightMap, heightMultiplier, borderHeightMap, heightCurve);
				++vertexIndex;
			}
		}
	}
};