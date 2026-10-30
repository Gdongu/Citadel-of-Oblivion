#include "MemoryShardDrop.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Knight.h"
#include "FortressGameInstance.h"

AMemoryShardDrop::AMemoryShardDrop()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(150.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AMemoryShardDrop::BeginPlay()
{
	Super::BeginPlay();
}

void AMemoryShardDrop::InitializeDrop(int32 InAmount)
{
	StoredAmount = FMath::Max(0, InAmount);
}

bool AMemoryShardDrop::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor != nullptr;
}

FText AMemoryShardDrop::GetInteractText_Implementation() const
{
	return FText::Format(
		FText::FromString(TEXT("[E] 기억의 파편 회수 ({0})")),
		FText::AsNumber(StoredAmount)
	);
}

void AMemoryShardDrop::Interact_Implementation(AActor* Interactor)
{
	AKnight* Knight = Cast<AKnight>(Interactor);
	if (!Knight)
	{
		return;
	}

	Knight->AddMemoryShards(StoredAmount, false);

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		GI->ClearPendingMemoryShardDrop();
	}

	Destroy();
}