#include "Bonfire.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "FortressGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Knight.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

ABonfire::ABonfire()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	BonfireMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BonfireMesh"));
	BonfireMesh->SetupAttachment(RootComponent);

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(150.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	FireEffectComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireEffectComp"));
	FireEffectComp->SetupAttachment(RootComponent);
	FireEffectComp->SetAutoActivate(false);
	FireEffectComp->SetVisibility(false);

	FireAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudioComp"));
	FireAudioComp->SetupAttachment(RootComponent);
	FireAudioComp->bAutoActivate = false;
	FireAudioComp->bAutoDestroy = false;
	FireAudioComp->SetVolumeMultiplier(FireSoundVolume);
}

void ABonfire::BeginPlay()
{
	Super::BeginPlay();

	if (FireEffect)
	{
		FireEffectComp->SetAsset(FireEffect);
	}

	if (FireAudioComp && FireLoopSound)
	{
		FireAudioComp->SetSound(FireLoopSound);
		FireAudioComp->SetVolumeMultiplier(FireSoundVolume);
	}

	InteractSphere->OnComponentBeginOverlap.AddDynamic(
		this,
		&ABonfire::OnInteractSphereBeginOverlap
	);

	InteractSphere->OnComponentEndOverlap.AddDynamic(
		this,
		&ABonfire::OnInteractSphereEndOverlap
	);

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		if (GI->IsBonfireActivated(BonfireID))
		{
			bActivated = true;
		}
	}

	if (bActivated)
	{
		FireEffectComp->SetVisibility(true);
		FireEffectComp->Activate(true);
	}
	else
	{
		FireEffectComp->Deactivate();
		FireEffectComp->SetVisibility(false);
	}
}

void ABonfire::Interact_Implementation(AActor* Interactor)
{
	if (AKnight* Knight = Cast<AKnight>(Interactor))
	{
		if (!bActivated)
		{
			Knight->BeginBonfireInteract(this);
		}
		else
		{
			Knight->StartBonfireRest(this);
		}
	}
}

bool ABonfire::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor != nullptr;
}

FText ABonfire::GetInteractText_Implementation() const
{
	return bActivated
		? FText::FromString(TEXT("[E] 휴식"))
		: FText::FromString(TEXT("[E] 점화"));
}

void ABonfire::ActivateBonfire()
{
	if (bActivated)
	{
		return;
	}

	bActivated = true;

	if (FireEffectComp)
	{
		FireEffectComp->SetVisibility(true);
		FireEffectComp->Activate(true);
	}

	StartFireSoundIfPlayerInside();
}

FTransform ABonfire::GetRespawnTransform() const
{
	if (USceneComponent* RespawnComp = FindRespawnPointComponent())
	{
		return RespawnComp->GetComponentTransform();
	}

	return GetActorTransform();
}

USceneComponent* ABonfire::FindRespawnPointComponent() const
{
	TArray<USceneComponent*> SceneComps;
	GetComponents<USceneComponent>(SceneComps);

	for (USceneComponent* Comp : SceneComps)
	{
		if (Comp && Comp->ComponentHasTag(RespawnPointTag))
		{
			return Comp;
		}
	}

	return nullptr;
}

void ABonfire::OnPlayerFinishedInteract(AActor* Interactor)
{
	ActivateBonfire();

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));

		GI->SetCheckpointData(CurrentLevelName, GetRespawnTransform());
		GI->RegisterBonfire(
			BonfireID,
			CurrentLevelName,
			GetRestTransform(),
			BonfireDisplayName,
			BonfireNumber
		);
	}
}

FTransform ABonfire::GetRestTransform() const
{
	TArray<USceneComponent*> SceneComps;
	GetComponents<USceneComponent>(SceneComps);

	for (USceneComponent* Comp : SceneComps)
	{
		if (Comp && Comp->ComponentHasTag(RestPointTag))
		{
			return Comp->GetComponentTransform();
		}
	}

	return GetActorTransform();
}

void ABonfire::OnInteractSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!bActivated)
	{
		return;
	}

	if (Cast<AKnight>(OtherActor))
	{
		StartFireSoundIfPlayerInside();
	}
}

void ABonfire::OnInteractSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	if (Cast<AKnight>(OtherActor))
	{
		StopFireSound();
	}
}

void ABonfire::StartFireSoundIfPlayerInside()
{
	if (!bActivated || !FireAudioComp || !FireLoopSound || !InteractSphere)
	{
		return;
	}

	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!PlayerCharacter)
	{
		return;
	}

	if (!InteractSphere->IsOverlappingActor(PlayerCharacter))
	{
		return;
	}

	if (!FireAudioComp->IsPlaying())
	{
		FireAudioComp->FadeIn(FireSoundFadeTime, FireSoundVolume);
	}
}

void ABonfire::StopFireSound()
{
	if (!FireAudioComp)
	{
		return;
	}

	if (FireAudioComp->IsPlaying())
	{
		FireAudioComp->FadeOut(FireSoundFadeTime, 0.f);
	}
}