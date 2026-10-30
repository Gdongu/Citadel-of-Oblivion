#include "BossRoomEntranceGate.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Knight.h"
#include "FortressGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TutorialBossMonster.h"
#include "Kismet/GameplayStatics.h"

ABossRoomEntranceGate::ABossRoomEntranceGate()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	GateFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateFrameMesh"));
	GateFrameMesh->SetupAttachment(RootComponent);

	BarrierMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierMesh"));
	BarrierMesh->SetupAttachment(RootComponent);

	TeleportTarget = CreateDefaultSubobject<USceneComponent>(TEXT("TeleportTarget"));
	TeleportTarget->SetupAttachment(RootComponent);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(180.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ABossRoomEntranceGate::BeginPlay()
{
	Super::BeginPlay();

	InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &ABossRoomEntranceGate::OnInteractSphereBeginOverlap);
	InteractSphere->OnComponentEndOverlap.AddDynamic(this, &ABossRoomEntranceGate::OnInteractSphereEndOverlap);

	RefreshBarrierState();
	TryBindBossDeathEvent();
}

void ABossRoomEntranceGate::RefreshBarrierState()
{
	bBarrierRemoved = false;

	if (BossUniqueIDToCheck.IsNone())
	{
		return;
	}

	const UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	if (GI->IsBossDefeated(BossUniqueIDToCheck))
	{
		bBarrierRemoved = true;

		if (BarrierMesh)
		{
			BarrierMesh->SetVisibility(false, true);
			BarrierMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		DisableInteraction();
	}
}

bool ABossRoomEntranceGate::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Interactor || !TeleportTarget)
	{
		return false;
	}

	if (bEnteredOnce || bBarrierRemoved)
	{
		return false;
	}

	return true;
}

FText ABossRoomEntranceGate::GetInteractText_Implementation() const
{
	if (bEnteredOnce || bBarrierRemoved)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(TEXT("[E] 보스방 입장"));
}

void ABossRoomEntranceGate::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	AKnight* Knight = Cast<AKnight>(Interactor);
	if (!Knight || !TeleportTarget)
	{
		return;
	}

	bEnteredOnce = true;
	Knight->ClearInteractActor(this);
	DisableInteraction();

	const FVector TargetLocation = TeleportTarget->GetComponentLocation();
	const FRotator TargetRotation = TeleportTarget->GetComponentRotation();

	Knight->SetActorLocationAndRotation(
		TargetLocation,
		FRotator(0.f, TargetRotation.Yaw, 0.f)
	);

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->SetControlRotation(FRotator(0.f, TargetRotation.Yaw, 0.f));
	}
}

void ABossRoomEntranceGate::OnInteractSphereBeginOverlap(
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
		if (!bEnteredOnce && !bBarrierRemoved)
		{
			Knight->SetInteractActor(this);
		}
	}
}

void ABossRoomEntranceGate::OnInteractSphereEndOverlap(
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

void ABossRoomEntranceGate::TryBindBossDeathEvent()
{
	if (BossUniqueIDToCheck.IsNone())
	{
		return;
	}

	TArray<AActor*> FoundBosses;
	UGameplayStatics::GetAllActorsOfClass(this, ATutorialBossMonster::StaticClass(), FoundBosses);

	for (AActor* Actor : FoundBosses)
	{
		ATutorialBossMonster* Boss = Cast<ATutorialBossMonster>(Actor);
		if (!Boss)
		{
			continue;
		}

		if (Boss->GetBossUniqueID() == BossUniqueIDToCheck)
		{
			BoundBoss = Boss;
			BoundBoss->OnDied.AddDynamic(this, &ABossRoomEntranceGate::HandleObservedBossDied);
			break;
		}
	}
}

void ABossRoomEntranceGate::HandleObservedBossDied()
{
	RefreshBarrierState();
}

void ABossRoomEntranceGate::DisableInteraction()
{
	if (InteractSphere)
	{
		InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	TArray<AActor*> OverlappingActors;
	if (InteractSphere)
	{
		InteractSphere->GetOverlappingActors(OverlappingActors, AKnight::StaticClass());
	}

	for (AActor* Actor : OverlappingActors)
	{
		if (AKnight* Knight = Cast<AKnight>(Actor))
		{
			Knight->ClearInteractActor(this);
		}
	}
}