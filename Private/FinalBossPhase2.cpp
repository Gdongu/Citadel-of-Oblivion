#include "FinalBossPhase2.h"
#include "BossNiagaraProjectile.h"
#include "MonsterAIController.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"

AFinalBossPhase2::AFinalBossPhase2()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFinalBossPhase2::BeginPlay()
{
	Super::BeginPlay();

	AttackRange = MeleeAttackRange;

	GetWorldTimerManager().SetTimer(
		TimerHandle_AutoTeleport,
		this,
		&AFinalBossPhase2::AutoTeleport,
		AutoTeleportInterval,
		true
	);

	GetWorldTimerManager().SetTimer(
		TimerHandle_MultiRangedNiagaraLoop,
		this,
		&AFinalBossPhase2::TryStartMultiRangedNiagaraAttack,
		MultiRangedNiagaraInterval,
		true
	);

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
}

void AFinalBossPhase2::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasPendingCloseRangeAttack() && !IsTargetInCloseRange())
	{
		MoveTowardTargetForMelee();
	}

	if (HasPendingCloseRangeAttack() && IsTargetInCloseRange() && !IsBusy())
	{
		if (bPowerAttackPending)
		{
			TryStartPowerAttack();
		}
		else if (bAreaAttackPending)
		{
			TryStartAreaNiagaraAttack();
		}
		else if (bGrabAttackPending)
		{
			TryStartGrabAttack();
		}
		else if (bNormalComboPending)
		{
			TryStartNormalCombo();
		}

		return;
	}

	if (IsBusy() || bIsAttacking)
	{
		return;
	}

	TryTeleportAwayFromPlayer();
}

void AFinalBossPhase2::FireMultiRangedProjectiles()
{
	if (bIsDead || !GetMesh() || !MultiRangedProjectileClass)
	{
		return;
	}

	TArray<FName> ValidSockets;
	for (const FName& SocketName : MultiProjectileSocketNames)
	{
		if (!SocketName.IsNone() && GetMesh()->DoesSocketExist(SocketName))
		{
			ValidSockets.Add(SocketName);
		}
	}

	// 소켓 배열이 비어 있으면 기존 단일 소켓 fallback
	if (ValidSockets.Num() <= 0)
	{
		if (!RangedNiagaraSpawnSocket.IsNone() && GetMesh()->DoesSocketExist(RangedNiagaraSpawnSocket))
		{
			ValidSockets.Add(RangedNiagaraSpawnSocket);
		}
	}

	if (ValidSockets.Num() <= 0)
	{
		return;
	}

	FRotator BaseRot = GetActorRotation();

	if (TargetActor)
	{
		const FVector BaseOrigin = GetActorLocation();
		BaseRot = (TargetActor->GetActorLocation() - BaseOrigin).Rotation();
	}

	int32 SpawnCount = MultiProjectileCount;
	if (bUseSocketCountAsProjectileCount)
	{
		SpawnCount = ValidSockets.Num();
	}

	SpawnCount = FMath::Clamp(SpawnCount, 1, ValidSockets.Num());

	const int32 Count = SpawnCount;
	const float CenterOffset = (Count - 1) * 0.5f;

	const FVector FirstFireSoundLocation =
		GetMesh()->GetSocketLocation(ValidSockets[0]) +
		FVector(0.f, 0.f, MultiProjectileSpawnHeightOffset);

	PlayBossSoundAtLocation(
		MultiRangedNiagaraFireSounds,
		FirstFireSoundLocation,
		MultiRangedNiagaraSoundVolume
	);

	for (int32 i = 0; i < Count; ++i)
	{
		const FName SocketName = ValidSockets[i];
		const FVector SpawnLoc =
			GetMesh()->GetSocketLocation(SocketName) +
			FVector(0.f, 0.f, MultiProjectileSpawnHeightOffset);

		const float OffsetIndex = static_cast<float>(i) - CenterOffset;
		const float YawOffset = OffsetIndex * MultiProjectileAngleStep;

		FRotator SpawnRot = BaseRot;
		SpawnRot.Yaw += YawOffset;

		if (TargetActor)
		{
			const FVector ToTarget = TargetActor->GetActorLocation() - SpawnLoc;
			SpawnRot = ToTarget.Rotation();
			SpawnRot.Yaw += YawOffset;
		}

		if (MultiRangedNiagaraMuzzleEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				MultiRangedNiagaraMuzzleEffect,
				SpawnLoc,
				SpawnRot
			);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;

		ABossNiagaraProjectile* Projectile = GetWorld()->SpawnActor<ABossNiagaraProjectile>(
			MultiRangedProjectileClass,
			SpawnLoc,
			SpawnRot,
			SpawnParams
		);

		if (Projectile)
		{
			Projectile->SetProjectileDamage(RangedNiagaraDamage);
			Projectile->SetHomingTarget(TargetActor);
			Projectile->SetAllowDodgeDuringPlayerHitReaction(true); // 2페이즈 다중 projectile은 피격 중에도 구르기 가능

			Projectile->SetHitSuccessSounds(
				RangedNiagaraHitSuccessSounds,
				BossHitSuccessVolume,
				BossAttackSoundPitchMin,
				BossAttackSoundPitchMax
			);
		}
	}
}

void AFinalBossPhase2::TryTeleportAwayFromPlayer()
{
	if (!CanPhase2Teleport())
	{
		return;
	}

	// 근접 공격 예약 중이면 텔포하지 말고 걸어서 접근
	if (HasPendingCloseRangeAttack())
	{
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	if (Dist > TeleportIfCloseDistance)
	{
		return;
	}

	FVector FinalLocation;

	if (HasPendingRangedAttack())
	{
		if (!TryFindFarTeleportLocation(FinalLocation))
		{
			return;
		}
	}
	else
	{
		if (!TryFindFarTeleportLocation(FinalLocation))
		{
			return;
		}
	}

	DoTeleportTo(FinalLocation);
}

void AFinalBossPhase2::AutoTeleport()
{
	if (!CanPhase2Teleport())
	{
		return;
	}

	// 근접 공격 예약 중이면 자동 텔포하지 말고 걸어서 접근
	if (HasPendingCloseRangeAttack())
	{
		MoveTowardTargetForMelee();
		return;
	}

	FVector FinalLocation;

	if (HasPendingRangedAttack())
	{
		if (!TryFindFarTeleportLocation(FinalLocation))
		{
			return;
		}
	}
	else
	{
		FVector TeleportDir = GetActorRightVector();

		if (bUseRandomSideTeleportOnAuto && FMath::RandBool())
		{
			TeleportDir *= -1.f;
		}

		TeleportDir.Z = 0.f;
		TeleportDir.Normalize();

		const FVector DesiredLocation = GetActorLocation() + TeleportDir * TeleportDistance;

		if (!FindTeleportLocation(DesiredLocation, FinalLocation))
		{
			return;
		}
	}

	DoTeleportTo(FinalLocation);
}

void AFinalBossPhase2::ResetTeleportCooldown()
{
	bCanTeleport = true;
}

bool AFinalBossPhase2::CanPhase2Teleport() const
{
	if (!bCanTeleport || bIsDead || !TargetActor)
	{
		return false;
	}

	if (bIsGroggy || bIsKnockedDown || bIsFrozenForBackAttack)
	{
		return false;
	}

	// 근접/원거리/잡기/장판 등 공격 몽타주 중에는 절대 텔포 금지
	if (IsBusy() || bIsAttacking)
	{
		return false;
	}

	return true;
}

bool AFinalBossPhase2::FindTeleportLocation(const FVector& DesiredLocation, FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		OutLocation = DesiredLocation;
		return true;
	}

	FNavLocation NavLoc;
	const bool bFound = NavSys->ProjectPointToNavigation(
		DesiredLocation,
		NavLoc,
		FVector(200.f, 200.f, 300.f)
	);

	if (!bFound)
	{
		return false;
	}

	OutLocation = NavLoc.Location;
	return true;
}

void AFinalBossPhase2::DoTeleportTo(const FVector& NewLocation)
{
	if (!CanPhase2Teleport())
	{
		return;
	}

	const FVector StartLoc = GetActorLocation();

	if (TeleportStartEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			TeleportStartEffect,
			StartLoc,
			GetActorRotation()
		);
	}
	
	// 사라지는 순간 사운드
	PlayBossSoundAtLocation(
		TeleportStartSounds,
		StartLoc,
		TeleportSoundVolume
	);

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
	}

	SetActorLocation(NewLocation);

	if (TargetActor)
	{
		FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
		}
	}

	if (TeleportEndEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			TeleportEndEffect,
			NewLocation,
			GetActorRotation()
		);
	}

	// 나타나는 순간 사운드
	PlayBossSoundAtLocation(
		TeleportEndSounds,
		NewLocation,
		TeleportSoundVolume
	);

	bCanTeleport = false;
	GetWorldTimerManager().SetTimer(
		TimerHandle_TeleportCooldown,
		this,
		&AFinalBossPhase2::ResetTeleportCooldown,
		TeleportCooldown,
		false
	);

	if (HasPendingRangedAttack())
	{
		if (bMultiRangedAttackPending)
		{
			TryStartMultiRangedNiagaraAttack();
		}
		else if (bRangedAttackPending)
		{
			TryStartRangedNiagaraAttack();
		}
	}
}

void AFinalBossPhase2::TryStartMultiRangedNiagaraAttack()
{
	if (!TargetActor)
	{
		return;
	}

	if (!IsTargetInRangedRange())
	{
		bMultiRangedAttackPending = true;
		ClearCloseRangePendings();
		return;
	}

	if (IsBusy() || bIsDead || !MultiRangedProjectileClass)
	{
		return;
	}

	if (MultiRangedNiagaraMontages.Num() <= 0)
	{
		return;
	}

	TArray<UAnimMontage*> ValidMontages;
	for (UAnimMontage* Montage : MultiRangedNiagaraMontages)
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

	bMultiRangedAttackPending = false;
	ClearCloseRangePendings();
	bRangedAttackPending = false;

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

	GetWorldTimerManager().SetTimer(
		TimerHandle_SpecialAttackEnd,
		this,
		&ATutorialBossMonster::FinishSpecialAttack,
		GetMontageLength(SelectedMontage),
		false
	);
}

bool AFinalBossPhase2::IsTargetInCloseRange() const
{
	if (!TargetActor)
	{
		return false;
	}

	const float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	return Dist <= MeleeAttackRange;
}

bool AFinalBossPhase2::IsTargetInRangedRange() const
{
	if (!TargetActor)
	{
		return false;
	}

	const float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	return Dist > RangedAttackMinRange && Dist <= RangedAttackMaxRange;
}

void AFinalBossPhase2::TryStartNormalCombo()
{
	if (!TargetActor)
	{
		return;
	}

	if (!IsTargetInCloseRange())
	{
		bNormalComboPending = true;
		ClearRangedPendings();

		MoveTowardTargetForMelee();
		return;
	}

	bNormalComboPending = false;
	ClearRangedPendings();

	Super::TryStartNormalCombo();
}

void AFinalBossPhase2::TryStartPowerAttack()
{
	if (!TargetActor)
	{
		return;
	}

	if (!IsTargetInCloseRange())
	{
		bPowerAttackPending = true;
		ClearRangedPendings();

		MoveTowardTargetForMelee();
		return;
	}

	bPowerAttackPending = false;
	ClearRangedPendings();

	Super::TryStartPowerAttack();
}

void AFinalBossPhase2::TryStartAreaNiagaraAttack()
{
	if (!TargetActor)
	{
		return;
	}

	if (!IsTargetInCloseRange())
	{
		bAreaAttackPending = true;
		ClearRangedPendings();
		return;
	}

	bAreaAttackPending = false;
	ClearRangedPendings();
	Super::TryStartAreaNiagaraAttack();
}

void AFinalBossPhase2::TryStartRangedNiagaraAttack()
{
	if (!TargetActor)
	{
		return;
	}

	if (!IsTargetInRangedRange())
	{
		bRangedAttackPending = true;
		ClearCloseRangePendings();
		return;
	}

	bRangedAttackPending = false;
	ClearCloseRangePendings();
	Super::TryStartRangedNiagaraAttack();
}

bool AFinalBossPhase2::HasPendingCloseRangeAttack() const
{
	return bNormalComboPending
		|| bPowerAttackPending
		|| bAreaAttackPending
		|| bGrabAttackPending;
}

bool AFinalBossPhase2::HasPendingRangedAttack() const
{
	return bRangedAttackPending || bMultiRangedAttackPending;
}

bool AFinalBossPhase2::TryFindFarTeleportLocation(FVector& OutLocation) const
{
	if (!TargetActor)
	{
		return false;
	}

	FVector AwayDir = GetActorLocation() - TargetActor->GetActorLocation();
	AwayDir.Z = 0.f;
	AwayDir = AwayDir.GetSafeNormal();

	if (AwayDir.IsNearlyZero())
	{
		AwayDir = -GetActorForwardVector();
	}

	const FVector DesiredLocation = GetActorLocation() + AwayDir * TeleportDistance;
	return FindTeleportLocation(DesiredLocation, OutLocation);
}

bool AFinalBossPhase2::TryFindEngageTeleportLocation(FVector& OutLocation) const
{
	if (!TargetActor)
	{
		return false;
	}

	FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	ToTarget = ToTarget.GetSafeNormal();

	if (ToTarget.IsNearlyZero())
	{
		ToTarget = GetActorForwardVector();
	}

	const FVector DesiredLocation =
		TargetActor->GetActorLocation() - ToTarget * EngageTeleportDistanceFromPlayer;

	return FindTeleportLocation(DesiredLocation, OutLocation);
}

void AFinalBossPhase2::ClearCloseRangePendings()
{
	bNormalComboPending = false;
	bPowerAttackPending = false;
	bAreaAttackPending = false;
	bGrabAttackPending = false;
}

void AFinalBossPhase2::ClearRangedPendings()
{
	bRangedAttackPending = false;
	bMultiRangedAttackPending = false;
}

void AFinalBossPhase2::TryStartGrabAttack()
{
	if (!TargetActor)
	{
		return;
	}

	if (!IsTargetInCloseRange())
	{
		bGrabAttackPending = true;
		ClearRangedPendings();

		MoveTowardTargetForMelee();
		return;
	}

	bGrabAttackPending = false;
	ClearRangedPendings();

	Super::TryStartGrabAttack();
}

void AFinalBossPhase2::MoveTowardTargetForMelee()
{
	if (!TargetActor || bIsDead)
	{
		return;
	}

	if (IsBusy() || bIsGroggy || bIsKnockedDown || bIsFrozenForBackAttack)
	{
		return;
	}

	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		return;
	}

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(AICon))
	{
		MonsterAI->SetTargetActor(TargetActor);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	AICon->MoveToActor(
		TargetActor,
		MeleeMoveAcceptanceRadius,
		true,
		true,
		true,
		nullptr,
		true
	);
}