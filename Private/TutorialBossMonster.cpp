#include "TutorialBossMonster.h"
#include "MonsterAIController.h"
#include "BossPowerAttackDamageType.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Knight.h"
#include "ItemDropActor.h"
#include "FortressGameInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Camera/CameraActor.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "BossNiagaraProjectile.h"
#include "Sound/SoundBase.h"
#include "BrainComponent.h"

ATutorialBossMonster::ATutorialBossMonster()
{
	MaxHP = 100.f;
	CurrentHP = MaxHP;

	MaxGroggyGauge = 40.f;
	CurrentGroggyGauge = MaxGroggyGauge;

	AttackCooldown = 0.f;
	bRespawnOnBonfireRest = false;

	bShowBossFelledMessageOnDeath = true;
	BossFelledMessage = FText::FromString(TEXT("ENEMY FELLED"));
	BossFelledMessageDuration = 4.0f;

	GrabCameraActorComp = CreateDefaultSubobject<UChildActorComponent>(TEXT("GrabCameraActorComp"));
	GrabCameraActorComp->SetupAttachment(GetMesh());
	GrabCameraActorComp->SetChildActorClass(ACameraActor::StaticClass());
	GrabCameraActorComp->SetRelativeLocation(FVector(-300.f, 120.f, 170.f));
	GrabCameraActorComp->SetRelativeRotation(FRotator(-8.f, -25.f, 0.f));
}

void ATutorialBossMonster::BeginPlay()
{
	Super::BeginPlay();

	if (BossUniqueID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss '%s' has no BossUniqueID"), *GetName());
	}

	if (IsAlreadyDefeated())
	{
		UE_LOG(LogTemp, Warning, TEXT("Boss '%s' already defeated. Removing."), *GetName());

		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (GetMesh())
		{
			GetMesh()->SetVisibility(false, true);
			GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		if (AggroSphere)
		{
			AggroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		SetLifeSpan(0.01f);
		return;
	}

	CurrentGroggyGauge = MaxGroggyGauge;

	if (bWaitForIntroMovie)
	{
		bBossIntroFinished = false;
		bIsAttacking = false;
		bAttackTraceActive = false;
		TargetActor = nullptr;
		bHasDetectedTarget = false;
		BossAttackState = EBossAttackState::Idle;

		GetWorldTimerManager().ClearTimer(TimerHandle_NormalAttackLoop);
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerAttackLoop);
		GetWorldTimerManager().ClearTimer(TimerHandle_GrabAttackLoop);
		GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraLoop);
		GetWorldTimerManager().ClearTimer(TimerHandle_RangedNiagaraLoop);

		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();

		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->StopAllMontages(0.1f);
		}

		if (AAIController* AICon = Cast<AAIController>(GetController()))
		{
			AICon->StopMovement();
			AICon->ClearFocus(EAIFocusPriority::Default);

			if (AICon->BrainComponent)
			{
				AICon->BrainComponent->StopLogic(TEXT("Waiting for boss intro movie"));
			}
		}

		if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
		{
			MonsterAI->SetTargetActor(nullptr);
			MonsterAI->SetIsAttacking(false);
		}

		return;
	}

	StartBossAfterIntroMovie();
}

void ATutorialBossMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

float ATutorialBossMonster::TakeDamage(
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

	const float FinalDamage = FMath::Max(0.f, DamageAmount);
	CurrentHP = FMath::Clamp(CurrentHP - FinalDamage, 0.f, MaxHP);

	if (DamageCauser && DamageCauser->ActorHasTag(TEXT("Player")))
	{
		SetHasDetectedTarget(true);
		SetTargetActor(DamageCauser);

		if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
		{
			MonsterAI->SetTargetActor(DamageCauser);
			MonsterAI->EnableFullAlertSight(DamageCauser);
		}

		if (AKnight* Knight = Cast<AKnight>(DamageCauser))
		{
			LastDamagingKnight = Knight;
		}
	}

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	UpdateHPWidget();
	ShowHPBar();

	if (CurrentHP <= 0.f)
	{
		if (LastDamagingKnight.IsValid())
		{
			LastDamagingKnight->AddMemoryShards(MemoryShardReward, true);
		}

		SpawnGuaranteedBossReward();
		Die();
		return FinalDamage;
	}

	// 보스는 일반 피격 모션 없음
	// 그로기 상태일 때 일반 공격 맞으면 그로기 해제
	const bool bWasGroggyBeforeHit = bIsGroggy;

	ConsumeGroggyGauge(FinalDamage);

	if (bWasGroggyBeforeHit && bIsGroggy && !bInBackAttackReaction)
	{
		ExitGroggy();
	}

	return FinalDamage;
}

void ATutorialBossMonster::Die()
{
	if (bIsDead)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraDamageTick);
	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraDamageEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabDamageTick);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabSuccessEnd);
	ReleaseGrabbedKnight(false);
	bIsGrabAttacking = false;

	MarkBossAsDefeated();

	Super::Die();
}

void ATutorialBossMonster::ConsumeGroggyGauge(float DamageAmount)
{
	if (bIsDead || bIsGroggy)
	{
		return;
	}

	CurrentGroggyGauge = FMath::Clamp(CurrentGroggyGauge - DamageAmount, 0.f, MaxGroggyGauge);

	if (CurrentGroggyGauge <= 0.f)
	{
		EnterGroggy();
	}
}

void ATutorialBossMonster::EnterGroggy()
{
	if (bIsDead || bIsGroggy)
	{
		return;
	}

	bIsGroggy = true;
	bIsKnockedDown = true;
	BossAttackState = EBossAttackState::Groggy;
	bIsAttacking = false;
	bAttackTraceActive = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabDamageTick);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabEnd);
	ReleaseGrabbedKnight(false);
	bIsGrabAttacking = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraDamageTick);
	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraDamageEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_NormalAttackLoop);
	GetWorldTimerManager().ClearTimer(TimerHandle_PowerAttackLoop);
	GetWorldTimerManager().ClearTimer(TimerHandle_NextCombo);
	GetWorldTimerManager().ClearTimer(TimerHandle_CurrentAttackEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabSuccessEnd);

	// 그로기 중 보스가 플레이어를 향해 계속 도는 것 방지
	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(true);
		MonsterAI->StopMovement();
	}
	else if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	if (GroggyMontageAsset)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(GroggyMontageAsset);
		}
	}

	PauseAttackLoops();

	GetWorldTimerManager().SetTimer(
		TimerHandle_GroggyEnd,
		this,
		&ATutorialBossMonster::ExitGroggy,
		GroggyDuration,
		false
	);
}

void ATutorialBossMonster::ExitGroggy()
{
	if (bIsDead || !bIsGroggy)
	{
		return;
	}

	bIsGroggy = false;
	bIsKnockedDown = true; // Exit 몽타주 동안은 아직 다운 상태 유지
	bIsExitingGroggy = true;
	BossAttackState = EBossAttackState::Idle;
	CurrentGroggyGauge = MaxGroggyGauge;

	GetWorldTimerManager().ClearTimer(TimerHandle_GroggyEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_NormalAttackLoop);
	GetWorldTimerManager().ClearTimer(TimerHandle_PowerAttackLoop);
	GetWorldTimerManager().ClearTimer(TimerHandle_NextCombo);
	GetWorldTimerManager().ClearTimer(TimerHandle_CurrentAttackEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_GroggyExitEnd);

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsKnockedDown(true);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->StopMovement();
	}

	GetCharacterMovement()->DisableMovement();

	float ExitLen = 0.2f;

	if (GroggyExitMontageAsset)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(GroggyExitMontageAsset);
			ExitLen = GetMontageLength(GroggyExitMontageAsset);
		}
	}

	GetWorldTimerManager().SetTimer(
		TimerHandle_GroggyExitEnd,
		this,
		&ATutorialBossMonster::FinishGroggyExit,
		ExitLen,
		false
	);
}

bool ATutorialBossMonster::CanBeBackAttacked() const
{
	return !bIsDead && bIsGroggy;
}

bool ATutorialBossMonster::CanAttack() const
{
	if (bIsDead || bIsGroggy || bIsKnockedDown || bIsFrozenForBackAttack || bIsExitingGroggy)
	{
		return false;
	}

	if (!TargetActor)
	{
		return false;
	}

	return true;
}

bool ATutorialBossMonster::IsBusy() const
{
	return bIsDead
		|| bIsGroggy
		|| bIsKnockedDown
		|| bIsFrozenForBackAttack
		|| bIsExitingGroggy
		|| BossAttackState == EBossAttackState::NormalCombo
		|| BossAttackState == EBossAttackState::PowerAttack
		|| BossAttackState == EBossAttackState::AreaNiagara
		|| BossAttackState == EBossAttackState::RangedNiagara
		|| BossAttackState == EBossAttackState::Grab;
}

void ATutorialBossMonster::PerformAttack()
{
	// BTTask가 호출해도 보스 전용 일반콤보로 연결
	TryStartNormalCombo();
}

void ATutorialBossMonster::TryStartNormalCombo()
{
	if (bPowerAttackPending)
	{
		TryStartPowerAttack();
		return;
	}

	if (!CanAttack() || IsBusy())
	{
		return;
	}

	if (!TargetActor)
	{
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (Dist > AttackRange)
	{
		return;
	}

	BossAttackState = EBossAttackState::NormalCombo;
	bIsAttacking = true;
	CurrentComboStep = 0;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(true);
	}

	StartNextNormalComboStep();
}

void ATutorialBossMonster::StartNextNormalComboStep()
{
	if (BossAttackState != EBossAttackState::NormalCombo)
	{
		FinishNormalCombo();
		return;
	}

	if (!TargetActor)
	{
		FinishNormalCombo();
		return;
	}

	if (CurrentComboStep >= 3)
	{
		FinishNormalCombo();
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (CurrentComboStep > 0 && Dist > ComboContinueRange)
	{
		FinishNormalCombo();
		return;
	}

	if (!NormalComboMontages.IsValidIndex(CurrentComboStep) || !NormalComboMontages[CurrentComboStep])
	{
		FinishNormalCombo();
		return;
	}

	FaceTarget();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(NormalComboMontages[CurrentComboStep]);
	}

	const float MontageLen = GetMontageLength(NormalComboMontages[CurrentComboStep]);

	CurrentComboStep++;

	if (CurrentComboStep < 3)
	{
		GetWorldTimerManager().SetTimer(
			TimerHandle_NextCombo,
			this,
			&ATutorialBossMonster::StartNextNormalComboStep,
			MontageLen + NextComboDelay,
			false
		);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			TimerHandle_CurrentAttackEnd,
			this,
			&ATutorialBossMonster::FinishNormalCombo,
			MontageLen,
			false
		);
	}
}

void ATutorialBossMonster::FinishNormalCombo()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_NextCombo);
	GetWorldTimerManager().ClearTimer(TimerHandle_CurrentAttackEnd);

	bIsAttacking = false;
	CurrentComboStep = 0;
	BossAttackState = EBossAttackState::Idle;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);
	}

	// 강공 예약이 있으면 다음 일반공격보다 먼저 바로 시도
	if (bPowerAttackPending)
	{
		TryStartPowerAttack();
	}
}

void ATutorialBossMonster::TryStartPowerAttack()
{
	// 일반 공격 중이나 바쁠 때 타이머가 차면 우선 예약만 해둠
	if (IsBusy() || !TargetActor || bIsDead || bIsGroggy || bIsKnockedDown || bIsFrozenForBackAttack)
	{
		bPowerAttackPending = true;
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (Dist > AttackRange * PowerAttackRangeMultiplier)
	{
		bPowerAttackPending = true;
		return;
	}

	bPowerAttackPending = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_NextCombo);
	GetWorldTimerManager().ClearTimer(TimerHandle_CurrentAttackEnd);

	// 일반공격 타이머 초기화
	GetWorldTimerManager().ClearTimer(TimerHandle_NormalAttackLoop);
	GetWorldTimerManager().SetTimer(
		TimerHandle_NormalAttackLoop,
		this,
		&ATutorialBossMonster::TryStartNormalCombo,
		NormalAttackInterval,
		true
	);

	// 강공 시작하면 자기 루프는 끊고, 끝났을 때 다시 10초로 재시작
	GetWorldTimerManager().ClearTimer(TimerHandle_PowerAttackLoop);

	BossAttackState = EBossAttackState::PowerAttack;
	bIsAttacking = true;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(true);
	}

	FaceTarget();

	if (PowerAttackMontages.Num() <= 0)
	{
		FinishPowerAttack();
		return;
	}

	const int32 RandIndex = FMath::RandRange(0, PowerAttackMontages.Num() - 1);
	UAnimMontage* SelectedMontage = PowerAttackMontages[RandIndex];
	if (!SelectedMontage)
	{
		FinishPowerAttack();
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(SelectedMontage);
	}

	PlayBossSoundAtActor(PowerAttackStartSounds, PowerAttackSoundVolume);

	GetWorldTimerManager().SetTimer(
		TimerHandle_CurrentAttackEnd,
		this,
		&ATutorialBossMonster::FinishPowerAttack,
		GetMontageLength(SelectedMontage),
		false
	);
}

void ATutorialBossMonster::FinishPowerAttack()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_CurrentAttackEnd);

	bIsAttacking = false;
	BossAttackState = EBossAttackState::Idle;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);
	}

	// 강공 종료 후 다시 10초 뒤부터 강공 가능
	GetWorldTimerManager().ClearTimer(TimerHandle_PowerAttackLoop);
	GetWorldTimerManager().SetTimer(
		TimerHandle_PowerAttackLoop,
		this,
		&ATutorialBossMonster::TryStartPowerAttack,
		PowerAttackInterval,
		true
	);
}

void ATutorialBossMonster::ResetAttackState()
{
	// 보스는 BaseMonster의 AttackCooldown 방식 안 씀
	bIsAttacking = false;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);
	}
}

void ATutorialBossMonster::PauseAttackLoops()
{
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_NormalAttackLoop))
	{
		GetWorldTimerManager().PauseTimer(TimerHandle_NormalAttackLoop);
	}

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_PowerAttackLoop))
	{
		GetWorldTimerManager().PauseTimer(TimerHandle_PowerAttackLoop);
	}
}

void ATutorialBossMonster::ResumeAttackLoops()
{
	if (bIsDead || bIsGroggy)
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerPaused(TimerHandle_NormalAttackLoop))
	{
		GetWorldTimerManager().UnPauseTimer(TimerHandle_NormalAttackLoop);
	}

	if (GetWorldTimerManager().IsTimerPaused(TimerHandle_PowerAttackLoop))
	{
		GetWorldTimerManager().UnPauseTimer(TimerHandle_PowerAttackLoop);
	}
}

float ATutorialBossMonster::GetMontageLength(UAnimMontage* Montage) const
{
	return Montage ? Montage->GetPlayLength() : 0.f;
}

void ATutorialBossMonster::FaceTarget()
{
	if (!TargetActor)
	{
		return;
	}

	if (BossAttackState == EBossAttackState::Grab || bIsGrabAttacking)
	{
		return;
	}

	FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}
}

void ATutorialBossMonster::PerformAttackTrace()
{
	if (!GetMesh())
	{
		return;
	}

	const FVector TraceStart = GetMesh()->GetSocketLocation(AttackTraceStartSocketName);
	const FVector TraceEnd = GetMesh()->GetSocketLocation(AttackTraceEndSocketName);

	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	const bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		this,
		TraceStart,
		TraceEnd,
		AttackTraceRadius,
		UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == this)
			{
				continue;
			}

			if (!HitActor->ActorHasTag(TEXT("Player")))
			{
				continue;
			}

			if (AlreadyHitActors.Contains(HitActor))
			{
				continue;
			}

			AlreadyHitActors.Add(HitActor);

			const bool bIsPowerAttack = (BossAttackState == EBossAttackState::PowerAttack);
			const float FinalDamage = bIsPowerAttack
				? AttackPower * PowerAttackDamageMultiplier
				: AttackPower;

			TSubclassOf<UDamageType> DamageTypeClass = bIsPowerAttack
				? UBossPowerAttackDamageType::StaticClass()
				: UDamageType::StaticClass();

			const float AppliedDamage = UGameplayStatics::ApplyDamage(
				HitActor,
				FinalDamage,
				GetController(),
				this,
				DamageTypeClass
			);

			if (AppliedDamage > 0.f)
			{
				FVector HitLocation = HitActor->GetActorLocation();

				if (!Hit.ImpactPoint.IsNearlyZero())
				{
					HitLocation = FVector(Hit.ImpactPoint);
				}

				FRotator HitRotation = GetActorRotation();

				if (!Hit.ImpactNormal.IsNearlyZero())
				{
					HitRotation = Hit.ImpactNormal.Rotation();
				}

				if (BossAttackState == EBossAttackState::PowerAttack)
				{
					PlayBossHitSuccessSoundAtLocation(
						PowerAttackHitSuccessSounds,
						HitLocation,
						BossHitSuccessVolume
					);
				}
				else
				{
					PlayBossHitSuccessSoundAtLocation(
						NormalAttackHitSuccessSounds,
						HitLocation,
						BossHitSuccessVolume
					);
				}

				SpawnAttackHitBloodEffect(HitLocation, HitRotation);
			}
		}
	}
}

void ATutorialBossMonster::PlayHitReaction_Implementation()
{
	// 보스는 피격 모션 없음
}

void ATutorialBossMonster::SpawnGuaranteedBossReward()
{
	if (!BossRewardDropClass)
	{
		return;
	}

	int32 SpawnIndex = 0;
	float CurrentYOffset = 0.f;
	const float YStepUnit = 40.f; // 1칸당 실제 월드 거리

	for (const FBossGuaranteedRewardEntry& RewardEntry : GuaranteedRewards)
	{
		if (RewardEntry.ItemData.ItemID.IsNone() || RewardEntry.Count <= 0)
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		const float XStep = 20.f;

		if (SpawnIndex > 0)
		{
			CurrentYOffset += SpawnIndex * YStepUnit;
		}

		const FVector SpawnOffset = FVector(SpawnIndex * XStep, CurrentYOffset, 0.f);

		AItemDropActor* DropActor = GetWorld()->SpawnActor<AItemDropActor>(
			BossRewardDropClass,
			GetActorLocation() + SpawnOffset,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (DropActor)
		{
			FInventoryItem DropItem;
			DropItem.ItemData = RewardEntry.ItemData;
			DropItem.Count = RewardEntry.Count;

			DropActor->InitializeDrop(DropItem);
		}

		SpawnIndex++;
	}
}

bool ATutorialBossMonster::IsAlreadyDefeated() const
{
	const UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return false;
	}

	return GI->IsBossDefeated(BossUniqueID);
}

void ATutorialBossMonster::MarkBossAsDefeated()
{
	UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	GI->MarkBossDefeated(BossUniqueID);
}

void ATutorialBossMonster::FinishGroggyFromBackAttack()
{
	if (bIsDead)
	{
		return;
	}

	SetBackAttackReaction(false);

	if (bIsGroggy)
	{
		ExitGroggy();
	}
	else
	{
		UnfreezeFromBackAttack();
	}
}

void ATutorialBossMonster::FinishGroggyExit()
{
	if (bIsDead)
	{
		return;
	}

	bIsExitingGroggy = false;
	bIsKnockedDown = false;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->SetIsAttacking(false);
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	UnfreezeFromBackAttack();

	if (!TargetActor)
	{
		AActor* PlayerActor = UGameplayStatics::GetPlayerCharacter(this, 0);

		if (PlayerActor && PlayerActor->ActorHasTag(TEXT("Player")))
		{
			SetHasDetectedTarget(true);
			SetTargetActor(PlayerActor);

			if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
			{
				MonsterAI->SetTargetActor(PlayerActor);
				MonsterAI->EnableFullAlertSight(PlayerActor);
				MonsterAI->SetFocus(PlayerActor);
				MonsterAI->StopMovement();
			}
		}
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_NormalAttackLoop);
	GetWorldTimerManager().SetTimer(
		TimerHandle_NormalAttackLoop,
		this,
		&ATutorialBossMonster::TryStartNormalCombo,
		NormalAttackInterval,
		true
	);

	GetWorldTimerManager().ClearTimer(TimerHandle_PowerAttackLoop);
	GetWorldTimerManager().SetTimer(
		TimerHandle_PowerAttackLoop,
		this,
		&ATutorialBossMonster::TryStartPowerAttack,
		PowerAttackInterval,
		true
	);

	ResumeAttackLoops();
}

void ATutorialBossMonster::TryStartGrabAttack()
{
	if (IsBusy() || !CanAttack() || !TargetActor)
	{
		return;
	}

	AKnight* KnightTarget = Cast<AKnight>(TargetActor);
	if (!KnightTarget || KnightTarget->IsBossKnockedDown())
	{
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (Dist > GrabAttackRange)
	{
		return;
	}

	if (GrabAttackMontages.Num() <= 0)
	{
		return;
	}

	TArray<UAnimMontage*> ValidMontages;
	for (UAnimMontage* Montage : GrabAttackMontages)
	{
		if (Montage)
		{
			ValidMontages.Add(Montage);
		}
	}

	if (ValidMontages.Num() <= 0)
	{
		return;
	}

	if (TargetActor)
	{
		FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
		}
	}

	BossAttackState = EBossAttackState::Grab;
	bIsAttacking = true;
	bIsGrabAttacking = true;
	bGrabSucceeded = false;
	GrabbedKnight = nullptr;

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->ClearFocus(EAIFocusPriority::Default);
		AICon->StopMovement();
	}

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(true);
	}

	// FaceTarget();

	const int32 RandIndex = FMath::RandRange(0, ValidMontages.Num() - 1);
	UAnimMontage* SelectedMontage = ValidMontages[RandIndex];

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(SelectedMontage);
	}

	PlayBossSoundAtSocket(GrabAttackStartSounds, GrabCheckSocketName, GrabSoundVolume);

	const float GrabAttackMontageLength = GetMontageLength(SelectedMontage);

	GetWorldTimerManager().SetTimer(
		TimerHandle_GrabEnd,
		this,
		&ATutorialBossMonster::FinishGrabAttack,
		GrabAttackMontageLength,
		false
	);
}

void ATutorialBossMonster::TryGrabPlayer()
{
	if (GrabbedKnight)
	{
		return;
	}

	if (!bIsGrabAttacking || bIsDead || !TargetActor || GrabbedKnight)
	{
		return;
	}

	AKnight* KnightTarget = Cast<AKnight>(TargetActor);
	if (!KnightTarget || KnightTarget->IsBossKnockedDown())
	{
		return;
	}

	// 구르기 무적 중이면 그랩 회피
	if (KnightTarget->IsInvincible())
	{
		return;
	}

	if (!GetMesh())
	{
		return;
	}

	const FVector GrabOrigin = GetMesh()->GetSocketLocation(GrabCheckSocketName);
	const FVector KnightLoc = KnightTarget->GetActorLocation();

	const float DistToHand = FVector::Dist(GrabOrigin, KnightLoc);
	if (DistToHand > GrabCheckRadius)
	{
		return;
	}

	GrabbedKnight = KnightTarget;
	AttachKnightToHand(GrabbedKnight);
	bGrabSucceeded = true;
	PlayBossSoundAtSocket(GrabSuccessSounds, GrabSocketName, GrabSoundVolume);
	PlayGrabSuccessMontage();

	GetWorldTimerManager().ClearTimer(TimerHandle_GrabDamageTick);
	GetWorldTimerManager().SetTimer(
		TimerHandle_GrabDamageTick,
		this,
		&ATutorialBossMonster::ApplyGrabDamageTick,
		GrabDamageTickInterval,
		true
	);
}

void ATutorialBossMonster::ApplyGrabDamageTick()
{
	if (!GrabbedKnight || bIsDead)
	{
		return;
	}

	UGameplayStatics::ApplyDamage(
		GrabbedKnight,
		GrabDamagePerTick,
		GetController(),
		this,
		UDamageType::StaticClass()
	);
}

void ATutorialBossMonster::PlayGrabSuccessMontage()
{
	if (GrabSuccessMontages.Num() <= 0)
	{
		return;
	}

	TArray<UAnimMontage*> ValidMontages;
	for (UAnimMontage* Montage : GrabSuccessMontages)
	{
		if (Montage)
		{
			ValidMontages.Add(Montage);
		}
	}

	if (ValidMontages.Num() <= 0)
	{
		return;
	}

	const int32 RandIndex = FMath::RandRange(0, ValidMontages.Num() - 1);
	UAnimMontage* SelectedMontage = ValidMontages[RandIndex];
	if (!SelectedMontage)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(SelectedMontage);
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_GrabEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabSuccessEnd);

	const float SuccessMontageLength = GetMontageLength(SelectedMontage);
	const float FinalGrabSuccessDuration = FMath::Max(SuccessMontageLength, GrabDuration);

	GetWorldTimerManager().SetTimer(
		TimerHandle_GrabSuccessEnd,
		this,
		&ATutorialBossMonster::FinishGrabSuccessAttack,
		FinalGrabSuccessDuration,
		false
	);
}

void ATutorialBossMonster::FinishGrabSuccessAttack()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabDamageTick);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabSuccessEnd);

	ReleaseGrabbedKnight(false);

	bGrabSucceeded = false;
	bIsGrabAttacking = false;
	bIsAttacking = false;
	BossAttackState = EBossAttackState::Idle;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);

		if (TargetActor)
		{
			MonsterAI->SetFocus(TargetActor);
		}
	}

	GrabbedKnight = nullptr;
}

void ATutorialBossMonster::FinishGrabAttack()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabDamageTick);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabEnd);

	// 이미 성공해서 성공 몽타주로 넘어갔으면 여기서 바로 해제하지 않음
	if (bGrabSucceeded)
	{
		return;
	}

	ReleaseGrabbedKnight(false);

	bIsGrabAttacking = false;
	bIsAttacking = false;
	BossAttackState = EBossAttackState::Idle;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);

		if (TargetActor)
		{
			MonsterAI->SetFocus(TargetActor);
		}
	}

	GrabbedKnight = nullptr;
}

void ATutorialBossMonster::ReleaseGrabbedKnight(bool bKnockDownPlayer)
{
	if (!GrabbedKnight)
	{
		return;
	}

	AKnight* Knight = GrabbedKnight;
	GrabbedKnight = nullptr;

	Knight->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Knight->SetGrabbedByBoss(false);
	RestoreGrabbedPlayerCamera(Knight);

	const FVector ReleaseLocation = GetActorLocation() + GetActorForwardVector() * 120.f;
	Knight->SetActorLocation(ReleaseLocation);

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}

	Knight->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (bKnockDownPlayer)
	{
		Knight->EnterBossKnockDown(this);
	}
}

void ATutorialBossMonster::StartGrabCheck()
{
	TryGrabPlayer();
}

void ATutorialBossMonster::AttachKnightToHand(AKnight* Knight)
{
	if (!Knight || !GetMesh())
	{
		return;
	}

	Knight->SetGrabbedByBoss(true);
	Knight->GetCharacterMovement()->DisableMovement();

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	Knight->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		GrabSocketName
	);

	if (USceneComponent* KnightRoot = Knight->GetRootComponent())
	{
		KnightRoot->SetRelativeLocationAndRotation(
			GrabbedPlayerRelativeLocation,
			GrabbedPlayerRelativeRotation
		);
	}

	SwitchToGrabCamera(Knight);
}

void ATutorialBossMonster::SwitchToGrabCamera(AKnight* Knight)
{
	if (!Knight || !GrabCameraActorComp)
	{
		return;
	}

	AActor* GrabCamActor = GrabCameraActorComp->GetChildActor();
	if (!GrabCamActor)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->bAutoManageActiveCameraTarget = false;
		PC->SetViewTargetWithBlend(GrabCamActor, GrabCameraBlendTime);
	}
}

void ATutorialBossMonster::RestoreGrabbedPlayerCamera(AKnight* Knight)
{
	if (!Knight)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
	{
		PC->SetViewTargetWithBlend(Knight, GrabCameraBlendTime);
		PC->bAutoManageActiveCameraTarget = true;
	}
}

void ATutorialBossMonster::TryStartAreaNiagaraAttack()
{
	if (IsBusy() || !CanAttack() || !TargetActor)
	{
		return;
	}

	if (AreaNiagaraMontages.Num() <= 0)
	{
		return;
	}

	TArray<UAnimMontage*> ValidMontages;
	for (UAnimMontage* Montage : AreaNiagaraMontages)
	{
		if (Montage)
		{
			ValidMontages.Add(Montage);
		}
	}

	if (ValidMontages.Num() <= 0)
	{
		return;
	}

	BossAttackState = EBossAttackState::AreaNiagara;
	bIsAttacking = true;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(true);
		MonsterAI->StopMovement();
	}

	FaceTarget();

	const int32 RandIndex = FMath::RandRange(0, ValidMontages.Num() - 1);
	UAnimMontage* SelectedMontage = ValidMontages[RandIndex];

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(SelectedMontage);
	}

	PlayBossSoundAtActor(AreaNiagaraStartSounds, AreaNiagaraSoundVolume);

	GetWorldTimerManager().SetTimer(
		TimerHandle_SpecialAttackEnd,
		this,
		&ATutorialBossMonster::FinishSpecialAttack,
		GetMontageLength(SelectedMontage),
		false
	);
}

void ATutorialBossMonster::ActivateAreaNiagaraAttack()
{
	if (bIsDead || !GetMesh())
	{
		return;
	}

	ActiveAreaNiagaraLocation = GetMesh()->GetSocketLocation(AreaNiagaraSpawnSocket);

	PlayBossSoundAtLocation(
		AreaNiagaraActivateSounds,
		ActiveAreaNiagaraLocation,
		AreaNiagaraSoundVolume
	);

	if (AreaNiagaraEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			AreaNiagaraEffect,
			ActiveAreaNiagaraLocation,
			FRotator::ZeroRotator
		);
	}

	// 바로 1틱
	ApplyAreaNiagaraDamageTick();

	// 1초마다 반복 데미지
	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraDamageTick);
	GetWorldTimerManager().SetTimer(
		TimerHandle_AreaNiagaraDamageTick,
		this,
		&ATutorialBossMonster::ApplyAreaNiagaraDamageTick,
		AreaNiagaraDamageTickInterval,
		true
	);

	// 일정 시간 후 종료
	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraDamageEnd);
	GetWorldTimerManager().SetTimer(
		TimerHandle_AreaNiagaraDamageEnd,
		this,
		&ATutorialBossMonster::EndAreaNiagaraDamage,
		AreaNiagaraDuration,
		false
	);
}

void ATutorialBossMonster::ApplyAreaNiagaraDamageTick()
{
	if (bIsDead)
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKnight::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		AKnight* Knight = Cast<AKnight>(Actor);
		if (!Knight || !Knight->ActorHasTag(TEXT("Player")))
		{
			continue;
		}

		const float Dist = FVector::Dist(ActiveAreaNiagaraLocation, Knight->GetActorLocation());
		if (Dist > AreaNiagaraRadius)
		{
			continue;
		}

		const float AppliedDamage = UGameplayStatics::ApplyDamage(
			Knight,
			AreaNiagaraDamage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);

		if (AppliedDamage > 0.f)
		{
			PlayBossHitSuccessSoundAtLocation(
				AreaNiagaraHitSuccessSounds,
				Knight->GetActorLocation(),
				BossHitSuccessVolume
			);
		}
	}
}

void ATutorialBossMonster::EndAreaNiagaraDamage()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraDamageTick);
	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraDamageEnd);
}

void ATutorialBossMonster::TryStartRangedNiagaraAttack()
{
	if (IsBusy() || bIsDead || !TargetActor || !RangedProjectileClass)
	{
		return;
	}

	if (RangedNiagaraMontages.Num() <= 0)
	{
		return;
	}

	TArray<UAnimMontage*> ValidMontages;
	for (UAnimMontage* Montage : RangedNiagaraMontages)
	{
		if (Montage)
		{
			ValidMontages.Add(Montage);
		}
	}

	if (ValidMontages.Num() <= 0)
	{
		return;
	}

	BossAttackState = EBossAttackState::RangedNiagara;
	bIsAttacking = true;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(true);
		MonsterAI->StopMovement();
	}

	FaceTarget();

	const int32 RandIndex = FMath::RandRange(0, ValidMontages.Num() - 1);
	UAnimMontage* SelectedMontage = ValidMontages[RandIndex];

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(SelectedMontage);
	}

	PlayBossSoundAtActor(RangedNiagaraStartSounds, RangedNiagaraSoundVolume);

	GetWorldTimerManager().SetTimer(
		TimerHandle_SpecialAttackEnd,
		this,
		&ATutorialBossMonster::FinishSpecialAttack,
		GetMontageLength(SelectedMontage),
		false
	);
}

void ATutorialBossMonster::FireRangedNiagaraProjectile()
{
	if (bIsDead || !GetMesh() || !RangedProjectileClass)
	{
		return;
	}

	const FVector SpawnLoc = GetMesh()->GetSocketLocation(RangedNiagaraSpawnSocket);
	FRotator SpawnRot = GetActorRotation();

	PlayBossSoundAtLocation(
		RangedNiagaraFireSounds,
		SpawnLoc,
		RangedNiagaraSoundVolume
	);

	if (TargetActor)
	{
		FVector ToTarget = TargetActor->GetActorLocation() - SpawnLoc;
		SpawnRot = ToTarget.Rotation();
	}

	if (RangedNiagaraMuzzleEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			RangedNiagaraMuzzleEffect,
			SpawnLoc,
			SpawnRot
		);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	ABossNiagaraProjectile* Projectile = GetWorld()->SpawnActor<ABossNiagaraProjectile>(
		RangedProjectileClass,
		SpawnLoc,
		SpawnRot,
		SpawnParams
	);

	if (Projectile)
	{
		Projectile->SetProjectileDamage(RangedNiagaraDamage);

		Projectile->SetHitSuccessSounds(
			RangedNiagaraHitSuccessSounds,
			BossHitSuccessVolume,
			BossAttackSoundPitchMin,
			BossAttackSoundPitchMax
		);
	}
}

void ATutorialBossMonster::FinishSpecialAttack()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_SpecialAttackEnd);

	bIsAttacking = false;
	BossAttackState = EBossAttackState::Idle;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);
	}
}

USoundBase* ATutorialBossMonster::GetRandomBossSound(const TArray<USoundBase*>& Sounds) const
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

void ATutorialBossMonster::PlayBossSoundAtActor(const TArray<USoundBase*>& Sounds, float Volume)
{
	USoundBase* Sound = GetRandomBossSound(Sounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(BossAttackSoundPitchMin, BossAttackSoundPitchMax);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		Volume,
		Pitch
	);
}

void ATutorialBossMonster::PlayBossSoundAtSocket(const TArray<USoundBase*>& Sounds, FName SocketName, float Volume)
{
	USoundBase* Sound = GetRandomBossSound(Sounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(BossAttackSoundPitchMin, BossAttackSoundPitchMax);

	if (GetMesh())
	{
		UGameplayStatics::SpawnSoundAttached(
			Sound,
			GetMesh(),
			SocketName,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget,
			true,
			Volume,
			Pitch
		);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Sound,
			GetActorLocation(),
			Volume,
			Pitch
		);
	}
}

void ATutorialBossMonster::PlayBossSoundAtLocation(const TArray<USoundBase*>& Sounds, const FVector& Location, float Volume)
{
	USoundBase* Sound = GetRandomBossSound(Sounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(BossAttackSoundPitchMin, BossAttackSoundPitchMax);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		Location,
		Volume,
		Pitch
	);
}

void ATutorialBossMonster::PlayBossHitSuccessSoundAtLocation(
	const TArray<USoundBase*>& Sounds,
	const FVector& Location,
	float Volume
)
{
	PlayBossSoundAtLocation(Sounds, Location, Volume);
}

void ATutorialBossMonster::StartBossAttackLoops()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_NormalAttackLoop);
	GetWorldTimerManager().ClearTimer(TimerHandle_PowerAttackLoop);
	GetWorldTimerManager().ClearTimer(TimerHandle_GrabAttackLoop);
	GetWorldTimerManager().ClearTimer(TimerHandle_AreaNiagaraLoop);
	GetWorldTimerManager().ClearTimer(TimerHandle_RangedNiagaraLoop);

	GetWorldTimerManager().SetTimer(
		TimerHandle_NormalAttackLoop,
		this,
		&ATutorialBossMonster::TryStartNormalCombo,
		NormalAttackInterval,
		true
	);

	GetWorldTimerManager().SetTimer(
		TimerHandle_PowerAttackLoop,
		this,
		&ATutorialBossMonster::TryStartPowerAttack,
		PowerAttackInterval,
		true
	);

	GetWorldTimerManager().SetTimer(
		TimerHandle_GrabAttackLoop,
		this,
		&ATutorialBossMonster::TryStartGrabAttack,
		GrabAttackInterval,
		true
	);

	GetWorldTimerManager().SetTimer(
		TimerHandle_AreaNiagaraLoop,
		this,
		&ATutorialBossMonster::TryStartAreaNiagaraAttack,
		AreaNiagaraInterval,
		true
	);

	GetWorldTimerManager().SetTimer(
		TimerHandle_RangedNiagaraLoop,
		this,
		&ATutorialBossMonster::TryStartRangedNiagaraAttack,
		RangedNiagaraInterval,
		true
	);
}

void ATutorialBossMonster::StartBossAfterIntroMovie()
{
	if (bIsDead || bBossIntroFinished)
	{
		return;
	}

	bBossIntroFinished = true;

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Default);

		if (AICon->BrainComponent)
		{
			AICon->BrainComponent->RestartLogic();
		}
	}

	AActor* PlayerActor = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (PlayerActor && PlayerActor->ActorHasTag(TEXT("Player")))
	{
		SetHasDetectedTarget(true);
		SetTargetActor(PlayerActor);

		if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
		{
			MonsterAI->SetTargetActor(PlayerActor);
			MonsterAI->EnableFullAlertSight(PlayerActor);
		}
	}

	StartBossAttackLoops();
}

bool ATutorialBossMonster::IsWaitingForIntroMovie() const
{
	return bWaitForIntroMovie && !bBossIntroFinished;
}