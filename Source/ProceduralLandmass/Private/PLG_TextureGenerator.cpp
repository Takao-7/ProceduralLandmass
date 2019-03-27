// Fill out your copyright notice in the Description page of Project Settings.

#include "PLG_TextureGenerator.h"
#include "Engine/Texture2D.h"
#include "Array2D.h"


UTexture2D* UPLG_TextureGenerator::TextureFromColorMap(const TArray<FLinearColor>& colorMap)
{
	const int32 size = FMath::Sqrt(colorMap.Num());

	UTexture2D* texture = UTexture2D::CreateTransient(size, size, EPixelFormat::PF_A32B32G32R32F); /* Create a texture with a 32 bit pixel format for the Linear Color. */
	texture->Filter = TextureFilter::TF_Nearest;
	texture->AddressX = TextureAddress::TA_Clamp;
	texture->AddressY = TextureAddress::TA_Clamp;

	FTexture2DMipMap& Mip = texture->PlatformData->Mips[0]; /* A texture has several mips, which are basically their level of detail. Mip 0 is the highest detail. */
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE); /* Get the bulk (raw) data from the mip and lock it for read/write access. */
	FMemory::Memcpy(Data, (void*)colorMap.GetData(), Mip.BulkData.GetBulkDataSize()); /* Copy the color map's raw data (the internal array) to the location of the texture's bulk data location. */
	Mip.BulkData.Unlock(); /* Unlock the bulk data. */
	
	texture->UpdateResource(); /* Update the texture, so that it's ready to be used. */

	return texture;
}

UTexture2D* UPLG_TextureGenerator::TextureFromHeightMap(const FArray2D& heightMap)
{
	const int32 size = heightMap[0].Num();

	TArray<FLinearColor> colorMap; colorMap.SetNum(size * size);
	for (int32 y = 0; y < size; y++)
	{
		for (int32 x = 0; x < size; x++)
		{
			FLinearColor color = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor::White, heightMap[y][x]);
			colorMap[y * size + x] = color;
		}
	}

	return TextureFromColorMap(colorMap);
}
