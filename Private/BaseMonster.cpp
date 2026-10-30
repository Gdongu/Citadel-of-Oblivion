#include "BaseMonster.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AIController.h"
#include "Blueprint/UserWidget.h"
#include "MonsterAIController.h"
#include "Animation/AnimInstance.h"
#include "MonsterHPBarWidget.h"
#include "Components/BoxComponent.h"
#include "Knight.h"
#include "ItemDropActor.h"
#include "Components/CapsuleComponent.h"
#include "BrainComponent.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "StageBGMManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Navigation/PathFollowingComponent.h"

ABaseMonster::ABaseMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	AggroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AggroSphere"));
	AggroSphere->SetupAttachment(RootComponent);
	AggroSphere->SetSphereRadius(AggroRange);
	AggroSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AggroSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AggroSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AggroSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap); // Player

	HPBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidgetComp"));
	HPBarWidgetComp->SetupAttachment(RootComponent);
	HPBarWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HPBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidgetComp->SetDrawSize(FVector2D(140.f, 20.f));
	HPBarWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	Tags.Add(TEXT("Monster"));

	AIControllerClass = AMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	LockOnWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidgetComp"));
	LockOnWidgetComp->SetupAttachment(GetMesh());
	LockOnWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	LockOnWidgetComp->SetDrawSize(FVector2D(32.f, 32.f));
	LockOnWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LockOnWidgetComp->SetVisibility(false);

	BackAttackTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("BackAttackTrigger"));
	BackAttackTrigger->SetupAttachment(GetMesh(), TEXT("spine_03"));
	BackAttackTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BackAttackTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	BackAttackTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	BackAttackTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BackAttackTrigger->SetBoxExtent(FVector(40.f, 60.f, 80.f));

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	InitialSpawnTransform = GetActorTransform();
	ActiveReturnHomeTransform = InitialSpawnTransform;
	CurrentHP = MaxHP;
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	AggroSphere->SetSphereRadius(AggroRange);

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	UpdateHPWidget();

	// 체력바 위젯은 기본적으로 보이지 않도록 설정
	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}

	if (LockOnWidgetComp && GetMesh())
	{
		LockOnWidgetComp->AttachToComponent(
			GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			LockOnAttachBoneName
		);
	}

	BackAttackTrigger->OnComponentBeginOverlap.AddDynamic(this, &ABaseMonster::OnBackAttackTriggerBeginOverlap);
	BackAttackTrigger->OnComponentEndOverlap.AddDynamic(this, &ABaseMonster::OnBackAttackTriggerEndOverlap);
	AggroSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseMonster::OnAggroSphereBeginOverlap);
	AggroSphere->OnComponentEndOverlap.AddDynamic(this, &ABaseMonster::OnAggroSphereEndOverlap);
	GetWorldTimerManager().SetTimerForNextTick(this, &ABaseMonster::CheckInitialBossHUDOverlap);
}

void ABaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	if (bReturningHome)
	{
		UpdateReturnHome();

		if (bAttackTraceActive)
		{
			bAttackTraceActive = false;
			AlreadyHitActors.Empty();
		}

		return;
	}

	// BeginOverlap이 다시 안 터지는 상황 방지
	// 플레이어가 이미 AggroSphere 안에 있거나 거리 안에 있으면 Tick에서도 감지 시도
	if (!ShouldUseBossHUD() &&
		CanAutoAcquireTarget() &&
		!TargetActor &&
		!bHasDetectedTarget &&
		!bIsAttacking &&
		!bIsKnockedDown &&
		!bIsFrozenForBackAttack &&
		!bInBackAttackReaction)
	{
		TryAcquireCurrentPlayer();
	}

	if (!ShouldUseBossHUD() && TargetActor && !bIsKnockedDown && !bIsFrozenForBackAttack)
	{
		const float DistToTargetSq = FVector::DistSquared(
			GetActorLocation(),
			TargetActor->GetActorLocation()
		);

		if (DistToTargetSq > FMath::Square(LoseTargetDistance))
		{
			LoseTargetAndReturnHome();
			return;
		}
	}

	if (bAttackTraceActive)
	{
		PerformAttackTrace();
	}
}

float ABaseMonster::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("Monster TakeDamage: %.2f -> CurrentHP=%.2f"), DamageAmount, CurrentHP);
	if (bIsDead)
	{
		return 0.f;
	}

	const float FinalDamage = FMath::Max(0.f, DamageAmount);
	CurrentHP = FMath::Clamp(CurrentHP - FinalDamage, 0.f, MaxHP);

	if (DamageCauser && DamageCauser->ActorHasTag(TEXT("Player")))
	{
		CancelAttackForPlayerHit();
		StartHitReactionAttackBlock();

		bReturningHome = false;

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->bUseControllerDesiredRotation = false;

		bHasDetectedTarget = true;
		bCanReacquireWithoutFrontSight = false;
		TargetActor = DamageCauser;

		if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
		{
			MonsterAI->StopMovement();
			MonsterAI->SetIsReturningHome(false);
			MonsterAI->SetTargetActor(DamageCauser);
			MonsterAI->SetIsAttacking(false);
			MonsterAI->SetIsKnockedDown(false);
			MonsterAI->ClearFocus(EAIFocusPriority::Default);
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
		Die();
	}
	else
	{
		if (!bInBackAttackReaction)
		{
			PlayHitReaction();
		}
	}

	return FinalDamage;
}

void ABaseMonster::SetTargetActor(AActor* NewTarget)
{
	if (bIsDead)
	{
		return;
	}

	TargetActor = NewTarget;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetTargetActor(NewTarget);

		if (!NewTarget)
		{
			MonsterAI->ClearFocus(EAIFocusPriority::Default);
			return;
		}

		MonsterAI->ClearFocus(EAIFocusPriority::Default);

		if (ShouldUseBossHUD() &&
			!bIsKnockedDown &&
			!bIsFrozenForBackAttack &&
			!bInBackAttackReaction)
		{
			MonsterAI->SetFocus(NewTarget);
		}
	}
}

void ABaseMonster::LoseTargetAndReturnHome()
{
	if (bIsDead || ShouldUseBossHUD())
	{
		return;
	}

	bReturningHome = true;
	bHasDetectedTarget = false;
	TargetActor = nullptr;

	// 이번 복귀 목적지 저장
	ActiveReturnHomeTransform = BuildReturnHomeTransform();

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsReturningHome(true);
		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->ResetSightToNormal();
		MonsterAI->ClearFocus(EAIFocusPriority::Default);
		MonsterAI->StopMovement();
	}

	ClearCombatTargetForReturnHome();

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	MoveBackToReturnHome();
}

void ABaseMonster::MoveBackToReturnHome()
{
	if (bIsDead)
	{
		return;
	}

	AAIController* AICon = Cast<AAIController>(GetController());
	if (!AICon)
	{
		return;
	}

	const FVector ReturnLocation = ActiveReturnHomeTransform.GetLocation();

	AICon->MoveToLocation(
		ReturnLocation,
		ReturnHomeAcceptanceRadius,
		true,
		true,
		true,
		false,
		nullptr,
		true
	);
}

void ABaseMonster::UpdateReturnHome()
{
	if (!bReturningHome || bIsDead)
	{
		return;
	}

	const FVector ReturnLocation = ActiveReturnHomeTransform.GetLocation();

	const float DistSq = FVector::DistSquared(
		GetActorLocation(),
		ReturnLocation
	);

	const bool bReachedExact =
		DistSq <= FMath::Square(ReturnHomeAcceptanceRadius);

	bool bMoveStopped = false;

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		const EPathFollowingStatus::Type MoveStatus = AICon->GetMoveStatus();
		bMoveStopped =
			MoveStatus == EPathFollowingStatus::Idle ||
			MoveStatus == EPathFollowingStatus::Waiting;
	}

	const bool bCloseEnoughToSnap =
		DistSq <= FMath::Square(ReturnHomeSnapRadius);

	// 핵심:
	// 정확히 목적지에 안 닿아도, AI 이동이 멈췄고 충분히 가까우면
	// 강제로 Home 도착 처리해서 위치/회전을 스냅한다.
	if (bReachedExact || (bMoveStopped && bCloseEnoughToSnap))
	{
		RestoreHomeIdleState();
		return;
	}

	if (bMoveStopped)
	{
		MoveBackToReturnHome();
	}
}

void ABaseMonster::RestoreHomeIdleState()
{
	if (bIsDead)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_AttackCooldown);
	GetWorldTimerManager().ClearTimer(TimerHandle_ReturnHomeFinalize);

	bReturnHomeRotationLockActive = false;
	bReturningHome = false;
	bIsAttacking = false;
	bAttackTraceActive = false;
	bHasDetectedTarget = false;
	TargetActor = nullptr;
	AlreadyHitActors.Empty();

	const FVector ReturnLoc = ActiveReturnHomeTransform.GetLocation();
	const FRotator ReturnRot(
		0.f,
		ActiveReturnHomeTransform.GetRotation().Rotator().Yaw,
		0.f
	);

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Default);
		AICon->SetControlRotation(ReturnRot);
	}

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->SetIsReturningHome(false);
		MonsterAI->ResetSightToNormal();
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	SetActorLocationAndRotation(
		ReturnLoc,
		ReturnRot,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	// Home 복귀 직후 플레이어가 근처에 있으면
	// 정면 시야 검사 기다리지 말고 바로 플레이어를 바라보고 타겟 획득
	AActor* PlayerActor = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (PlayerActor &&
		PlayerActor->ActorHasTag(TEXT("Player")) &&
		IsActorInsideAggroRange(PlayerActor) &&
		CanSeeTargetInFront(PlayerActor))
	{
		TryAcquireTargetAfterReturnHome(PlayerActor);
		return;
	}

	if (ShouldUsePatrol())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
	else
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		SetActorRotation(ReturnRot);
	}

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->SetControlRotation(ReturnRot);
	}
}

bool ABaseMonster::CanAttack() const
{
	if (bIsDead || bReturningHome || bIsAttacking || bIsHitReactionLocked ||
		!TargetActor || bIsKnockedDown || bIsFrozenForBackAttack)
	{
		return false;
	}

	const float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	return Distance <= AttackRange;
}

void ABaseMonster::PerformAttack()
{
	if (bReturningHome || bIsHitReactionLocked)
	{
		return;
	}

	if (!CanAttack())
	{
		return;
	}

	if (TargetActor)
	{
		FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			const FRotator FaceRot = ToTarget.Rotation();
			SetActorRotation(FRotator(0.f, FaceRot.Yaw, 0.f));
		}
	}

	bIsAttacking = true;

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();

		AICon->ClearFocus(EAIFocusPriority::Default);
	}

	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(true);
	}

	PlayAttackMontage();

	GetWorldTimerManager().SetTimer(
		TimerHandle_AttackCooldown,
		this,
		&ABaseMonster::ResetAttackState,
		AttackCooldown,
		false
	);
}

void ABaseMonster::Die()
{
	if (bIsDead)
	{
		return;
	}

	StopBossBGM();

	bIsDead = true;
	bIsAttacking = false;
	bAttackTraceActive = false;
	TargetActor = nullptr;
	AlreadyHitActors.Empty();
	GetWorldTimerManager().ClearTimer(TimerHandle_HitReactionAttackBlock);
	bIsHitReactionLocked = false;
	SetActorEnableCollision(false);

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (AggroSphere)
	{
		AggroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (BackAttackTrigger)
	{
		BackAttackTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}

	if (LockOnWidgetComp)
	{
		LockOnWidgetComp->SetVisibility(false);
	}

	GetCharacterMovement()->DisableMovement();

	if (bShowBossFelledMessageOnDeath)
	{
		if (AKnight* Knight = Cast<AKnight>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			Knight->ShowBossFelledMessage(BossFelledMessage, BossFelledMessageDuration);
		}
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HideHPBar);

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->StopMovement();
		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsDead(true);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
	}

	TrySpawnRandomDrop();
	PlayDeathSound();
	PlayDeathMontage();
	OnDied.Broadcast();

	if (bRespawnOnBonfireRest)
	{
		GetWorldTimerManager().SetTimer(
			TimerHandle_DestroyAfterDeath,
			[this]()
			{
				SetActorHiddenInGame(true);
				GetMesh()->SetVisibility(false, true);
			},
			5.0f,
			false
		);
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			TimerHandle_DestroyAfterDeath,
			[this]()
			{
				Destroy();
			},
			5.0f,
			false
		);
	}
}

void ABaseMonster::ResetAttackState()
{
	if (bIsDead)
	{
		return;
	}

	bIsAttacking = false;
	bAttackTraceActive = false;
	AlreadyHitActors.Empty();

	if (bReturningHome)
	{
		if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
		{
			MonsterAI->SetIsAttacking(false);
			MonsterAI->ClearFocus(EAIFocusPriority::Default);
		}
		return;
	}

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);
		MonsterAI->ClearFocus(EAIFocusPriority::Default);

		// 보스만 Focus 유지
		if (ShouldUseBossHUD() && TargetActor && !bIsKnockedDown && !bIsFrozenForBackAttack)
		{
			MonsterAI->SetFocus(TargetActor);
		}
	}

	if (ShouldUseBossHUD())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void ABaseMonster::CancelAttackForPlayerHit()
{
	bIsAttacking = false;
	bAttackTraceActive = false;
	AlreadyHitActors.Empty();

	GetWorldTimerManager().ClearTimer(TimerHandle_AttackCooldown);

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		// 현재 공격 몽타주 강제 중단
		AnimInstance->StopAllMontages(0.05f);
	}

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Default);
	}

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsReturningHome(false);
		MonsterAI->ClearFocus(EAIFocusPriority::Default);
		MonsterAI->StopMovement();
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void ABaseMonster::StartHitReactionAttackBlock()
{
	bIsHitReactionLocked = true;

	float LockTime = HitReactionAttackBlockFallbackTime;

	if (HitReactionMontageAsset)
	{
		LockTime = FMath::Max(LockTime, HitReactionMontageAsset->GetPlayLength());
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HitReactionAttackBlock);
	GetWorldTimerManager().SetTimer(
		TimerHandle_HitReactionAttackBlock,
		this,
		&ABaseMonster::EndHitReactionAttackBlock,
		LockTime,
		false
	);
}

void ABaseMonster::EndHitReactionAttackBlock()
{
	bIsHitReactionLocked = false;
}

void ABaseMonster::UpdateHPWidget()
{
	if (!HPBarWidgetComp)
	{
		return;
	}

	UMonsterHPBarWidget* HPWidget = Cast<UMonsterHPBarWidget>(HPBarWidgetComp->GetUserWidgetObject());
	if (!HPWidget)
	{
		return;
	}

	HPWidget->UpdateHP(CurrentHP, MaxHP);
	HPWidget->UpdateMonsterName(MonsterDisplayName);
}

void ABaseMonster::StartAttackTrace()
{
	bAttackTraceActive = true;
	AlreadyHitActors.Empty();
}

void ABaseMonster::EndAttackTrace()
{
	bAttackTraceActive = false;
	AlreadyHitActors.Empty();
}

void ABaseMonster::PerformAttackTrace()
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

			const float AppliedDamage = UGameplayStatics::ApplyDamage(
				HitActor,
				AttackPower,
				GetController(),
				this,
				nullptr
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

				PlayAttackHitSuccessSoundAtLocation(HitLocation);
				SpawnAttackHitBloodEffect(HitLocation, HitRotation);
			}
		}
	}
}

void ABaseMonster::PlayAttackMontage_Implementation()
{
	UAnimMontage* MontageToPlay = nullptr;

	// 배열이 있으면 배열에서 랜덤 선택
	if (AttackMontageVariants.Num() > 0)
	{
		TArray<UAnimMontage*> ValidMontages;
		for (UAnimMontage* Montage : AttackMontageVariants)
		{
			if (Montage)
			{
				ValidMontages.Add(Montage);
			}
		}

		if (ValidMontages.Num() > 0)
		{
			const int32 RandIndex = FMath::RandRange(0, ValidMontages.Num() - 1);
			MontageToPlay = ValidMontages[RandIndex];
		}
	}

	// 배열이 비어 있으면 기존 단일 몽타주 fallback
	if (!MontageToPlay)
	{
		MontageToPlay = AttackMontageAsset;
	}

	if (!MontageToPlay)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(MontageToPlay);
	}
}

void ABaseMonster::PlayHitReaction_Implementation()
{
	if (!HitReactionMontageAsset)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(HitReactionMontageAsset);
	}
}

void ABaseMonster::PlayDeathMontage_Implementation()
{
	if (!DeathMontageAsset)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(DeathMontageAsset);
	}
}

void ABaseMonster::ShowHPBar()
{
	if (!HPBarWidgetComp)
	{
		return;
	}

	HPBarWidgetComp->SetVisibility(true);

	GetWorldTimerManager().ClearTimer(TimerHandle_HideHPBar);
	GetWorldTimerManager().SetTimer(
		TimerHandle_HideHPBar,
		this,
		&ABaseMonster::HideHPBar,
		HPBarVisibleTime,
		false
	);
}

void ABaseMonster::HideHPBar()
{
	if (!HPBarWidgetComp || bIsDead)
	{
		return;
	}

	HPBarWidgetComp->SetVisibility(false);
}

void ABaseMonster::SetLockOnVisible(bool bVisible)
{
	if (!LockOnWidgetComp)
	{
		return;
	}

	if (bIsDead)
	{
		LockOnWidgetComp->SetVisibility(false);
		return;
	}

	LockOnWidgetComp->SetVisibility(bVisible);
}

void ABaseMonster::EnterKnockDown(float Duration)
{
	if (bIsDead)
	{
		return;
	}

	bIsKnockedDown = true;
	bIsAttacking = false;
	bAttackTraceActive = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_AttackCooldown);

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(true);

		// 타겟은 유지하되 이동/바라보기 중지
		MonsterAI->SetTargetActor(TargetActor);
		MonsterAI->StopMovement();
		MonsterAI->ClearFocus(EAIFocusPriority::Default);

		//if (MonsterAI->BrainComponent)
		//{
		//	// 완전히 AI를 죽이고 싶지는 않으면 StopLogic은 쓰지 말고
		//	// Blackboard의 IsKnockedDown으로 BT에서 MoveTo만 막는 방식 추천
		//}
	}
	else if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Default);
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->DisableMovement();

	// 컨트롤러 Focus 때문에 몸이 돌아가는 것 방지
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_KnockDown);
	GetWorldTimerManager().SetTimer(
		TimerHandle_KnockDown,
		this,
		&ABaseMonster::EndKnockDown,
		Duration,
		false
	);
}

void ABaseMonster::EndKnockDown()
{
	if (bIsDead)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (KnockDownMontageAsset && AnimInstance->Montage_IsPlaying(KnockDownMontageAsset))
		{
			AnimInstance->Montage_Stop(0.15f, KnockDownMontageAsset);
		}
	}

	bIsKnockedDown = false;
	bIsAttacking = false;
	bAttackTraceActive = false;
	bInBackAttackReaction = false;

	UnfreezeFromBackAttack();

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// AI 회전 복구
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->SetIsAttacking(false);

		// 녹다운이 끝난 뒤 다시 바라보고 추적 가능하게 복구
		if (TargetActor)
		{
			MonsterAI->SetTargetActor(TargetActor);
			MonsterAI->SetFocus(TargetActor);
		}
	}
}

void ABaseMonster::PlayKnockDownMontage_Implementation()
{
	if (!KnockDownMontageAsset)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(KnockDownMontageAsset);
	}
}

void ABaseMonster::FreezeForBackAttack()
{
	if (bIsDead)
	{
		return;
	}

	bIsFrozenForBackAttack = true;
	bIsAttacking = false;
	bAttackTraceActive = false;

	SetTargetActor(nullptr);

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetTargetActor(nullptr);
	}

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.1f);
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
}

void ABaseMonster::UnfreezeFromBackAttack()
{
	if (bIsDead)
	{
		return;
	}

	bIsFrozenForBackAttack = false;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void ABaseMonster::SetBackAttackReaction(bool bEnable)
{
	bInBackAttackReaction = bEnable;
}

bool ABaseMonster::CanBeBackAttacked() const
{
	return !bIsDead && !bIsKnockedDown && !bIsFrozenForBackAttack;
}

void ABaseMonster::OnBackAttackTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!CanBeBackAttacked() || !OtherActor)
	{
		return;
	}

	if (AKnight* Knight = Cast<AKnight>(OtherActor))
	{
		Knight->RegisterBackAttackTarget(this);
	}
}

void ABaseMonster::OnBackAttackTriggerEndOverlap(
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
		Knight->UnregisterBackAttackTarget(this);
	}
}

bool ABaseMonster::CanSeeTargetInFront(AActor* OtherActor) const
{
	if (!OtherActor)
	{
		return false;
	}

	FVector ToTarget = OtherActor->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	ToTarget.Normalize();

	const FRotator ForwardRot(
		0.f,
		GetActorRotation().Yaw + SightForwardYawOffset,
		0.f
	);

	FVector Forward = ForwardRot.Vector();
	Forward.Z = 0.f;
	Forward.Normalize();

	const float Dot = FVector::DotProduct(Forward, ToTarget);
	const float Threshold = FMath::Cos(FMath::DegreesToRadians(SightAngleDegrees * 0.5f));
	if (Dot < Threshold)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 80.f);
	const FVector End = OtherActor->GetActorLocation() + FVector(0.f, 0.f, 80.f);

	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	return !bBlocked || Hit.GetActor() == OtherActor;
}

void ABaseMonster::OnAggroSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || bIsDead)
	{
		return;
	}

	AKnight* Knight = Cast<AKnight>(OtherActor);
	if (!Knight)
	{
		return;
	}

	bPlayerInsideAggroSphere = true;

	if (bReturningHome)
	{
		return;
	}

	if (ShouldUseBossHUD())
	{
		Knight->SetCurrentBossHUDTarget(this);
		StartBossBGM();

		if (IsWaitingForIntroMovie())
		{
			return;
		}

		TryAcquireTarget(Knight);
		return;
	}

	if (!CanAutoAcquireTarget())
	{
		return;
	}

	TryAcquireTarget(Knight);
}

void ABaseMonster::OnAggroSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	AKnight* Knight = Cast<AKnight>(OtherActor);
	if (!Knight)
	{
		return;
	}

	bPlayerInsideAggroSphere = false;

	if (ShouldUseBossHUD())
	{
		if (Knight->GetCurrentBossHUDTarget() == this)
		{
			Knight->SetCurrentBossHUDTarget(nullptr);
		}

		StopBossBGM();
	}
}

void ABaseMonster::TrySpawnRandomDrop()
{
	if (!ItemDropActorClass)
	{
		return;
	}

	if (RandomDropList.Num() <= 0)
	{
		return;
	}

	TArray<FMonsterRandomDropEntry> PassedEntries;

	for (const FMonsterRandomDropEntry& Entry : RandomDropList)
	{
		if (Entry.ItemData.ItemID.IsNone())
		{
			continue;
		}

		if (Entry.Count <= 0)
		{
			continue;
		}

		const float ClampedChance = FMath::Clamp(Entry.Chance, 0.f, 1.f);
		if (FMath::FRand() <= ClampedChance)
		{
			PassedEntries.Add(Entry);
		}
	}

	// 확률 통과한 후보가 없으면 드랍 없음
	if (PassedEntries.Num() <= 0)
	{
		return;
	}

	// 통과한 후보들 중 1개만 랜덤 선택
	const int32 SelectedIndex = FMath::RandRange(0, PassedEntries.Num() - 1);
	const FMonsterRandomDropEntry& SelectedEntry = PassedEntries[SelectedIndex];

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AItemDropActor* DropActor = GetWorld()->SpawnActor<AItemDropActor>(
		ItemDropActorClass,
		GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (DropActor)
	{
		FInventoryItem DropItem;
		DropItem.ItemData = SelectedEntry.ItemData;
		DropItem.Count = SelectedEntry.Count;
		DropActor->InitializeDrop(DropItem);
	}
}

void ABaseMonster::RespawnMonster()
{
	if (!bIsDead)
	{
		return;
	}

	// 죽을 때 걸린 타이머들 정리
	GetWorldTimerManager().ClearTimer(TimerHandle_DestroyAfterDeath);
	GetWorldTimerManager().ClearTimer(TimerHandle_AttackCooldown);
	GetWorldTimerManager().ClearTimer(TimerHandle_KnockDown);
	GetWorldTimerManager().ClearTimer(TimerHandle_HideHPBar);

	bIsDead = false;
	bIsAttacking = false;
	bAttackTraceActive = false;
	bIsKnockedDown = false;
	bIsFrozenForBackAttack = false;
	bInBackAttackReaction = false;
	bHasDetectedTarget = false;
	TargetActor = nullptr;
	AlreadyHitActors.Empty();
	CurrentPatrolIndex = 0;

	CurrentHP = MaxHP;
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	UpdateHPWidget();

	// 스폰 위치로 완전 복귀
	SetActorTransform(InitialSpawnTransform);

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetVisibility(true, true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 몽타주 전부 중지
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.f);
	}

	// ABP 상태 다시 초기화
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->InitAnim(true);

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (AggroSphere)
	{
		AggroSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}

	if (LockOnWidgetComp)
	{
		LockOnWidgetComp->SetVisibility(false);
	}

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Default);
	}

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsDead(false);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);

		if (MonsterAI->BrainComponent)
		{
			MonsterAI->BrainComponent->RestartLogic();
		}
	}
}

void ABaseMonster::CheckInitialBossHUDOverlap()
{
	if (!ShouldUseBossHUD() || !AggroSphere)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	AggroSphere->GetOverlappingActors(OverlappingActors, AKnight::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (AKnight* Knight = Cast<AKnight>(Actor))
		{
			Knight->SetCurrentBossHUDTarget(this);
			StartBossBGM();
			break;
		}
	}
}

void ABaseMonster::ResetToInitialStateOnBonfireRest()
{
	if (bIsDead)
	{
		return;
	}

	// 전투 상태 초기화
	bIsAttacking = false;
	bAttackTraceActive = false;
	bHasDetectedTarget = false;
	bIsKnockedDown = false;
	bIsFrozenForBackAttack = false;
	bInBackAttackReaction = false;
	TargetActor = nullptr;
	AlreadyHitActors.Empty();
	CurrentPatrolIndex = 0;

	// 타이머 정리
	GetWorldTimerManager().ClearTimer(TimerHandle_AttackCooldown);
	GetWorldTimerManager().ClearTimer(TimerHandle_KnockDown);
	GetWorldTimerManager().ClearTimer(TimerHandle_HideHPBar);

	// HP바/락온 UI 숨김
	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}

	if (LockOnWidgetComp)
	{
		LockOnWidgetComp->SetVisibility(false);
	}

	// AI 정리
	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->StopMovement();
		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->ClearFocus(EAIFocusPriority::Default);
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.1f);
	}

	// 원래 자리로 복귀
	SetActorLocationAndRotation(
		InitialSpawnTransform.GetLocation(),
		InitialSpawnTransform.GetRotation().Rotator()
	);

	// 이동 상태 복구
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

AActor* ABaseMonster::GetCurrentPatrolPoint() const
{
	if (!ShouldUsePatrol())
	{
		return nullptr;
	}

	if (!PatrolPoints.IsValidIndex(CurrentPatrolIndex))
	{
		return nullptr;
	}

	return PatrolPoints[CurrentPatrolIndex];
}

void ABaseMonster::AdvancePatrolPoint()
{
	if (!ShouldUsePatrol())
	{
		return;
	}

	CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
}

USoundBase* ABaseMonster::GetRandomMonsterSound(const TArray<USoundBase*>& Sounds) const
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

void ABaseMonster::PlayWeaponSwingSound()
{
	USoundBase* Sound = GetRandomMonsterSound(WeaponSwingSounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(0.96f, 1.04f);

	if (GetMesh())
	{
		UGameplayStatics::SpawnSoundAttached(
			Sound,
			GetMesh(),
			AttackTraceEndSocketName,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget,
			true,
			AttackSFXVolume,
			Pitch
		);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Sound,
			GetActorLocation(),
			AttackSFXVolume,
			Pitch
		);
	}
}

void ABaseMonster::PlayAttackVoiceSound()
{
	USoundBase* Sound = GetRandomMonsterSound(AttackVoiceSounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(0.97f, 1.03f);

	UGameplayStatics::SpawnSoundAttached(
		Sound,
		GetMesh(),
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true,
		VoiceVolume,
		Pitch
	);
}

void ABaseMonster::StartBossBGM()
{
	if (!ShouldUseBossHUD())
	{
		return;
	}

	if (!BossBGMSound)
	{
		return;
	}

	if (bBossBGMActive)
	{
		return;
	}

	bBossBGMActive = true;

	if (AStageBGMManager* StageBGM = AStageBGMManager::FindStageBGMManager(GetWorld()))
	{
		StageBGM->PauseStageBGMForBoss();
	}

	if (!BossBGMComponent)
	{
		BossBGMComponent = UGameplayStatics::CreateSound2D(
			this,
			BossBGMSound,
			BossBGMVolume,
			1.0f,
			0.0f,
			nullptr,
			false,
			false
		);
	}

	if (BossBGMComponent)
	{
		BossBGMComponent->FadeIn(BossBGMFadeInTime, BossBGMVolume);
	}
}

void ABaseMonster::StopBossBGM()
{
	if (!bBossBGMActive)
	{
		return;
	}

	bBossBGMActive = false;

	if (BossBGMComponent)
	{
		BossBGMComponent->FadeOut(BossBGMFadeOutTime, 0.0f);
	}

	if (AStageBGMManager* StageBGM = AStageBGMManager::FindStageBGMManager(GetWorld()))
	{
		StageBGM->ResumeStageBGMAfterBoss();
	}

	if (AKnight* Knight = Cast<AKnight>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (Knight->GetCurrentBossHUDTarget() == this)
		{
			Knight->SetCurrentBossHUDTarget(nullptr);
		}
	}
}

USoundBase* ABaseMonster::GetRandomDeathSound() const
{
	TArray<USoundBase*> ValidSounds;

	for (USoundBase* Sound : DeathSounds)
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

void ABaseMonster::PlayDeathSound()
{
	USoundBase* Sound = GetRandomDeathSound();
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(DeathSoundPitchMin, DeathSoundPitchMax);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		DeathSoundVolume,
		Pitch
	);
}

bool ABaseMonster::CanBeLockedOn() const
{
	return !bIsDead;
}

void ABaseMonster::PlayAttackHitSuccessSoundAtLocation(const FVector& Location)
{
	if (AttackHitSuccessSounds.Num() <= 0)
	{
		return;
	}

	TArray<USoundBase*> ValidSounds;
	for (USoundBase* Sound : AttackHitSuccessSounds)
	{
		if (Sound)
		{
			ValidSounds.Add(Sound);
		}
	}

	if (ValidSounds.Num() <= 0)
	{
		return;
	}

	const int32 RandIndex = FMath::RandRange(0, ValidSounds.Num() - 1);
	USoundBase* SelectedSound = ValidSounds[RandIndex];

	const float Pitch = FMath::RandRange(
		AttackHitSuccessPitchMin,
		AttackHitSuccessPitchMax
	);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		SelectedSound,
		Location,
		AttackHitSuccessVolume,
		Pitch
	);
}

void ABaseMonster::SpawnAttackHitBloodEffect(const FVector& Location, const FRotator& Rotation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector EffectScale(AttackHitBloodEffectScale);

	if (AttackHitBloodNiagaraEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			AttackHitBloodNiagaraEffect,
			Location,
			Rotation,
			EffectScale
		);
	}

	if (AttackHitBloodParticleEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			AttackHitBloodParticleEffect,
			Location,
			Rotation,
			EffectScale,
			true
		);
	}
}

void ABaseMonster::ClearCombatTargetForReturnHome()
{
	TargetActor = nullptr;
	bHasDetectedTarget = false;
	bIsAttacking = false;
	bAttackTraceActive = false;
	AlreadyHitActors.Empty();

	GetWorldTimerManager().ClearTimer(TimerHandle_AttackCooldown);

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (AttackMontageAsset && AnimInstance->Montage_IsPlaying(AttackMontageAsset))
		{
			AnimInstance->Montage_Stop(0.1f, AttackMontageAsset);
		}

		for (UAnimMontage* Montage : AttackMontageVariants)
		{
			if (Montage && AnimInstance->Montage_IsPlaying(Montage))
			{
				AnimInstance->Montage_Stop(0.1f, Montage);
			}
		}
	}

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->ClearFocus(EAIFocusPriority::Default);
		MonsterAI->StopMovement();
	}
	else if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->ClearFocus(EAIFocusPriority::Default);
		AICon->StopMovement();
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}

bool ABaseMonster::CanAcquireTarget(AActor* OtherActor) const
{
	if (!OtherActor || bIsDead)
	{
		return false;
	}

	if (!CanAutoAcquireTarget())
	{
		return false;
	}

	if (ShouldUseBossHUD())
	{
		return true;
	}

	if (bReturningHome)
	{
		return false;
	}

	if (bHasDetectedTarget || TargetActor)
	{
		return false;
	}

	if (bIsKnockedDown || bIsFrozenForBackAttack || bInBackAttackReaction)
	{
		return false;
	}

	if (!IsActorInsideAggroRange(OtherActor))
	{
		return false;
	}

	// 핵심: 일반 몬스터의 최초 감지는 반드시 정면 시야 안에서만 허용
	if (!CanSeeTargetInFront(OtherActor))
	{
		return false;
	}

	return true;
}

void ABaseMonster::TryAcquireTarget(AActor* OtherActor)
{
	TryAcquireTargetFromAI(OtherActor);
}

bool ABaseMonster::TryAcquireTargetFromAI(AActor* OtherActor)
{
	if (!CanAcquireTarget(OtherActor))
	{
		return false;
	}

	bCanReacquireWithoutFrontSight = false;
	bReturnHomeRotationLockActive = false;
	bReturningHome = false;

	bHasDetectedTarget = true;
	TargetActor = OtherActor;

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	GetCharacterMovement()->StopMovementImmediately();

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	FaceActorImmediately(OtherActor);

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->SetIsReturningHome(false);
		MonsterAI->SetTargetActor(OtherActor);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->ClearFocus(EAIFocusPriority::Default);
	}

	return true;
}

bool ABaseMonster::IsActorInsideAggroRange(AActor* OtherActor) const
{
	if (!OtherActor)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(
		GetActorLocation(),
		OtherActor->GetActorLocation()
	);

	return DistSq <= FMath::Square(AggroRange);
}

void ABaseMonster::TryAcquireCurrentPlayer()
{
	AActor* PlayerActor = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!PlayerActor || !PlayerActor->ActorHasTag(TEXT("Player")))
	{
		return;
	}

	TryAcquireTargetFromAI(PlayerActor);
}

FTransform ABaseMonster::BuildReturnHomeTransform()
{
	if (ShouldUsePatrol())
	{
		float BestDistSq = TNumericLimits<float>::Max();
		int32 BestIndex = INDEX_NONE;

		for (int32 i = 0; i < PatrolPoints.Num(); ++i)
		{
			AActor* Point = PatrolPoints[i];
			if (!Point)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(
				GetActorLocation(),
				Point->GetActorLocation()
			);

			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestIndex = i;
			}
		}

		if (PatrolPoints.IsValidIndex(BestIndex) && PatrolPoints[BestIndex])
		{
			CurrentPatrolIndex = BestIndex;

			const FVector ReturnLoc = PatrolPoints[BestIndex]->GetActorLocation();

			FRotator ReturnRot = PatrolPoints[BestIndex]->GetActorRotation();
			ReturnRot.Pitch = 0.f;
			ReturnRot.Roll = 0.f;

			return FTransform(ReturnRot, ReturnLoc, GetActorScale3D());
		}
	}

	FVector HomeLoc = InitialSpawnTransform.GetLocation();
	FRotator HomeRot = InitialSpawnTransform.GetRotation().Rotator();
	HomeRot.Pitch = 0.f;
	HomeRot.Roll = 0.f;

	return FTransform(HomeRot, HomeLoc, GetActorScale3D());
}

void ABaseMonster::ApplyReturnHomeTransformLocked()
{
	const FVector ReturnLoc = ActiveReturnHomeTransform.GetLocation();
	const FRotator ReturnRot(
		0.f,
		ActiveReturnHomeTransform.GetRotation().Rotator().Yaw,
		0.f
	);

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Default);
		AICon->SetControlRotation(ReturnRot);
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	// 회전 고정 중에는 이동 방향 회전 금지
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	SetActorLocationAndRotation(
		ReturnLoc,
		ReturnRot,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	SetActorRotation(ReturnRot);
}

void ABaseMonster::FinishReturnHomeIdleState()
{
	if (bIsDead)
	{
		return;
	}

	bReturnHomeRotationLockActive = false;
	bReturningHome = false;
	bCanReacquireWithoutFrontSight = false;

	ApplyReturnHomeTransformLocked();

	const FRotator ReturnRot(
		0.f,
		ActiveReturnHomeTransform.GetRotation().Rotator().Yaw,
		0.f
	);

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->StopMovement();
		MonsterAI->ClearFocus(EAIFocusPriority::Default);
		MonsterAI->SetControlRotation(ReturnRot);

		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
		MonsterAI->SetIsReturningHome(false);
		MonsterAI->ResetSightToNormal();
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	// 핵심:
	// Home 도착 직후 플레이어가 이미 감지 범위 안이면
	// Home 방향 유지하지 말고 즉시 플레이어를 바라보고 추적 시작
	AActor* PlayerActor = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (PlayerActor && PlayerActor->ActorHasTag(TEXT("Player")) && IsActorInsideAggroRange(PlayerActor))
	{
		TryAcquireTargetAfterReturnHome(PlayerActor);
		return;
	}

	// 플레이어가 근처에 없을 때만 Home/Patrol 회전 유지
	if (ShouldUsePatrol())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
	else
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
		SetActorRotation(ReturnRot);
	}

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->SetControlRotation(ReturnRot);
	}
}

void ABaseMonster::FaceActorImmediately(AActor* OtherActor)
{
	if (!OtherActor)
	{
		return;
	}

	FVector ToTarget = OtherActor->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator FaceRot(0.f, ToTarget.Rotation().Yaw, 0.f);

	SetActorRotation(FaceRot);

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->ClearFocus(EAIFocusPriority::Default);
		AICon->SetControlRotation(FaceRot);
	}
}

bool ABaseMonster::TryAcquireTargetAfterReturnHome(AActor* OtherActor)
{
	if (!OtherActor || bIsDead || ShouldUseBossHUD())
	{
		return false;
	}

	if (!CanAutoAcquireTarget())
	{
		return false;
	}

	if (!OtherActor->ActorHasTag(TEXT("Player")))
	{
		return false;
	}

	if (bIsKnockedDown || bIsFrozenForBackAttack || bInBackAttackReaction)
	{
		return false;
	}

	if (!IsActorInsideAggroRange(OtherActor))
	{
		return false;
	}

	if (!CanSeeTargetInFront(OtherActor))
	{
		return false;
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_ReturnHomeFinalize);
	GetWorldTimerManager().ClearTimer(TimerHandle_AttackCooldown);

	bReturnHomeRotationLockActive = false;
	bReturningHome = false;
	bCanReacquireWithoutFrontSight = false;

	bIsAttacking = false;
	bAttackTraceActive = false;
	bHasDetectedTarget = true;
	TargetActor = OtherActor;
	AlreadyHitActors.Empty();

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
	GetCharacterMovement()->StopMovementImmediately();

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	// 핵심: Home에서 뒤돌아있든 뭐든, 재감지 성공 시 즉시 플레이어를 바라보게 함
	FaceActorImmediately(OtherActor);

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->StopMovement();
		MonsterAI->ClearFocus(EAIFocusPriority::Default);

		MonsterAI->SetIsReturningHome(false);
		MonsterAI->SetTargetActor(OtherActor);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
	}

	return true;
}