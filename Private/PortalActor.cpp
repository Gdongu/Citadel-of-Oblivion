#include "PortalActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Knight.h"
#include "FortressGameInstance.h"
#include "GameFramework/PlayerController.h"

APortalActor::APortalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootComponent);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(150.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void APortalActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractSphere)
	{
		InteractSphere->OnComponentBeginOverlap.AddDynamic(
			this,
			&APortalActor::OnInteractSphereBeginOverlap
		);

		InteractSphere->OnComponentEndOverlap.AddDynamic(
			this,
			&APortalActor::OnInteractSphereEndOverlap
		);
	}
}

void APortalActor::OnInteractSphereBeginOverlap(
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

void APortalActor::OnInteractSphereEndOverlap(
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

bool APortalActor::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor != nullptr && !bTriggered && !TargetLevelName.IsNone();
}

FText APortalActor::GetInteractText_Implementation() const
{
	return InteractText;
}

void APortalActor::Interact_Implementation(AActor* Interactor)
{
	if (bTriggered || TargetLevelName.IsNone())
	{
		return;
	}

	AKnight* Knight = Cast<AKnight>(Interactor);
	if (!Knight)
	{
		return;
	}

	bTriggered = true;

	Knight->InterruptCurrentAction();

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	Knight->SaveInventoryToGameInstance();
	Knight->SaveEnhancementToGameInstance();

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		if (!TargetArrivalTag.IsNone())
		{
			GI->SetPendingPortalTravel(TargetLevelName, TargetArrivalTag);
		}

		if (bUseLoadingScreen)
		{
			GI->SetPendingLoadingScreen(LoadingScreenDuration);
		}
	}

	UGameplayStatics::OpenLevel(this, TargetLevelName);
}