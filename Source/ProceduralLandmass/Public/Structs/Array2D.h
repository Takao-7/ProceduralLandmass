#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Array2D.generated.h"


USTRUCT(BlueprintType)
struct FArray2D
{
	GENERATED_BODY()

public:
	FArray2D() {};
	~FArray2D() {};

	/**
	 * @param sizeX Number of columns
	 * @param sizeY Number of rows
	 */
	FArray2D(int32 sizeX, int32 sizeY)
	{
		ArrayIntern = TArray<TArray<float>>();
		ArrayIntern.SetNum(sizeY);

		for (int32 y = 0; y < sizeY; ++y)
		{
			TArray<float> row = TArray<float>();
			row.SetNum(sizeX);
			ArrayIntern[y] = row;
		}

		NumRows = sizeY;
		NumColumns = sizeX;
	};

	FORCEINLINE TArray<float>& operator[] (int32 row)
	{
		return ArrayIntern[row];
	};

	FORCEINLINE const TArray<float>& operator[] (int32 row) const
	{
		return ArrayIntern[row];
	};

	/* Returns the value at row y and column x. */
	FORCEINLINE float Get(int32 x, int32 y) const
	{
		return ArrayIntern[y][x];
	}

	/* Set value to row y and column x. */
	void Set(int32 x, int32 y, float value)
	{
		ArrayIntern[y][x] = value;
	}

	FORCEINLINE int32 GetHeight() const { return NumRows; };
	FORCEINLINE int32 GetWidth() const { return NumColumns; };

	/* Loops through the entire array, row by row and calls the lambda with each value as a parameter (passed by reference). 
	 * The lambda should be from type: (float&)->void */
	template <typename Lambda>
	void ForEach(Lambda lambda)
	{
		for (int32 y = 0; y < NumColumns; y++)
		{
			for (int32 x = 0; x < NumRows; x++)
			{
				lambda(ArrayIntern[y][x]);
			}
		}
	}

	/* Loops through the entire array, row by row and calls the lambda with each value as the first parameter (passed by reference)
	 * and the current X and Y position as second and third parameter.
	 * The lambda should be from type: (float&, int32, int32)->void */
	template <typename Lambda>
	void ForEachWithIndex(Lambda lambda)
	{
		for (int32 y = 0; y < NumColumns; y++)
		{
			for (int32 x = 0; x < NumRows; x++)
			{
				lambda(ArrayIntern[y][x], x, y);
			}
		}
	}

private:
	TArray<TArray<float>> ArrayIntern;
	int32 NumRows;
	int32 NumColumns;
};


UCLASS(meta = (DisplayName = "Array2D Functions"))
class PROCEDURALLANDMASS_API UArray2DFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


public:
	UFUNCTION(BlueprintCallable)
	static float Get(const FArray2D& array, int32 column, int32 row)
	{
		return array.Get(column, row);
	}

	UFUNCTION(BlueprintCallable)
	static void Set(UPARAM(ref) FArray2D& array, int32 column, int32 row, float value)
	{
		array.Set(column, row, value);
	}

	UFUNCTION(BlueprintCallable)
	static TArray<float>& GetRow(UPARAM(ref) FArray2D& array, int32 row)
	{
		return array[row];
	}
};