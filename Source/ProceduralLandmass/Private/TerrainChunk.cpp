#include "TerrainChunk.h"
#include "TerrainGenerator.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/BoxComponent.h"
#include "LODInfo.h"


FVector UTerrainChunk::CameraLocation = FVector::ZeroVector;
FVector UTerrainChunk::LastCameraLocation = FVector::ZeroVector;


UTerrainChunk::~UTerrainChunk()
{
	for (FTerrainMeshData* meshData : LODMeshes)
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

void UTerrainChunk::InitChunk(ATerrainGenerator* parentTerrainGenerator, TArray<FLODInfo>* lodInfoArray)
{
	DetailLevels = lodInfoArray;
	TerrainGenerator = parentTerrainGenerator;

	const int32 maxLOD = DetailLevels->Last().LOD;
	LODMeshes.SetNum(maxLOD + 1);
	RequestedMeshData.SetNum(maxLOD + 1);
	AttachToComponent(TerrainGenerator->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
	
	TotalChunkSize = TerrainGenerator->Configuration.GetChunkSize() * parentTerrainGenerator->Configuration.MapScale;
}

void UTerrainChunk::SetChunkBoundingBox()
{
	int32 chunksSize = TerrainGenerator->Configuration.GetChunkSize();
	Box = FBox::BuildAABB(GetComponentLocation(), FVector(chunksSize / 2));
}

void UTerrainChunk::UpdateChunk(FVector cameraLocation)
{
	if (Status != EChunkStatus::IDLE || cameraLocation == FVector::ZeroVector)
	{
		return;
	}

	const FVector chunkLocation = GetComponentLocation();
	const float distanceToCamera = FMath::Sqrt(Box.ComputeSquaredDistanceToPoint(cameraLocation));

	const int32 newLOD = FLODInfo::FindLOD(*DetailLevels, distanceToCamera);
	if (newLOD == CurrentLOD)
	{
		return;
	}

	/* Request a mesh data for the new LOD if we don't have one and didn't already requested one. */
	if (LODMeshes[newLOD] == nullptr && RequestedMeshData[newLOD] == false)
	{
		RequestedMeshData[newLOD] = true;
		const FVector2D relativePosition = FVector2D(GetRelativeTransform().GetLocation());
		TerrainGenerator->CreateAndEnqueueMeshDataJob(this, newLOD, false, relativePosition);
		return;
	}

	SetNewLOD(newLOD);
}

void UTerrainChunk::UpdateChunk()
{
	UpdateChunk(UTerrainChunk::CameraLocation);
}

int32 UTerrainChunk::GetOptimalLOD(FVector cameraLocation)
{
	const float distanceToCamera = FMath::Sqrt(Box.ComputeSquaredDistanceToPoint(cameraLocation));
	return FLODInfo::FindLOD(*DetailLevels, distanceToCamera);
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
