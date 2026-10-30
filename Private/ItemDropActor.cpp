#include "ItemDropActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Knight.h"
#include "FortressGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComponent.h"

AItemDropActor::AItemDropActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(120.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AItemDropActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bUsePersistentDropSave || DropID.IsNone())
	{
		return;
	}

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		const FName SaveKey = MakeDropSaveKey();

		if (GI->IsWorldDropConsumed(SaveKey))
		{
			Destroy();
		}
	}
}

void AItemDropActor::InitializeDrop(const FInventoryItem& InItem)
{
	DroppedItem = InItem;
}

bool AItemDropActor::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor != nullptr && DroppedItem.IsValid();
}

FText AItemDropActor::GetInteractText_Implementation() const
{
	return FText::Format(
		FText::FromString(TEXT("[E] {0} 획득 x{1}")),
		DroppedItem.ItemData.ItemName,
		FText::AsNumber(DroppedItem.Count)
	);
}

void AItemDropActor::Interact_Implementation(AActor* Interactor)
{
	AKnight* Knight = Cast<AKnight>(Interactor);
	if (!Knight)
	{
		return;
	}

	if (!Knight->StartItemPickupMontage())
	{
		return;
	}

	UInventoryComponent* InvComp = Knight->GetInventoryComp();
	if (!InvComp)
	{
		Knight->FinishItemPickupMontage();
		return;
	}

	if (InvComp->AddItem(DroppedItem.ItemData, DroppedItem.Count))
	{
		if (bUsePersistentDropSave && !DropID.IsNone())
		{
			if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
			{
				GI->MarkWorldDropConsumed(MakeDropSaveKey());
			}
		}

		Knight->ShowItemPickupNotice(DroppedItem.ItemData, DroppedItem.Count);
		Knight->ClearInteractActor(this);

		Destroy();
	}
	else
	{
		Knight->FinishItemPickupMontage();
	}
}

FName AItemDropActor::MakeDropSaveKey() const
{
	if (DropID.IsNone())
	{
		return NAME_None;
	}

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	const FString KeyString = FString::Printf(
		TEXT("%s_%s"),
		*LevelName,
		*DropID.ToString()
	);

	return FName(*KeyString);
}