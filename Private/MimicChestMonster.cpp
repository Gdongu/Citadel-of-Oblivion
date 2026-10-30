#include "MimicChestMonster.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ItemDropActor.h"
#include "MonsterAIController.h"
#include "Knight.h"
#include "BrainComponent.h"
#include "AIController.h"
#include "Components/WidgetComponent.h"
#include "FortressGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "ChestTutorialWidget.h"
#include "Sound/SoundBase.h"
#include "Components/ArrowComponent.h"

AMimicChestMonster::AMimicChestMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(RootComponent);
	InteractSphere->SetSphereRadius(150.f);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ChestOpenPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ChestOpenPoint"));
	ChestOpenPoint->SetupAttachment(RootComponent);
	ChestOpenPoint->SetRelativeLocation(FVector(120.f, 0.f, 0.f));
	ChestOpenPoint->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	ChestOpenPoint->ArrowSize = 1.5f;

	MonsterDisplayName = FText::FromString(TEXT("보물상자"));
}

void AMimicChestMonster::BeginPlay()
{
	Super::BeginPlay();

	// 이미 죽인 상자면 다시 나타나지 않게
	if (IsAlreadyConsumed())
	{
		SetOpenedChestState();
		return;
	}

	SetInitialChestState();

	if (InteractSphere)
	{
		InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &AMimicChestMonster::OnInteractSphereBeginOverlap);
		InteractSphere->OnComponentEndOverlap.AddDynamic(this, &AMimicChestMonster::OnInteractSphereEndOverlap);
	}
}

void AMimicChestMonster::SetInitialChestState()
{
	bOpened = false;
	bAwakenedAsMimic = false;
	bIsAttacking = false;
	bAttackTraceActive = false;
	TargetActor = nullptr;
	SetHasDetectedTarget(false);

	MonsterDisplayName = FText::FromString(TEXT("보물상자"));

	// 처음엔 그냥 상자처럼 가만히
	GetCharacterMovement()->DisableMovement();

	SetMimicPlayerCollisionToOverlap();

	SetLockOnVisible(false);

	if (AggroSphere)
	{
		AggroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}

	if (LockOnWidgetComp)
	{
		LockOnWidgetComp->SetVisibility(false);
	}

	// 상자 메쉬/애님 적용
	if (ClosedChestMesh)
	{
		GetMesh()->SetSkeletalMesh(ClosedChestMesh);
	}

	if (ClosedChestAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(ClosedChestAnimClass);
	}
}

bool AMimicChestMonster::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Interactor)
	{
		return false;
	}

	if (bIsDead || bOpened || bAwakenedAsMimic)
	{
		return false;
	}

	return true;
}

FText AMimicChestMonster::GetInteractText_Implementation() const
{
	if (bOpened || bAwakenedAsMimic)
	{
		return FText::GetEmpty();
	}

	return FText::FromString(TEXT("[E] 보물상자 열기"));
}

void AMimicChestMonster::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return;
	}

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		if (!GI->HasSeenTutorial(ChestTutorialID) && ChestTutorialWidgetClass)
		{
			ShowChestTutorial(Interactor);
			GI->MarkTutorialSeen(ChestTutorialID);
			return;
		}
	}

	StartChestOpenFlow(Interactor);
}

void AMimicChestMonster::StartChestOpenFlow(AActor* Interactor)
{
	if (bOpened || bAwakenedAsMimic || bOpeningInProgress)
	{
		return;
	}

	CachedInteractor = Interactor;
	bOpeningInProgress = true;

	if (AKnight* Knight = Cast<AKnight>(Interactor))
	{
		Knight->ClearInteractActor(this);
		Knight->BeginChestOpen(this);
		return;
	}

	OpenChestAfterPlayerMontage(Interactor);
}

void AMimicChestMonster::OpenChestAfterPlayerMontage(AActor* Interactor)
{
	if (bOpened || bAwakenedAsMimic)
	{
		return;
	}

	bOpeningInProgress = false;
	bOpened = true;
	CachedInteractor = Interactor;

	if (InteractSphere)
	{
		InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AKnight* Knight = Cast<AKnight>(Interactor))
	{
		Knight->ClearInteractActor(this);
	}

	float ResolveDelay = OpenResultDelay;

	PlayChestOpenSound();

	if (ChestOpenMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			const float PlayedLength = AnimInstance->Montage_Play(ChestOpenMontage);

			if (PlayedLength > 0.f)
			{
				ResolveDelay = PlayedLength;
			}
			else
			{
				ResolveDelay = ChestOpenMontage->GetPlayLength();
			}
		}
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_OpenResolve);
	GetWorldTimerManager().SetTimer(
		TimerHandle_OpenResolve,
		this,
		&AMimicChestMonster::ResolveOpenResult,
		FMath::Max(0.01f, ResolveDelay),
		false
	);
}

void AMimicChestMonster::CancelPendingChestOpen(AActor* Interactor)
{
	if (CachedInteractor != Interactor)
	{
		return;
	}

	if (bOpened || bAwakenedAsMimic)
	{
		return;
	}

	bOpeningInProgress = false;
	CachedInteractor = nullptr;
}

void AMimicChestMonster::ResolveOpenResult()
{
	if (ChestType == EChestType::NormalChest)
	{
		OpenAsNormalChest();
	}
	else
	{
		OpenAsMimic();
	}
}

void AMimicChestMonster::OpenAsNormalChest()
{
	MarkChestAsConsumed();
	SpawnChestReward();

	bOpened = true;

	if (InteractSphere)
	{
		InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AggroSphere)
	{
		AggroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AMimicChestMonster::SpawnChestReward()
{
	if (!ChestDropActorClass)
	{
		return;
	}

	if (ChestRewardItemData.ItemID.IsNone())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemDropActor* DropActor = GetWorld()->SpawnActor<AItemDropActor>(
		ChestDropActorClass,
		GetActorLocation() + FVector(0.f, 0.f, 30.f),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (DropActor)
	{
		FInventoryItem DropItem;
		DropItem.ItemData = ChestRewardItemData;
		DropItem.Count = ChestRewardCount;
		DropActor->InitializeDrop(DropItem);
	}
}

void AMimicChestMonster::OpenAsMimic()
{
	SetMimicPlayerCollisionToOverlap();
	PlayMimicTransformSound();

	if (MimicTransformEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			MimicTransformEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	// 미믹 메쉬로 교체
	if (MimicSkeletalMesh)
	{
		GetMesh()->SetSkeletalMesh(MimicSkeletalMesh);
	}

	if (MimicAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(MimicAnimClass);
	}

	ActivateMimic(CachedInteractor);
}

void AMimicChestMonster::ActivateMimic(AActor* Interactor)
{
	bAwakenedAsMimic = true;
	MonsterDisplayName = FText::FromString(TEXT("미믹"));

	CurrentHP = MaxHP;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	UpdateHPWidget();

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// 변신 직후에는 플레이어와 충돌시키지 않음.
	GetWorldTimerManager().ClearTimer(TimerHandle_EnableMimicPlayerBlock);
	GetWorldTimerManager().SetTimer(
		TimerHandle_EnableMimicPlayerBlock,
		this,
		&AMimicChestMonster::EnableMimicPlayerBlockCollision,
		MimicPlayerCollisionBlockDelay,
		false
	);

	if (AggroSphere)
	{
		AggroSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	// 플레이어를 바로 타겟팅
	if (Interactor)
	{
		SetHasDetectedTarget(true);
		SetTargetActor(Interactor);
	}

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsDead(false);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->SetTargetActor(Interactor);

		if (MonsterAI->BrainComponent)
		{
			MonsterAI->BrainComponent->RestartLogic();
		}

		if (Interactor)
		{
			MonsterAI->EnableFullAlertSight(Interactor);
		}
	}
	else if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
	}
}

bool AMimicChestMonster::CanAttack() const
{
	if (!bAwakenedAsMimic)
	{
		return false;
	}

	return Super::CanAttack();
}

bool AMimicChestMonster::CanAutoAcquireTarget() const
{
	return bAwakenedAsMimic && !bOpened && !bIsDead;
}

void AMimicChestMonster::PerformAttack()
{
	if (!bAwakenedAsMimic)
	{
		return;
	}

	Super::PerformAttack();
}

void AMimicChestMonster::OnInteractSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (bOpened || bAwakenedAsMimic || !OtherActor)
	{
		return;
	}

	if (AKnight* Knight = Cast<AKnight>(OtherActor))
	{
		Knight->SetInteractActor(this);
	}
}

void AMimicChestMonster::OnInteractSphereEndOverlap(
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

float AMimicChestMonster::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (bIsDead)
	{
		return 0.f;
	}

	// 이미 깨어난 미믹이면 일반 몬스터 데미지 처리
	if (bAwakenedAsMimic)
	{
		return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}

	// 아직 상자 상태일 때
	if (ChestType == EChestType::NormalChest)
	{
		// 일반 상자는 데미지 완전 무시
		return 0.f;
	}

	// 상자 상태의 미믹이면 첫 타 맞는 순간 바로 변신
	if (ChestType == EChestType::Mimic)
	{
		// 중복 변신 방지
		if (bOpened)
		{
			return 0.f;
		}

		bOpened = true;
		CachedInteractor = DamageCauser;

		if (InteractSphere)
		{
			InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// 상호작용 중 예약된 타이머가 있으면 제거
		GetWorldTimerManager().ClearTimer(TimerHandle_OpenResolve);

		OpenAsMimic();

		// 첫 타는 "변신 트리거"만 하고 데미지는 넣지 않음
		return 0.f;
	}

	return 0.f;
}

bool AMimicChestMonster::IsAlreadyConsumed() const
{
	const UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return false;
	}

	return GI->IsChestConsumed(ChestUniqueID);
}

void AMimicChestMonster::MarkChestAsConsumed()
{
	UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	GI->MarkChestConsumed(ChestUniqueID);
}

void AMimicChestMonster::Die()
{
	if (bIsDead)
	{
		return;
	}

	// 깨어난 미믹이 죽는 순간 영구 제거 기록
	MarkChestAsConsumed();

	Super::Die();
}

void AMimicChestMonster::ShowChestTutorial(AActor* Interactor)
{
	if (!ChestTutorialWidgetClass)
	{
		StartChestOpenFlow(Interactor);
		return;
	}

	AKnight* Knight = Cast<AKnight>(Interactor);
	if (!Knight)
	{
		StartChestOpenFlow(Interactor);
		return;
	}

	CachedInteractor = Interactor;
	bWaitingForChestTutorialClose = true;

	if (!ChestTutorialWidget)
	{
		ChestTutorialWidget = CreateWidget<UUserWidget>(GetWorld(), ChestTutorialWidgetClass);
	}

	if (!ChestTutorialWidget)
	{
		StartChestOpenFlow(Interactor);
		return;
	}

	if (!ChestTutorialWidget->IsInViewport())
	{
		ChestTutorialWidget->AddToViewport(300);
	}

	if (UChestTutorialWidget* ChestWidget = Cast<UChestTutorialWidget>(ChestTutorialWidget))
	{
		ChestWidget->OwningChest = this;
	}

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ChestTutorialWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);

		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}
}

void AMimicChestMonster::CloseChestTutorialAndContinue()
{
	if (ChestTutorialWidget)
	{
		ChestTutorialWidget->RemoveFromParent();
	}

	bWaitingForChestTutorialClose = false;

	AKnight* Knight = Cast<AKnight>(CachedInteractor);
	if (Knight)
	{
		if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
		{
			PC->bShowMouseCursor = false;

			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);

			PC->SetIgnoreMoveInput(false);
			PC->SetIgnoreLookInput(false);
		}
	}

	StartChestOpenFlow(CachedInteractor);
}

void AMimicChestMonster::SetOpenedChestState()
{
	bOpened = true;
	bAwakenedAsMimic = false;
	bIsAttacking = false;
	bAttackTraceActive = false;
	TargetActor = nullptr;
	SetHasDetectedTarget(false);

	MonsterDisplayName = FText::FromString(TEXT("빈 보물상자"));

	GetCharacterMovement()->DisableMovement();
	SetMimicPlayerCollisionToOverlap();

	if (InteractSphere)
	{
		InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AggroSphere)
	{
		AggroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}

	if (LockOnWidgetComp)
	{
		LockOnWidgetComp->SetVisibility(false);
	}

	if (OpenedChestMesh)
	{
		GetMesh()->SetSkeletalMesh(OpenedChestMesh);
	}
	else if (ClosedChestMesh)
	{
		GetMesh()->SetSkeletalMesh(ClosedChestMesh);
	}

	if (OpenedChestAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(OpenedChestAnimClass);
	}
}

void AMimicChestMonster::ResetToInitialStateOnBonfireRest()
{
	// 죽은 미믹은 기존처럼 휴식으로 부활시키지 않음
	if (bIsDead)
	{
		return;
	}

	// 아직 그냥 상자 상태면 굳이 할 일 없음
	// 그래도 혹시 모르니 스폰 위치로만 정렬
	if (!bAwakenedAsMimic)
	{
		SetActorLocationAndRotation(
			InitialSpawnTransform.GetLocation(),
			InitialSpawnTransform.GetRotation().Rotator()
		);
		return;
	}

	// 공격/추적 상태 정리
	GetWorldTimerManager().ClearTimer(TimerHandle_OpenResolve);
	GetWorldTimerManager().ClearTimer(TimerHandle_AttackCooldown);
	GetWorldTimerManager().ClearTimer(TimerHandle_KnockDown);
	GetWorldTimerManager().ClearTimer(TimerHandle_HideHPBar);

	bIsAttacking = false;
	bAttackTraceActive = false;
	bHasDetectedTarget = false;
	bIsKnockedDown = false;
	bIsFrozenForBackAttack = false;
	bInBackAttackReaction = false;
	TargetActor = nullptr;
	AlreadyHitActors.Empty();

	bOpened = false;
	bAwakenedAsMimic = false;
	CachedInteractor = nullptr;
	bWaitingForChestTutorialClose = false;

	// AI 정리
	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->StopMovement();
		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->ClearFocus(EAIFocusPriority::Default);

		if (MonsterAI->BrainComponent)
		{
			MonsterAI->BrainComponent->RestartLogic();
		}
	}
	else if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Default);
	}

	// 원래 자리로 복귀
	SetActorLocationAndRotation(
		InitialSpawnTransform.GetLocation(),
		InitialSpawnTransform.GetRotation().Rotator()
	);

	// 상자 초기 상태로 되돌리기
	SetInitialChestState();

	if (InteractSphere)
	{
		InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

USoundBase* AMimicChestMonster::GetRandomChestSound(const TArray<USoundBase*>& Sounds) const
{
	TArray<USoundBase*> ValidSounds;

	for (USoundBase* Sound : Sounds)
	{
		if (Sound)
		{
			ValidSounds.Add(Sound);
		}
	}

	if (ValidSounds.Num() <= 0)
	{
		return nullptr;
	}

	const int32 Index = FMath::RandRange(0, ValidSounds.Num() - 1);
	return ValidSounds[Index];
}

void AMimicChestMonster::PlayChestOpenSound()
{
	USoundBase* Sound = GetRandomChestSound(ChestOpenSounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(
		ChestOpenSoundPitchMin,
		ChestOpenSoundPitchMax
	);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		ChestOpenSoundVolume,
		Pitch
	);
}

void AMimicChestMonster::PlayMimicTransformSound()
{
	USoundBase* Sound = GetRandomChestSound(MimicTransformSounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(
		MimicTransformSoundPitchMin,
		MimicTransformSoundPitchMax
	);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		MimicTransformSoundVolume,
		Pitch
	);
}

bool AMimicChestMonster::CanBeBackAttacked() const
{
	// 미믹이 실제 몬스터로 깨어난 뒤에만 백어택 가능
	if (!bAwakenedAsMimic)
	{
		return false;
	}

	// 일반 상자는 절대 백어택 불가
	if (ChestType != EChestType::Mimic)
	{
		return false;
	}

	return Super::CanBeBackAttacked();
}

bool AMimicChestMonster::CanBeLockedOn() const
{
	// 일반 상자는 절대 락온 불가
	if (ChestType != EChestType::Mimic)
	{
		return false;
	}

	// 미믹도 상자 상태일 땐 락온 불가
	if (!bAwakenedAsMimic)
	{
		return false;
	}

	return Super::CanBeLockedOn();
}

FTransform AMimicChestMonster::GetChestOpenPointTransform() const
{
	if (!ChestOpenPoint)
	{
		return ChestOpenPoint->GetComponentTransform();
	}

	const FVector FallbackLocation = GetActorLocation() + GetActorForwardVector() * 120.f;
	const FRotator FallbackRotation = (GetActorLocation() - FallbackLocation).Rotation();

	return FTransform(FallbackRotation, FallbackLocation);
}

void AMimicChestMonster::SetMimicPlayerCollisionToOverlap()
{
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	}
}

void AMimicChestMonster::EnableMimicPlayerBlockCollision()
{
	if (bIsDead || !bAwakenedAsMimic)
	{
		return;
	}

	if (!bMimicCapsuleBlocksPlayer)
	{
		SetMimicPlayerCollisionToOverlap();
		return;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	}
}