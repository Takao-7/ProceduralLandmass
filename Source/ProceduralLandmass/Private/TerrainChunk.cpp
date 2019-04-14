#include "TerrainChunk.h"
#include "TerrainGenerator.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/BoxComponent.h"
#include "LODInfo.h"


UTerrainChunk::UTerrainChunk()
{
	
}

void UTerrainChunk::InitChunk(ATerrainGenerator* parentTerrainGenerator, TArray<FLODInfo>* lodInfoArray)
{
	this->DetailLevels = lodInfoArray;
	this->TerrainGenerator = parentTerrainGenerator;

	const int32 maxLOD = DetailLevels->Last().LOD;
	LODMeshes.SetNum(maxLOD + 1);
	AttachToComponent(TerrainGenerator->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);

	/*CollisionBox = NewObject<UBoxComponent>(this, TEXT("Collision box"));
	CollisionBox->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Overlap);
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &UTerrainChunk::HandleCameraOverlap);
	
	const FVector boxSize = FVector(parentTerrainGenerator->Configuration.GetNumVertices()-1);
	CollisionBox->SetBoxExtent(boxSize);*/
}

void UTerrainChunk::HandleCameraOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UKismetSystemLibrary::PrintString(this, TEXT("Camera Overlap"));
}
