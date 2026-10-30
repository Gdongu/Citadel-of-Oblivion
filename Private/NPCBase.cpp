#include "NPCBase.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Knight.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"

ANPCBase::ANPCBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	NPCMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NPCMesh"));
	NPCMesh->SetupAttachment(RootComponent);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(180.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	DialogueCameraPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DialogueCameraPoint"));
	DialogueCameraPoint->SetupAttachment(RootComponent);
	DialogueCameraPoint->SetRelativeLocation(FVector(120.f, 0.f, 70.f));
	DialogueCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DialogueCamera"));
	DialogueCamera->SetupAttachment(DialogueCameraPoint);
}

void ANPCBase::BeginPlay()
{
	Super::BeginPlay();
}

bool ANPCBase::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor != nullptr && !bIsTalking;
}

FText ANPCBase::GetInteractText_Implementation() const
{
	return FText::FromString(TEXT("[E] 대화하기"));
}

void ANPCBase::Interact_Implementation(AActor* Interactor)
{
	AKnight* Knight = Cast<AKnight>(Interactor);
	if (!Knight)
	{
		return;
	}

	StartDialogue(Knight);
}

void ANPCBase::StartDialogue(AKnight* Knight)
{
	if (!Knight || bIsTalking)
	{
		return;
	}

	CurrentInteractingKnight = Knight;
	CurrentDialogueIndex = 0;
	bIsTalking = true;

	APlayerController* PC = Cast<APlayerController>(Knight->GetController());
	if (!PC)
	{
		return;
	}

	EnterDialogueCamera(PC);

	if (DialogueWidgetClass && !DialogueWidget)
	{
		DialogueWidget = CreateWidget<UUserWidget>(GetWorld(), DialogueWidgetClass);
	}

	if (DialogueWidget)
	{
		DialogueWidget->AddToViewport(500);
	}

	PC->bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	if (DialogueWidget)
	{
		InputMode.SetWidgetToFocus(DialogueWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	Knight->SetPlayerHUDVisible(false);
}

void ANPCBase::AdvanceDialogue()
{
	if (!bIsTalking)
	{
		return;
	}

	if (CurrentDialogueIndex + 1 < DialogueLines.Num())
	{
		CurrentDialogueIndex++;
	}
}

void ANPCBase::EndDialogue()
{
	if (!CurrentInteractingKnight)
	{
		bIsTalking = false;
		return;
	}

	APlayerController* PC = Cast<APlayerController>(CurrentInteractingKnight->GetController());
	if (PC)
	{
		ExitDialogueCamera(PC);

		PC->bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}

	if (DialogueWidget)
	{
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	CurrentInteractingKnight->SetPlayerHUDVisible(true);

	CurrentInteractingKnight = nullptr;
	CurrentDialogueIndex = 0;
	bIsTalking = false;
}

FText ANPCBase::GetCurrentDialogueText() const
{
	if (!DialogueLines.IsValidIndex(CurrentDialogueIndex))
	{
		return FText::GetEmpty();
	}

	return DialogueLines[CurrentDialogueIndex];
}

bool ANPCBase::IsLastDialogueLine() const
{
	return !DialogueLines.IsValidIndex(CurrentDialogueIndex + 1);
}

void ANPCBase::OnShopButtonClicked()
{
	// 나중에 구현
}

void ANPCBase::OnEnhanceButtonClicked()
{
	// 나중에 구현
}

void ANPCBase::EnterDialogueCamera(APlayerController* PC)
{
	if (!PC || !DialogueCameraPoint || !DialogueCamera)
	{
		return;
	}

	const FVector CamLoc = DialogueCameraPoint->GetComponentLocation();
	const FVector LookTarget = NPCMesh->GetComponentLocation() + FVector(0.f, 0.f, 60.f);
	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(CamLoc, LookTarget);

	DialogueCamera->SetWorldLocation(CamLoc);
	DialogueCamera->SetWorldRotation(LookAtRot);

	PC->SetViewTargetWithBlend(this, 0.35f);
}

void ANPCBase::ExitDialogueCamera(APlayerController* PC)
{
	if (!PC || !CurrentInteractingKnight)
	{
		return;
	}

	PC->SetViewTargetWithBlend(CurrentInteractingKnight, 0.25f);
}