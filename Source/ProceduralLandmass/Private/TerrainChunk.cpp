#include "TerrainChunk.h"
#include "TerrainGenerator.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/BoxComponent.h"
#include "LODInfo.h"


FVector UTerrainChunk::CameraLocation = FVector::ZeroVector;


UTerrainChunk::~UTerrainChunk()
{
	for (FMeshData* meshData : LODMeshes)
	{
		if (meshData)
		{
			delete meshData;
			meshData = nullptr;
		}
	}

	delete HeightMap;
	HeightMap = nullptr;
}

void UTerrainChunk::InitChunk(ATerrainGenerator* parentTerrainGenerator, TArray<FLODInfo>* lodInfoArray, FVector2D noiseOffset /*= FVector2D::ZeroVector*/)
{
	this->DetailLevels = lodInfoArray;
	this->TerrainGenerator = parentTerrainGenerator;

	const int32 maxLOD = DetailLevels->Last().LOD;
	LODMeshes.SetNum(maxLOD + 1);
	RequestedMeshData.SetNum(maxLOD + 1);
	AttachToComponent(TerrainGenerator->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);

	TotalChunkSize = TerrainGenerator->Configuration.GetChunkSize() * parentTerrainGenerator->Configuration.MapScale;
	NoiseOffset = noiseOffset;
}

void UTerrainChunk::UpdateChunk(FVector cameraLocation)
{
	if (Status != EChunkStatus::IDLE)
	{
		return;
	}

	const FVector chunkLocation = GetComponentLocation();
	const float distanceToCamera = FVector::Dist(chunkLocation, cameraLocation) - TotalChunkSize;

	const int32 newLOD = FLODInfo::FindLOD(*DetailLevels, distanceToCamera);
	if (newLOD == CurrentLOD)
	{
		return;
	}

	/* Request a mesh data for the new LOD if we don't have one and didn't already requested one. */
	if (LODMeshes[newLOD] == nullptr && RequestedMeshData[newLOD] == false)
	{
		RequestedMeshData[newLOD] = true;
		TerrainGenerator->CreateAndEnqueueMeshDataJob(this, newLOD, false, NoiseOffset);
		return;
	}

	SetNewLOD(newLOD);
}

void UTerrainChunk::UpdateChunk()
{
	UpdateChunk(UTerrainChunk::CameraLocation);
}

void UTerrainChunk::SetNewLOD(int32 newLOD)
{
	if (newLOD == CurrentLOD || LODMeshes[newLOD] == nullptr)
	{
		return;
	}
	
	RequestedMeshData[newLOD] = true;

	SetMeshSectionVisible(CurrentLOD, false);
	SetMeshSectionVisible(newLOD, true);
	CurrentLOD = newLOD;
}
