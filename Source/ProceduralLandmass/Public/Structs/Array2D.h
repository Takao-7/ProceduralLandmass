#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Array2D.generated.h"


USTRUCT(BlueprintType)
struct FArray2D
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadOnly)
	TArray<float> ArrayIntern;

	UPROPERTY(BlueprintReadOnly)
	int32 NumRows;

	UPROPERTY(BlueprintReadOnly)
	int32 NumColumns;

public:
	FArray2D() {};
	~FArray2D() {};

	/**
	 * @param sizeX Number of columns
	 * @param sizeY Number of rows
	 */
	FArray2D(int32 sizeX, int32 sizeY)
	{
		ArrayIntern = TArray<float>();
		ArrayIntern.SetNum(sizeX * sizeY);

		NumRows = sizeY;
		NumColumns = sizeX;
	};

	FArray2D(const FArray2D& otherArray)
	{
		ArrayIntern.SetNum(otherArray.NumColumns * otherArray.NumRows);
		this->NumRows = otherArray.NumRows;
		this->NumColumns = otherArray.NumColumns;

		const auto lamdba = [&](int32 x, int32 y, float& value) { value = otherArray.GetValue(x, y); };
		ForEachWithIndex(lamdba);
	}

	FORCEINLINE float operator[] (int32 index) const
	{
		return ArrayIntern[index];
	};

	FORCEINLINE float& operator[] (int32 index)
	{
		return ArrayIntern[index];
	};

	/* Returns the value at column x and row y. */
	FORCEINLINE float GetValue(int32 x, int32 y) const
	{
		return ArrayIntern[y * NumColumns + x];
	}

	/* Sets the value in cell at column x and row y. */
	FORCEINLINE void Set(int32 x, int32 y, float value)
	{
		ArrayIntern[y * NumColumns + x] = value;
	}

	FORCEINLINE int32 GetHeight() const { return NumRows; };
	FORCEINLINE int32 GetWidth() const { return NumColumns; };

	/* Loops through the entire array, row by row and calls the lambda with each value as a parameter (passed by reference). */
	void ForEach(TFunction<void (float& value)> lambda)
	{
		for (int32 y = 0; y < NumColumns; y++)
		{
			for (int32 x = 0; x < NumRows; x++)
			{
				float& value = ArrayIntern[y * NumColumns + x];
				lambda(value);
			}
		}
	}

	/* Loops through the entire array, row by row and calls the lambda with each value as the first parameter (passed by reference)
	 * and the current X and Y position as second and third parameter. */
	void ForEachWithIndex(TFunction<void (float& value, int32 xIndex, int32 yIndex)> lambda)
	{
		for (int32 y = 0; y < NumColumns; y++)
		{
			for (int32 x = 0; x < NumRows; x++)
			{
				float& value = ArrayIntern[y * NumColumns + x];
				lambda(value, x, y);
			}
		}
	}

	/* Loops through the entire array, row by row and calls the lambda with each value as the first parameter (passed by reference)
	 * and the current X and Y position as second and third parameter. */
	void ForEachWithIndex(TFunctionRef<void (int32 xIndex, int32 yIndex, float& value)> lambda)
	{
		for (int32 y = 0; y < NumColumns; y++)
		{
			for (int32 x = 0; x < NumRows; x++)
			{
				float& value = ArrayIntern[y * NumColumns + x];
				lambda(x, y, value);
			}
		}
	}
};
