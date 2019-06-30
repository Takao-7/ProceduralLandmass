#pragma once
#include "CoreMinimal.h"
#include "Object.h"
#include "NoiseGeneratorInterface.generated.h"


UCLASS(Abstract, Blueprintable)
class PROCEDURALLANDMASS_API UNoiseGenerator : public UObject
{
    GENERATED_BODY()

public:
	UNoiseGenerator() {};
	UNoiseGenerator(float noiseScale, int32 seed, float persistence, float lacunarity, int32 octaves) :
		NoiseScale(noiseScale), Seed(seed), Persistence(persistence), Lacunarity(lacunarity), Octaves(octaves) {};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Noise Generator")
	void CopyGenerator(const UNoiseGenerator* otherGenerator);
	virtual void CopyGenerator_Implementation(const UNoiseGenerator* otherGenerator)
	{
		NoiseScale = otherGenerator->NoiseScale;
		Seed = otherGenerator->Seed;
		Persistence = otherGenerator->Persistence;
		Lacunarity = otherGenerator->Lacunarity;
		Octaves = otherGenerator->Octaves;
		Limit = otherGenerator->Limit;
		OctaveOffsets = otherGenerator->OctaveOffsets;
	};

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Noise Generator")
    float GetNoise2D(float X, float Y) const;
    virtual float GetNoise2D_Implementation(float X, float Y) const { return 0.0f; };

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Noise Generator")
    float GetNoise3D(float X, float Y, float Z) const;
    virtual float GetNoise3D_Implementation(float X, float Y, float Z) const { return 0.0f; };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
	float NoiseScale = 50.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
	int32 Seed = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
	float Persistence = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
	float Lacunarity = 2.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true))
	int32 Octaves = 4;

	UPROPERTY(BlueprintReadWrite)
	float Limit = 1.0f;

	UPROPERTY(BlueprintReadWrite)
	TArray<FVector2D> OctaveOffsets;
};
