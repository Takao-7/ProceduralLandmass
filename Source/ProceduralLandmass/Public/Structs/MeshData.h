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

	TArray<FVector> BorderVertices;

	/* Array for border triangle indices.
	 * A negative index means that the vertex is NOT 
	 * a border vertex and therefore found in the Vertices array 
	 * with (index + 1) * -1. */
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
	FMeshData(const FArray2D& heightMap, float heightMultiplier, int32 levelOfDetail, const TArray<float>* borderHeightMap, const UCurveFloat* heightCurve = nullptr, float mapScale = 100.0f)
		: LOD(levelOfDetail), MapScale(mapScale)
	{
		int32 triangleIndex = 0;
		auto AddTriangle = [](int32 a, int32 b, int32 c, TArray<int32>& triangles, int32& index) -> void
		{
			triangles[index] = a;
			triangles[index + 1] = b;
			triangles[index + 2] = c;

			index += 3;
		};

		const int32 meshSize = heightMap.GetWidth();
		const int32 meshSimplificationIncrement = LOD == 0 ? 1 : LOD * 2;
		const int32 verticesPerLine = (meshSize - 1) / meshSimplificationIncrement + 1;
		const int32 numVertices = FMath::Pow(verticesPerLine, 2);

		Vertices.SetNum(numVertices);
		Normals.SetNum(numVertices);
		Tangents.SetNum(numVertices);
		Triangles.SetNum((verticesPerLine - 1) * (verticesPerLine - 1) * 6);
		UVs.SetNum(numVertices);
		VertexColors.SetNum(numVertices);

		const int32 numBorderVertices = (verticesPerLine * 4 + 4);
		BorderVertices.SetNum(numBorderVertices);
		BorderTriangles.SetNum(((verticesPerLine - 1) * 4 + 6) * 3);

		int32 vertexIndex = 0;
		for (int32 y = 0; y < meshSize; y += meshSimplificationIncrement)
		{
			#if PROFILE_CPU
			SCOPE_CYCLE_COUNTER(STAT_GenerateTriangles);
			#endif // PROFILE_CPU

			for (int32 x = 0; x < meshSize; x += meshSimplificationIncrement)
			{
				SetVertexAndUV(x, y, vertexIndex, heightMap, heightMultiplier, heightCurve);

				/* We need to initialize the normals array, because all normal vectors has to be exactly 0,
				 * when we start to calculate them. */
				Normals[vertexIndex] = FVector::ZeroVector;	

				if (x < meshSize - 1 && y < meshSize - 1)
				{
					const int32 a = vertexIndex;
					const int32 b = vertexIndex + 1;
					const int32 c = vertexIndex + verticesPerLine;
					const int32 d = vertexIndex + verticesPerLine + 1;
					AddTriangle(a, d, c, Triangles, triangleIndex);
					AddTriangle(d, a, b, Triangles, triangleIndex);
				}

				++vertexIndex;
			}
		}

		const int32 totalMeshSize = heightMap.GetWidth() + 2 * meshSimplificationIncrement;

		auto setBorderVertex = [&](int32 x, int32 y, int32 index) -> void
		{
			const float topLeftX = (totalMeshSize - 1) / -2.0f;
			const float topLeftY = (totalMeshSize - 1) / 2.0f;

			const float height = (*borderHeightMap)[index];
			const float curveValue = heightCurve ? heightCurve->GetFloatValue(height) : 1.0f;
			const float totalHeight = height * heightMultiplier * curveValue;
			BorderVertices[index] = FVector((topLeftX + x), (topLeftY - y), totalHeight);
		};

		vertexIndex = 0;

		/* Top row */
		for (int32 x = 0; x < meshSize + (meshSimplificationIncrement * 2); x += meshSimplificationIncrement)
		{
			setBorderVertex(x, 0, vertexIndex);
			++vertexIndex;
		}

		/* Sides */
		for (int32 y = meshSimplificationIncrement; y < meshSize; y += meshSimplificationIncrement)
		{
			setBorderVertex(0, y, vertexIndex);
			setBorderVertex(totalMeshSize, y, vertexIndex + 1);
			vertexIndex += 2;
		}

		/* Bottom row*/
		for (int32 x = 0; x < meshSize + meshSimplificationIncrement * 2; x += meshSimplificationIncrement)
		{
			setBorderVertex(x, totalMeshSize, vertexIndex);
			++vertexIndex;
		}

		/* Calculate triangles */
		int32 borderTriangleIndex = 0;
		const int32 borderWidth = verticesPerLine + 2;
		/* Top left */
		{
			const int32 a = 0;
			const int32 b = 1;
			const int32 c = verticesPerLine;
			const int32 d = -1;

			AddTriangle(a, d, c, BorderTriangles, borderTriangleIndex);
			AddTriangle(d, a, b, BorderTriangles, borderTriangleIndex);
		}
		/* Top row */
		for (int32 x = 0; x < verticesPerLine - 1; x++)
		{
			const int32 a = x;
			const int32 b = x + 1;
			const int32 c = (x * -1) - 1;
			const int32 d = (x * -1) - 2;

			AddTriangle(a, d, c, BorderTriangles, borderTriangleIndex);
			AddTriangle(d, a, b, BorderTriangles, borderTriangleIndex);
		}
		/* Top right */
		{
			const int32 a = borderWidth - 2;
			const int32 b = borderWidth - 1;
			const int32 c = verticesPerLine * -1;
			const int32 d = borderWidth + 1;

			AddTriangle(a, d, c, BorderTriangles, borderTriangleIndex);
		}
		/* Sides */
		for (int32 y = 0; y < verticesPerLine - 2; y++)
		{
			/* Left side */
			int32 a = borderWidth + (y * 2);
			int32 b = ((y * verticesPerLine) - 1) * -1;
			int32 c = borderWidth + ((y + 1) * 2);
			int32 d = (((y + 1) * verticesPerLine) - 1) * -1;

			AddTriangle(a, d, c, BorderTriangles, borderTriangleIndex);
			AddTriangle(d, a, b, BorderTriangles, borderTriangleIndex);

			/* Right side */
			a = verticesPerLine - 1 + (y * verticesPerLine); a = (a - 1) * -1;
			b = borderWidth + 1 + y * 2;
			c = (verticesPerLine * 2) - 1 + y * verticesPerLine; c = (c - 1) * -1;
			d = borderWidth + 3 + y * 2;

			AddTriangle(a, d, c, BorderTriangles, borderTriangleIndex);
			AddTriangle(d, a, b, BorderTriangles, borderTriangleIndex);
		}
		/* Bottom left */
		{
			const int32 a = borderWidth + (verticesPerLine - 1) * 2;
			const int32 b = Vertices.Num() - verticesPerLine;
			const int32 d = a + 3;

			AddTriangle(a, b, d, BorderTriangles, borderTriangleIndex);
		}
		/* Bottom row */
		for (int32 x = 0; x < verticesPerLine - 1; x++)
		{
			int32 a = (Vertices.Num() - verticesPerLine) + x; a = (a - 1) * -1;
			int32 b = a - 1;
			const int32 c = x + borderWidth - 1 + (borderWidth - 1) * 2;
			const int32 d = c + 1;

			AddTriangle(a, d, c, BorderTriangles, borderTriangleIndex);
			AddTriangle(d, a, b, BorderTriangles, borderTriangleIndex);
		}
		/* Bottom right */
		{
			const int32 a = (Vertices.Num() - 2) * -1;
			const int32 b = BorderVertices.Num() - 1 - borderWidth;
			const int32 c = BorderVertices.Num() - 2;
			const int32 d = BorderVertices.Num() - 1;

			AddTriangle(a, d, c, BorderTriangles, borderTriangleIndex);
			AddTriangle(d, a, b, BorderTriangles, borderTriangleIndex);
		}

		/* Calculate border normals */
		{
			auto GetVertex = [&](int32 index) -> FVector
			{
				if (index < 0)
				{
					return Vertices[(index + 1) * -1];
				}
				else
				{
					return BorderVertices[index];
				}
			};

			const int32 triangleCount = BorderTriangles.Num() / 3;
			for (int32 i = 0; i < triangleCount; i++)
			{
				const int32 normalTriangleIndex = i * 3;
				int32 vertexIndexA = BorderTriangles[normalTriangleIndex];
				int32 vertexIndexB = BorderTriangles[normalTriangleIndex + 1];
				int32 vertexIndexC = BorderTriangles[normalTriangleIndex + 2];

				/* Calculate triangle normal */
				const FVector pointA = GetVertex(vertexIndexA);
				const FVector pointB = GetVertex(vertexIndexB);
				const FVector pointC = GetVertex(vertexIndexC);

				const FVector sideAC = pointA - pointC;
				const FVector sideAB = pointA - pointB;

				FVector triangleNormal = FVector::CrossProduct(sideAC, sideAB);
				triangleNormal.Normalize();

				if (vertexIndexA < 0)
				{
					vertexIndexA = (vertexIndexA + 1) * -1;
					Normals[vertexIndexA] += triangleNormal;
				}
				if (vertexIndexB < 0)
				{
					vertexIndexB = (vertexIndexB + 1) * -1;
					Normals[vertexIndexB] += triangleNormal;
				}
				if (vertexIndexC < 0)
				{
					vertexIndexC = (vertexIndexC + 1) * -1;
					Normals[vertexIndexC] += triangleNormal;
				}
			}
		}

		/* Calculate normals. */
		{
			#if PROFILE_CPU
			SCOPE_CYCLE_COUNTER(STAT_GenerateNormals);
			#endif // PROFILE_CPU

			const int32 triangleCount = Triangles.Num() / 3;
			for (int32 i = 0; i < triangleCount; i++)
			{
				const int32 normalTriangleIndex = i * 3;
				const int32 vertexIndexA = Triangles[normalTriangleIndex];
				const int32 vertexIndexB = Triangles[normalTriangleIndex + 1];
				const int32 vertexIndexC = Triangles[normalTriangleIndex + 2];

				/* Calculate triangle normal */
				const FVector pointA = Vertices[vertexIndexA];
				const FVector pointB = Vertices[vertexIndexB];
				const FVector pointC = Vertices[vertexIndexC];

				const FVector sideAC = pointA - pointC;
				const FVector sideAB = pointA - pointB;

				FVector triangleNormal = FVector::CrossProduct(sideAC, sideAB);
				triangleNormal.Normalize();

				// We add the normals together, because a vertex normal is the average
				// normal across it's connected faces.
				Normals[vertexIndexA] += triangleNormal;
				Normals[vertexIndexB] += triangleNormal;
				Normals[vertexIndexC] += triangleNormal;
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

	void SetVertexAndUV(int32 x, int32 y, int32 vertexIndex, const FArray2D& heightMap, float heightMultiplier, const UCurveFloat* heightCurve = nullptr)
	{
		const int32 meshSize = heightMap.GetWidth();
		const float topLeftX = (meshSize - 1) / -2.0f;
		const float topLeftY = (meshSize - 1) / 2.0f;

		const float height = heightMap[x + y * meshSize];
		const float curveValue = heightCurve ? heightCurve->GetFloatValue(height) : 1.0f;
		const float totalHeight = height * heightMultiplier * curveValue;
		
		Vertices[vertexIndex] = FVector((topLeftX + x), (topLeftY - y), totalHeight);
		UVs[vertexIndex] = (FVector2D(topLeftX + x, topLeftY - y) * MapScale) / (float)meshSize;

		/* Safe the height map to the red vertex color channel. */
		float mappedHeight = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.0f, 255.0f), height * curveValue);
		VertexColors[vertexIndex] = FColor(FMath::RoundToInt(mappedHeight), 0, 0);
	}

	void UpdateMeshData(const FArray2D& heightMap, float heightMultiplier, const UCurveFloat* heightCurve = nullptr)
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
				SetVertexAndUV(x, y, vertexIndex, heightMap, heightMultiplier, heightCurve);
				++vertexIndex;
			}
		}
	}
};