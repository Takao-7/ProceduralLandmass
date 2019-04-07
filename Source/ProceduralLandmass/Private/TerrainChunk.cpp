#include "TerrainChunk.h"
#include "TerrainGenerator.h"
#include "Kismet/KismetSystemLibrary.h"


void UTerrainChunk::InitChunk(float viewDistance, ATerrainGenerator* parentTerrainGenerator, TArray<FLODInfo>* lodInfoArray, AActor* viewer, float zPosition /*= 0.0f*/)
{
	this->MaxViewDistance = viewDistance;
	this->DetailLevels = lodInfoArray;
	this->Viewer = viewer;
	this->TerrainGenerator = parentTerrainGenerator;

	SetIsVisible(false);
	LODmeshes.SetNum(DetailLevels->Num());
	for (int32 i = 0; i < LODmeshes.Num(); i++)
	{
		//LODmeshes[i] = FLODMesh((*DetailLevels)[i].LOD, TerrainGenerator);
	}

	const bool bSuccessFullyAttached = AttachToComponent(TerrainGenerator->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	if(!bSuccessFullyAttached)
	{
		UKismetSystemLibrary::PrintString(this, TEXT("Terrain chunk not successfuly attached!"));
	}
}

bool UTerrainChunk::UpdateTerrainChunk()
{
	if (Status != EChunkStatus::IDLE)
	{
		return false; // We are not ready, because we don't have mesh data.
	}

	const FVector viewerPosition = Viewer ? Viewer->GetActorLocation() : FVector::ZeroVector;
	const float viewerDistanceToCenter = FVector::Distance(Location, viewerPosition);

	const bool bIsVisible = (viewerDistanceToCenter <= MaxViewDistance)/* || viewerDistanceToCenter < ChunkSize*/;
	SetIsVisible(bIsVisible);

	if (!bIsVisible)
	{
		return false;	// When we are not visible, we don't have to update our level of detail.
	}

	// Find the new level of detail
	CurrentLOD = 0;
	for (int32 i = 0; i < DetailLevels->Num() - 1; i++)
	{
		if (viewerDistanceToCenter > (*DetailLevels)[i].VisibleDistanceThreshold)
		{
			CurrentLOD++;
		}
		else
		{
			break;
		}
	}

	if (CurrentLOD != PreviousLOD)
	{
		PreviousLOD = CurrentLOD;

		FMeshData* lodMesh = LODmeshes[CurrentLOD];
		if (lodMesh)
		{
			lodMesh->CreateMesh(MeshComponent);
		}
		else
		{
			// Request mesh data.
		}
	}

	return true;
}

void UTerrainChunk::SetIsVisible(bool bVisible)
{
	if (MeshComponent)
	{
		MeshComponent->SetMeshSectionVisible(CurrentLOD, bVisible);
	}
}

void UTerrainChunk::DeleteMesh()
{
	ClearAllMeshSections();
}
