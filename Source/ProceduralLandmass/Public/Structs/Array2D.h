#pragma once

#include "CoreMinimal.h"
#include "Array2D.generated.h"

USTRUCT()
struct FArray2D
{
	GENERATED_BODY()

public:
	FArray2D() {};
	FArray2D(int32 sizeX, int32 sizeY)
	{
		for (int32 i = 0; i < sizeY; i++)
		{
			TArray<float> row = TArray<float>();
			row.SetNum(sizeX);
			ArrayIntern.Add(i, row);
		}
	};


	TArray<float>& operator[] (int32 index)
	{
		return *ArrayIntern.Find(index);
	};

	float Get(int32 x, int32 y)
	{
		TArray<float>& row = *ArrayIntern.Find(y);
		return row[x];
	}

	void Set(int32 x, int32 y, float value)
	{
		TArray<float>& row = *ArrayIntern.Find(y);
		row[x] = value;
	}

private:
	TMap<int32, TArray<float>> ArrayIntern;
};