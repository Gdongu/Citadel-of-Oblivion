#include "DoorInteractActor.h"
#include "Components/SphereComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Knight.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FortressGameInstance.h"
#include "Kismet/GameplayStatics.h"

ADoorInteractActor::ADoorInteractActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	DoorFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrameMesh"));
	DoorFrameMesh->SetupAttachment(RootComponent);

	LeftDoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LeftDoorPivot"));
	LeftDoorPivot->SetupAttachment(RootComponent);

	RightDoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("RightDoorPivot"));
	RightDoorPivot->SetupAttachment(RootComponent);

	LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
	LeftDoorMesh->SetupAttachment(LeftDoorPivot);

	RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
	RightDoorMesh->SetupAttachment(RightDoorPivot);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(180.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	DoorTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DoorTimeline"));
}

void ADoorInteractActor::BeginPlay()
{
	Super::BeginPlay();

	LeftClosedRotation = LeftDoorPivot->GetRelativeRotation();
	RightClosedRotation = RightDoorPivot->GetRelativeRotation();

	if (DoorOpenCurve)
	{
		FOnTimelineFloat UpdateEvent;
		UpdateEvent.BindUFunction(this, FName("UpdateDoorRotation"));
		DoorTimeline->AddInterpFloat(DoorOpenCurve, UpdateEvent);

		FOnTimelineEvent FinishedEvent;
		FinishedEvent.BindUFunction(this, FName("OnDoorTimelineFinished"));
		DoorTimeline->SetTimelineFinishedFunc(FinishedEvent);
	}

	InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &ADoorInteractActor::OnInteractSphereBeginOverlap);
	InteractSphere->OnComponentEndOverlap.AddDynamic(this, &ADoorInteractActor::OnInteractSphereEndOverlap);

	if (DoorUniqueID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Door '%s' has no DoorUniqueID"), *GetName());
	}

	if (IsAlreadyOpened())
	{
		ApplyOpenedStateInstantly();
	}
}

bool ADoorInteractActor::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Interactor || bOpened || bOpening)
	{
		return false;
	}

	return true;
}

FText ADoorInteractActor::GetInteractText_Implementation() const
{
	if (bOpened || bOpening)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(TEXT("[E] 문 열기"));
}

void ADoorInteractActor::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	if (!IsDoorUnlocked())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Yellow,
				LockedMessage.ToString()
			);
		}
		return;
	}

	AKnight* Knight = Cast<AKnight>(Interactor);
	if (!Knight)
	{
		return;
	}

	bOpening = true;
	Knight->ClearInteractActor(this);

	float Delay = 0.2f;

	if (PlayerOpenMontage)
	{
		if (UAnimInstance* AnimInstance = Knight->GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(PlayerOpenMontage);
			Delay = PlayerOpenMontage->GetPlayLength();
		}
	}

	Knight->BeginDoorInteraction(Delay);

	FTimerHandle TimerHandle_OpenDoor;
	GetWorldTimerManager().SetTimer(
		TimerHandle_OpenDoor,
		this,
		&ADoorInteractActor::StartOpenDoor,
		Delay,
		false
	);
}
void ADoorInteractActor::StartOpenDoor()
{
	PlayDoorOpenSound();

	if (DoorTimeline)
	{
		DoorTimeline->PlayFromStart();
	}
}

void ADoorInteractActor::UpdateDoorRotation(float Alpha)
{
	const FRotator LeftTargetRotation =
		LeftClosedRotation + FRotator(0.f, LeftOpenAngle, 0.f);

	const FRotator RightTargetRotation =
		RightClosedRotation + FRotator(0.f, RightOpenAngle, 0.f);

	const FRotator NewLeftRotation =
		FMath::Lerp(LeftClosedRotation, LeftTargetRotation, Alpha);

	const FRotator NewRightRotation =
		FMath::Lerp(RightClosedRotation, RightTargetRotation, Alpha);

	LeftDoorPivot->SetRelativeRotation(NewLeftRotation);
	RightDoorPivot->SetRelativeRotation(NewRightRotation);
}

void ADoorInteractActor::OnDoorTimelineFinished()
{
	bOpening = false;
	bOpened = true;

	MarkDoorAsOpened();

	if (InteractSphere)
	{
		InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ADoorInteractActor::OnInteractSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (AKnight* Knight = Cast<AKnight>(OtherActor))
	{
		if (!bOpened && !bOpening)
		{
			Knight->SetInteractActor(this);
		}
	}
}

void ADoorInteractActor::OnInteractSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	if (AKnight* Knight = Cast<AKnight>(OtherActor))
	{
		Knight->ClearInteractActor(this);
	}
}

bool ADoorInteractActor::IsAlreadyOpened() const
{
	const UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return false;
	}

	return GI->IsDoorOpened(DoorUniqueID);
}

void ADoorInteractActor::MarkDoorAsOpened()
{
	UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	GI->MarkDoorOpened(DoorUniqueID);
}

void ADoorInteractActor::ApplyOpenedStateInstantly()
{
	bOpened = true;
	bOpening = false;

	const FRotator LeftOpenRot =
		LeftClosedRotation + FRotator(0.f, LeftOpenAngle, 0.f);

	const FRotator RightOpenRot =
		RightClosedRotation + FRotator(0.f, RightOpenAngle, 0.f);

	LeftDoorPivot->SetRelativeRotation(LeftOpenRot);
	RightDoorPivot->SetRelativeRotation(RightOpenRot);

	if (InteractSphere)
	{
		InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

bool ADoorInteractActor::IsDoorUnlocked() const
{
	if (!bRequireBossDefeat)
	{
		return true;
	}

	const UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return false;
	}

	return GI->IsBossDefeated(RequiredBossID);
}

void ADoorInteractActor::PlayDoorOpenSound()
{
	if (!DoorOpenSound)
	{
		return;
	}

	const float Pitch = FMath::RandRange(
		DoorOpenSoundPitchMin,
		DoorOpenSoundPitchMax
	);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		DoorOpenSound,
		GetActorLocation(),
		DoorOpenSoundVolume,
		Pitch
	);
}