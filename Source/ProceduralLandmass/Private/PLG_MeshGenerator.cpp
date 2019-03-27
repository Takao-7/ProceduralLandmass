// Fill out your copyright notice in the Description page of Project Settings.

#include "PLG_MeshGenerator.h"
#include "Array2D.h"
#include "Curves/CurveFloat.h"


FMeshData& UPLG_MeshGenerator::GenerateMeshData(FArray2D& heightMap, float heightMultiplier, int32 levelOfDetail, float targetSize, const UCurveFloat* heightCurve /*= nullptr*/)
{
	const int32 heightMapSize = heightMap.GetWidth();
	const float topLeftX = (heightMapSize - 1) / -2.0f;
	const float topLeftY = (heightMapSize - 1) / 2.0f;

	const int32 meshSimplificationIncrement = levelOfDetail == 0 ? 1 : levelOfDetail * 2;
	const int32 verticesPerLine = (heightMapSize - 1) / meshSimplificationIncrement + 1;
	const float scaleFactor = targetSize / (float)verticesPerLine;

	FMeshData* meshData = new FMeshData(verticesPerLine);

	int32 vertexIndex = 0;
	for (int32 y = 0; y < heightMapSize; y += meshSimplificationIncrement)
	{
		for (int32 x = 0; x < heightMapSize; x += meshSimplificationIncrement)
		{
			const float height = heightMap[y][x];
			const float curveValue = heightCurve ? heightCurve->GetFloatValue(height) : 1.0f;
			meshData->Vertices[vertexIndex] = FVector((topLeftX + x), (topLeftY - y), height * heightMultiplier * curveValue);
			meshData->UVs[vertexIndex] = FVector2D(x / (float)heightMapSize, y / (float)heightMapSize);

			if (x < heightMapSize - 1 && y < heightMapSize - 1)
			{
				meshData->AddTriangle(vertexIndex, vertexIndex + verticesPerLine + 1, vertexIndex + verticesPerLine);
				meshData->AddTriangle(vertexIndex + verticesPerLine + 1, vertexIndex, vertexIndex + 1);
			}

			vertexIndex++;
		}
	}

	return *meshData;
}
