#include "TutorialTriggerZone.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "FortressGameInstance.h"
#include "Knight.h"
#include "TutorialPopupWidget.h"
#include "GameFramework/PlayerController.h"

ATutorialTriggerZone::ATutorialTriggerZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
}

void ATutorialTriggerZone::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATutorialTriggerZone::OnTriggerBeginOverlap);
	}

	if (bTriggerOnlyOnce)
	{
		if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
		{
			if (GI->HasSeenTutorial(TutorialID))
			{
				bAlreadyTriggered = true;
			}
		}
	}
}

void ATutorialTriggerZone::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	AKnight* Knight = Cast<AKnight>(OtherActor);
	if (!Knight)
	{
		return;
	}

	if (bTriggerOnlyOnce && bAlreadyTriggered)
	{
		return;
	}

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		if (GI->HasSeenTutorial(TutorialID))
		{
			bAlreadyTriggered = true;
			return;
		}
	}

	ShowTutorialWidget(Knight);

	if (bTriggerOnlyOnce)
	{
		bAlreadyTriggered = true;

		if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
		{
			GI->MarkTutorialSeen(TutorialID);
		}
	}
}

void ATutorialTriggerZone::ShowTutorialWidget(AKnight* Knight)
{
	if (!Knight || !TutorialWidgetClass)
	{
		return;
	}

	if (!SpawnedTutorialWidget)
	{
		SpawnedTutorialWidget = CreateWidget<UUserWidget>(GetWorld(), TutorialWidgetClass);
	}

	if (!SpawnedTutorialWidget)
	{
		return;
	}

	CurrentShowingKnight = Knight;

	if (!SpawnedTutorialWidget->IsInViewport())
	{
		SpawnedTutorialWidget->AddToViewport(300);
	}

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(SpawnedTutorialWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);

		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	if (UTutorialPopupWidget* TutorialPopup = Cast<UTutorialPopupWidget>(SpawnedTutorialWidget))
	{
		TutorialPopup->OwningTutorialTrigger = this;
	}
}

void ATutorialTriggerZone::HideTutorialWidget()
{
	if (SpawnedTutorialWidget && SpawnedTutorialWidget->IsInViewport())
	{
		SpawnedTutorialWidget->RemoveFromParent();
	}

	if (CurrentShowingKnight)
	{
		if (APlayerController* PC = Cast<APlayerController>(CurrentShowingKnight->GetController()))
		{
			PC->bShowMouseCursor = false;

			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);

			PC->SetIgnoreMoveInput(false);
			PC->SetIgnoreLookInput(false);
		}
	}

	CurrentShowingKnight = nullptr;
}