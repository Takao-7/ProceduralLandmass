// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AsyncWork.h"
#include "UnityLibrary.generated.h"


class UTexture2D;
struct FArray2D;


/**
 * Library for functions that are available in Unity, but not in Unreal.
 */
UCLASS()
class PROCEDURALLANDMASS_API UUnityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


public:
	/////////////////////////////////////////////////////
				/* Noise functions */
	/////////////////////////////////////////////////////
	/* Implementation of 2D Perlin noise based on Ken Perlin's original version (http://mrl.nyu.edu/~perlin/doc/oscar.html)
	 * (See Random1.tps for additional third party software info.)
	 * @return A value between -1 and 1. */
	static float PerlinNoise(float x, float y);

	UFUNCTION(BlueprintCallable, Category = "Unity Library|Noise")
	static float PerlinNoise(const FVector2D& vec);


	/////////////////////////////////////////////////////
					/* Falloff Generator */
	/////////////////////////////////////////////////////
	static FArray2D* GenerateFalloffMap(int32 size);

	/**
	 * @param x The x-coordinate on the map
	 * @param y The y-coordinate on the map
	 * @param size The falloff map size.
	 */
	static float GetValueWithFalloff(int32 x, int32 y, int32 size);


	/////////////////////////////////////////////////////
					/* Texture */
	/////////////////////////////////////////////////////
	/**
	 * Replaces the texture data of the given texture with new data.
	 * @param texture The texture to work on. Must not be null.
	 * @param newData The new data. This has to be an array containing the pixel (=color) data.
	 * If it's not the same size or color type than the given texture, then the result will not look o.k.
	 * @param mipToUpdate The mip from the texture to update.
	 */
	UFUNCTION(BlueprintCallable, Category = "Unity Library|Texture")
	static void ReplaceTextureData(UTexture2D* texture, const TArray<FLinearColor>& newData, int32 mipToUpdate = 0);
	static void ReplaceTextureData(UTexture2D* texture, const void* newData, int32 mipToUpdate = 0);

	UFUNCTION(BlueprintCallable, Category = "Unity Library|Texture")
	static UTexture2D* TextureFromColorMap(const TArray<FLinearColor>& colorMap);

	UFUNCTION(BlueprintCallable, Category = "Unity Library|Texture")
	UTexture2D* TextureFromHeightMap(const TArray<float>& heightMap);
	UTexture2D* TextureFromHeightMap(const FArray2D& heightMap);

	/////////////////////////////////////////////////////
					/* Multi-threading */
	/////////////////////////////////////////////////////
	/**
	 * Class for async tasks.
	 * The constructors take in callbacks. This can be TFunctions or normal C++ lambdas.
	 */
	class MyAsyncTask : public FNonAbandonableTask
	{
		friend class FAsyncTask<MyAsyncTask>;

		TFunction<void (void)> WorkCallback = nullptr;
		TFunction<bool (void)> CanAbandonCallback = nullptr;
		TFunction<void (void)> AbandonCallback = nullptr;

	public:
		MyAsyncTask() {};
		MyAsyncTask(TFunction<void(void)> workCallback)
		{
			WorkCallback = workCallback;
		};
		MyAsyncTask(TFunction<void (void)> workCallback, TFunction<void (void)> abandonCallback)
		{
			WorkCallback = workCallback;
			AbandonCallback = abandonCallback;
		};
		MyAsyncTask(TFunction<void (void)> workCallback, TFunction<void (void)> abandonCallback, TFunction<bool (void)> canAbandonCallback)
		{
			WorkCallback = workCallback;
			AbandonCallback = abandonCallback;
			CanAbandonCallback = canAbandonCallback;
		};

		void DoWork()
		{
			if (WorkCallback)
			{
				WorkCallback();
			}
		}

		bool CanAbandon()
		{
			return CanAbandonCallback ? CanAbandonCallback() : true;
		}

		void Abandon()
		{
			if (AbandonCallback)
			{
				AbandonCallback();
			}
		}

		FORCEINLINE TStatId GetStatId() const
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(MyAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
		}
	};

	/**
	 * Creates an asynchronous task for multi threading.
	 * @param workCallback Function that will be called when the task is executed. Can also be a normal C++ lambda.
	 * @param abandonCallback Function that will be called when the task is abandoned, before it was executed.
	 * @param canAbandonCallback Will be called when this task is about to get abandoned. When the function returns true, it is allowed to get abandoned, otherwise not. By default we will return "true".
	 */
	static FAutoDeleteAsyncTask<MyAsyncTask>* CreateTask(TFunction<void (void)> workCallback);
	static FAutoDeleteAsyncTask<MyAsyncTask>* CreateTask(TFunction<void (void)> workCallback, TFunction<void (void)> abandonCallback, TFunction<bool (void)> canAbandonCallback);
};
