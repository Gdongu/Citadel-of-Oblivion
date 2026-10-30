#include "EndingCinematicActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Knight.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"

AEndingCinematicActor::AEndingCinematicActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(150.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AEndingCinematicActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractSphere)
	{
		InteractSphere->OnComponentBeginOverlap.AddDynamic(
			this,
			&AEndingCinematicActor::OnInteractSphereBeginOverlap
		);

		InteractSphere->OnComponentEndOverlap.AddDynamic(
			this,
			&AEndingCinematicActor::OnInteractSphereEndOverlap
		);
	}
}

bool AEndingCinematicActor::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor != nullptr && !bTriggered;
}

FText AEndingCinematicActor::GetInteractText_Implementation() const
{
	return FText::FromString(TEXT("[E] 희미한 기억의 핵을 넣는다."));
}

void AEndingCinematicActor::Interact_Implementation(AActor* Interactor)
{
	if (bTriggered)
	{
		return;
	}

	AKnight* Knight = Cast<AKnight>(Interactor);
	if (!Knight)
	{
		return;
	}

	bTriggered = true;

	// 플레이어 행동 정지
	Knight->InterruptCurrentAction();
	//Knight->GetCharacterMovement()->StopMovementImmediately();
	Knight->walkSpeed = 0.f;
	Knight->SetPlayerHUDVisible(false);

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		PC->bShowMouseCursor = false;

		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	// 시네마틱 위젯 표시
	if (EndingCinematicWidgetClass)
	{
		EndingCinematicWidget = CreateWidget<UUserWidget>(GetWorld(), EndingCinematicWidgetClass);
		if (EndingCinematicWidget)
		{
			EndingCinematicWidget->AddToViewport(500);
		}
	}

	// 끝나면 시작 레벨로 복귀
	GetWorldTimerManager().SetTimer(
		TimerHandle_EndingFinish,
		this,
		&AEndingCinematicActor::FinishEndingAndReturnToStart,
		EndingCinematicDuration,
		false
	);
}

void AEndingCinematicActor::FinishEndingAndReturnToStart()
{
	if (EndingCinematicWidget)
	{
		EndingCinematicWidget->RemoveFromParent();
		EndingCinematicWidget = nullptr;
	}

	UGameplayStatics::OpenLevel(this, StartLevelName);
}

void AEndingCinematicActor::OnInteractSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (bTriggered || !OtherActor)
	{
		return;
	}

	if (AKnight* Knight = Cast<AKnight>(OtherActor))
	{
		Knight->SetInteractActor(this);
	}
}

void AEndingCinematicActor::OnInteractSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	if (!OtherActor)
	{
		return;
	}

	if (AKnight* Knight = Cast<AKnight>(OtherActor))
	{
		Knight->ClearInteractActor(this);
	}
}