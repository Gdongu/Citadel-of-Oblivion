#include "Knight.h"
#include <GameFramework/SpringArmComponent.h>
#include <Camera/CameraComponent.h>
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include <Kismet/GameplayStatics.h>
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BaseMonster.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "InteractInterface.h"
#include "FortressGameInstance.h"
#include "Bonfire.h"
#include "Engine/OverlapResult.h"
#include "BossPowerAttackDamageType.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TutorialBossMonster.h"
#include "Engine/DamageEvents.h"
#include "MemoryShardDrop.h"
#include "InventoryComponent.h"
#include "MimicChestMonster.h"
#include "TutorialPopupWidget.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "Components/StaticMeshComponent.h"
#include "BossFelledWidget.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "BossNiagaraProjectile.h"
#include "Engine/World.h"
#include "ItemDropActor.h"
#include "FinalBossPhase1.h"
#include "FinalBossPhase2.h"
#include "Components/PointLightComponent.h"

AKnight::AKnight()
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinder<USkeletalMesh>
		TempMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/Dark_Knight/Dark_Knight_Male/Meshes/SKM_DKM_Full.SKM_DKM_Full'"));
	if (TempMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(TempMesh.Object);

		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
	}

	swordMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMeshComp"));
	swordMeshComp->SetupAttachment(GetMesh(), TEXT("WeaponWaistSocket"));
	ConstructorHelpers::FObjectFinder<UStaticMesh>
		TempSwordMesh(TEXT("/Script/Engine.StaticMesh'/Game/Dark_Knight/Dark_Knight_Male/Meshes/SM_DKM_Sword.SM_DKM_Sword'"));
	if (TempSwordMesh.Succeeded())
	{
		DefaultWeaponStaticMesh = TempSwordMesh.Object;
		swordMeshComp->SetStaticMesh(TempSwordMesh.Object);
		//swordMeshComp->SetRelativeLocation(FVector(-9, 3, -2));
		//swordMeshComp->SetRelativeRotation(FRotator(-90, 0, 90));
	}

	springArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	springArmComp->SetupAttachment(RootComponent);
	//springArmComp->SetRelativeLocation(FVector(0, 0, 160));
	//springArmComp->TargetArmLength = 400;
	springArmComp->SetRelativeLocation(NormalCameraRelativeLocation);
	springArmComp->TargetArmLength = NormalCameraArmLength;
	springArmComp->SocketOffset = NormalCameraSocketOffset;

	springArmComp->bUsePawnControlRotation = true;

	playerCamComp = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamComp"));
	playerCamComp->SetupAttachment(springArmComp);
	playerCamComp->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComp"));

	Tags.Add(TEXT("Player"));
}

void AKnight::BeginPlay()
{
	Tags.Remove(TEXT("DeadPlayer"));
	if (!Tags.Contains(TEXT("Player")))
	{
		Tags.Add(TEXT("Player"));
	}

	Super::BeginPlay();

	// 초기 이동 속도 300으로 설정
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

	CurrentHP = MaxHP;
	CurrentStamina = MaxStamina;

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	OnExhaustedChanged.Broadcast(bIsExhausted);

	auto pc = Cast<APlayerController>(GetController());
	if (pc)
	{
		auto Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(imc_Player, 0);
		}
	}

	// 플레이어 위젯 생성
	if (PlayerWidgetClass)
	{
		PlayerWidget = CreateWidget<UUserWidget>(GetWorld(), PlayerWidgetClass);
		if (PlayerWidget)
		{
			PlayerWidget->AddToViewport();
		}
	}

	if (InventoryComp)
	{
		InventoryComp->OnInventoryUpdated.AddDynamic(this, &AKnight::HandleInventoryUpdated);
	}

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));

		// 포탈 이동이 제일 우선
		if (TryMoveToPendingPortalArrival(GI, CurrentLevelName))
		{
			// 포탈 도착 위치 적용 완료
		}
		// 기억의 불씨 이동
		else if (GI->HasPendingBonfireTravel() && GI->GetPendingTravelLevelName() == CurrentLevelName)
		{
			const FTransform TravelTransform = GI->GetPendingTravelTransform();
			const FRotator TravelYawRot(0.f, TravelTransform.GetRotation().Rotator().Yaw, 0.f);

			SetActorLocationAndRotation(
				TravelTransform.GetLocation(),
				TravelYawRot
			);

			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				PC->SetControlRotation(TravelYawRot);
			}

			GI->ClearPendingBonfireTravel();
		}
		// 같은 레벨에서 죽고 재시작한 경우만 체크포인트 적용
		else if (GI->HasCheckpoint() && GI->GetCheckpointLevelName() == CurrentLevelName)
		{
			const FTransform CheckpointTransform = GI->GetCheckpointTransform();
			const FRotator CheckpointYawRot(0.f, CheckpointTransform.GetRotation().Rotator().Yaw, 0.f);

			SetActorLocationAndRotation(
				CheckpointTransform.GetLocation(),
				CheckpointYawRot
			);
		}

		float LoadingDuration = 0.f;
		if (GI->ConsumePendingLoadingScreen(LoadingDuration))
		{
			ShowLoadingScreen(LoadingDuration);
		}
	}

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		CurrentMemoryShardCount = GI->GetCurrentMemoryShards();
		OnMemoryShardChanged.Broadcast(CurrentMemoryShardCount);

		SpawnPendingMemoryShardDropFromGI();

		LoadEnhancementFromGameInstance();
		ApplyEnhancementDataToStats(true);

		LoadInventoryFromGameInstance();
	}

	TryPlayTownIntro();

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		if (GI->HasPendingDeathPenaltyTutorial())
		{
			TryShowDeathPenaltyTutorial();
			GI->SetPendingDeathPenaltyTutorial(false);
		}
	}

	AttachSwordToWaist();
	SetSwordVisible(true);

	LastFootstepLocation = GetActorLocation();
	FootstepDistanceAccumulator = 0.f;
}

bool AKnight::FindPortalArrivalTransform(FName ArrivalTag, FTransform& OutTransform) const
{
	if (ArrivalTag.IsNone() || !GetWorld())
	{
		return false;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this)
		{
			continue;
		}

		if (Actor->ActorHasTag(ArrivalTag))
		{
			OutTransform = Actor->GetActorTransform();
			return true;
		}
	}

	return false;
}

bool AKnight::TryMoveToPendingPortalArrival(UFortressGameInstance* GI, FName CurrentLevelName)
{
	if (!GI || !GI->HasPendingPortalTravel())
	{
		return false;
	}

	if (GI->GetPendingPortalLevelName() != CurrentLevelName)
	{
		return false;
	}

	const FName ArrivalTag = GI->GetPendingPortalArrivalTag();

	FTransform ArrivalTransform;
	const bool bFoundArrival = FindPortalArrivalTransform(ArrivalTag, ArrivalTransform);

	if (bFoundArrival)
	{
		const FRotator ArrivalYawRot(
			0.f,
			ArrivalTransform.GetRotation().Rotator().Yaw,
			0.f
		);

		SetActorLocationAndRotation(
			ArrivalTransform.GetLocation(),
			ArrivalYawRot,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetControlRotation(ArrivalYawRot);
		}
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Portal arrival point not found. Tag: %s"),
			*ArrivalTag.ToString()
		);
	}

	GI->ClearPendingPortalTravel();

	// 못 찾았더라도 true 반환.
	// 그래야 체크포인트/화톳불 위치가 포탈 이동을 덮어쓰지 않음.
	return true;
}

// Called every frame
void AKnight::Tick(float DeltaTime)
{
	if (bIsDead)
	{
		return;
	}

	Super::Tick(DeltaTime);

	if (bIsGrabbedByBoss)
	{
		MoveInput = FVector2D::ZeroVector;
		return;
	}

	UpdateRecentSafeTransforms(DeltaTime);
	UpdateStamina(DeltaTime);
	PlayerMove();
	UpdateFootstepSound(DeltaTime);

	if (bIsLockOnActive)
	{
		UpdateLockOn(DeltaTime);
	}

	if (bIsExhausted)
	{
		ExhaustedRemainingTime -= DeltaTime;
		ExhaustedRemainingTime = FMath::Max(0.f, ExhaustedRemainingTime);

		OnExhaustedTimeChanged.Broadcast(ExhaustedRemainingTime);
	}

	if (bAttackTraceActive)
	{
		PerformAttackTrace();
	}

	// 구르기 실제 이동
	if (bDodgeMoving)
	{
		const float MovePerSecond = DodgeMoveDistance / DodgeMoveDuration;
		const FVector DeltaMove = DodgeMoveDirection * MovePerSecond * DeltaTime;

		FHitResult Hit;
		AddActorWorldOffset(DeltaMove, true, &Hit);

		DodgeMoveElapsed += DeltaTime;
		if (DodgeMoveElapsed >= DodgeMoveDuration)
		{
			bDodgeMoving = false;
			DodgeMoveElapsed = 0.f;
		}
	}

	// 스킬 이동
	if (bSkillMoving)
	{
		const float SafeMoveDuration = FMath::Max(0.01f, ActiveSkillMoveDuration);
		const float MovePerSecond = ActiveSkillForwardDistance / SafeMoveDuration;
		const FVector DeltaMove = SkillMoveDirection * MovePerSecond * DeltaTime;

		FHitResult Hit;
		AddActorWorldOffset(DeltaMove, true, &Hit);

		SkillMoveElapsed += DeltaTime;
		if (SkillMoveElapsed >= SkillMoveDuration)
		{
			bSkillMoving = false;
			SkillMoveElapsed = 0.f;
		}
	}
}

// Called to bind functionality to input
void AKnight::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto PlayerInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (PlayerInput)
	{
		PlayerInput->BindAction(ia_Turn, ETriggerEvent::Triggered, this, &AKnight::Turn);
		PlayerInput->BindAction(ia_LookUp, ETriggerEvent::Triggered, this, &AKnight::LookUp);
		PlayerInput->BindAction(ia_Move, ETriggerEvent::Triggered, this, &AKnight::Move);
		PlayerInput->BindAction(ia_Run, ETriggerEvent::Started, this, &AKnight::StartRun);
		PlayerInput->BindAction(ia_Run, ETriggerEvent::Completed, this, &AKnight::StopRun);
		PlayerInput->BindAction(ia_Attack, ETriggerEvent::Started, this, &AKnight::Attack);
		PlayerInput->BindAction(ia_Dodge, ETriggerEvent::Started, this, &AKnight::Dodge);
		PlayerInput->BindAction(ia_LockOn, ETriggerEvent::Started, this, &AKnight::OnLockOnPressed);
		PlayerInput->BindAction(ia_LockOn, ETriggerEvent::Completed, this, &AKnight::OnLockOnReleased);
		PlayerInput->BindAction(ia_Drink, ETriggerEvent::Started, this, &AKnight::DrinkPotion);
		PlayerInput->BindAction(ia_Interact, ETriggerEvent::Started, this, &AKnight::Interact);
		PlayerInput->BindAction(ia_Skill, ETriggerEvent::Started, this, &AKnight::UseSkill);
		PlayerInput->BindAction(ia_Inventory, ETriggerEvent::Started, this, &AKnight::ToggleInventory);
		PlayerInput->BindAction(ia_Pause, ETriggerEvent::Started, this, &AKnight::HandleEscapeInput);
	}
}

void AKnight::Turn(const FInputActionValue& inputValue)
{
	if (bIsLockOnActive)
	{
		return;
	}

	float value = inputValue.Get<float>();
	AddControllerYawInput(value);
}

void AKnight::LookUp(const FInputActionValue& inputValue)
{
	if (bIsLockOnActive)
	{
		return;
	}

	float value = inputValue.Get<float>();
	AddControllerPitchInput(value);
}

void AKnight::Move(const FInputActionValue& inputValue)
{
	if (bIsKnockedDownByBoss || bIsGettingUpFromBossKnockDown || bIsGrabbedByBoss)
	{
		MoveInput = FVector2D::ZeroVector;
		return;
	}

	if (bIsDead || bIsHitReacting || bIsAttacking || bIsExhausted || bIsUsingSkill 
		|| bIsBackAttacking || bIsOpeningDoor || bIsWeaponBouncing || bIsOpeningChest || bIsPlayingItemPickupMontage)
	{
		return;
	}

	FVector2D value = inputValue.Get<FVector2D>();

	MoveInput = value;

	if (!value.IsNearlyZero())
	{
		LastMoveInput = value;

		// 공격 중이거나 콤보 유지 시간 중에 이동 입력이 들어오면 콤보 초기화
		if (CurrentComboIndex > 0 || GetWorldTimerManager().IsTimerActive(TimerHandle_ComboReset))
		{
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				if (AttackMontage && AnimInstance->Montage_IsPlaying(AttackMontage))
				{
					AnimInstance->Montage_Stop(0.1f, AttackMontage);
				}
			}

			GetWorldTimerManager().ClearTimer(TimerHandle_ComboReset);
			ResetCombo();
		}
	}
}

void AKnight::Dodge(const FInputActionValue& inputValue)
{
	if (bIsGrabbedByBoss)
	{
		return;
	}

	if (bIsGettingUpFromBossKnockDown)
	{
		return;
	}

	if (bIsKnockedDownByBoss)
	{
		TryGetUpFromBossKnockDown();
		return;
	}

	const bool bDodgeOutOfHitReaction =
		bIsHitReacting && bCanDodgeDuringHitReaction;

	if (bIsDead || (!bDodgeOutOfHitReaction && bIsHitReacting) || bIsDodging || !bCanDodge || bIsExhausted
		|| bIsAttacking || bIsDrinking || bIsUsingSkill || bIsBackAttacking || bIsOpeningDoor
		|| bIsWeaponBouncing || bIsOpeningChest || bIsPlayingItemPickupMontage)
	{
		return;
	}

	if (!UseStamina(10.f))
	{
		return;
	}

	FVector DodgeWorldDir = GetCameraRelativeMoveDirection(LastMoveInput);
	if (DodgeWorldDir.IsNearlyZero())
	{
		DodgeWorldDir = GetActorForwardVector();
	}

	DodgeMoveDirection = DodgeWorldDir.GetSafeNormal();
	DodgeMoveElapsed = 0.f;

	EDodgeDirection DodgeDir = GetDodgeDirection(LastMoveInput);
	UAnimMontage* SelectedDodgeMontage = GetDodgeMontageByDirection(DodgeDir);
	if (!SelectedDodgeMontage)
	{
		return;
	}

	if (bDodgeOutOfHitReaction)
	{
		CancelHitReactionForDodge();
	}

	bIsDodging = true;
	bCanDodge = false;
	bIsInvincible = true;

	bIsRunning = false;
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	GetCharacterMovement()->StopMovementImmediately();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(SelectedDodgeMontage);
	}

	GetWorldTimerManager().SetTimer(
		TimerHandle_DodgeInvincible,
		this,
		&AKnight::EndInvincible,
		DodgeInvincibleTime,
		false
	);

	GetWorldTimerManager().SetTimer(
		TimerHandle_DodgeEnd,
		this,
		&AKnight::EndDodge,
		SelectedDodgeMontage->GetPlayLength(),
		false
	);

	GetWorldTimerManager().SetTimer(
		TimerHandle_DodgeCooldown,
		this,
		&AKnight::ResetDodgeCooldown,
		DodgeCooldown,
		false
	);
}

EDodgeDirection AKnight::GetDodgeDirection(const FVector2D& InputValue) const
{
	if (InputValue.IsNearlyZero())
	{
		return EDodgeDirection::Forward;
	}

	if (FMath::Abs(InputValue.Y) >= FMath::Abs(InputValue.X))
	{
		return (InputValue.Y >= 0.f) ? EDodgeDirection::Forward : EDodgeDirection::Backward;
	}

	return (InputValue.X >= 0.f) ? EDodgeDirection::Right : EDodgeDirection::Left;
}

UAnimMontage* AKnight::GetDodgeMontageByDirection(EDodgeDirection DodgeDir) const
{
	switch (DodgeDir)
	{
	case EDodgeDirection::Forward:
		return DodgeForwardMontage;
	case EDodgeDirection::Backward:
		return DodgeBackwardMontage;
	case EDodgeDirection::Left:
		return DodgeLeftMontage;
	case EDodgeDirection::Right:
		return DodgeRightMontage;
	default:
		return DodgeForwardMontage;
	}
}

void AKnight::StartDodgeMove()
{
	bDodgeMoving = true;
	DodgeMoveElapsed = 0.f;
}

void AKnight::StopDodgeMove()
{
	bDodgeMoving = false;
}

void AKnight::Attack(const FInputActionValue& inputValue)
{
	if (bIsDead || bIsHitReacting || bIsExhausted || bIsDodging || !AttackMontage || bIsDrinking
		|| bIsUsingSkill || bIsBackAttacking || bIsKnockedDownByBoss || bIsOpeningDoor || bIsWeaponBouncing || bIsOpeningChest || bIsPlayingItemPickupMontage)
	{
		return;
	}

	// 백어택 가능하면 일반 공격 대신 백어택
	if (TryStartBackAttack())
	{
		return;
	}

	// 공격 중이면 다음 타 예약만
	if (bIsAttacking)
	{
		if (CurrentComboIndex < 3)
		{
			DrawSwordForCombat();
			bComboInputQueued = true;
		}
		return;
	}

	// 새 공격 시작 또는 콤보 유지시간 안의 후속 공격
	int32 NextComboIndex = 1;

	const bool bCanContinueCombo =
		bIsComboWindowActive &&
		GetWorldTimerManager().IsTimerActive(TimerHandle_ComboReset) &&
		CurrentComboIndex > 0 &&
		CurrentComboIndex < 3;

	if (bCanContinueCombo)
	{
		NextComboIndex = CurrentComboIndex + 1;
	}

	if (!UseStamina(AttackStaminaCost))
	{
		return;
	}

	DrawSwordForCombat();

	GetWorldTimerManager().ClearTimer(TimerHandle_ComboReset);

	bIsAttacking = true;
	bComboInputQueued = false;
	bIsComboWindowActive = false;
	CurrentComboIndex = NextComboIndex;

	PlayAttackSection(CurrentComboIndex);
}

FName AKnight::GetAttackSectionNameByIndex(int32 ComboIndex) const
{
	switch (ComboIndex)
	{
	case 1:
		return FName(TEXT("Attack1"));
	case 2:
		return FName(TEXT("Attack2"));
	case 3:
		return FName(TEXT("Attack3"));
	default:
		return NAME_None;
	}
}

void AKnight::PlayAttackSection(int32 ComboIndex)
{
	if (!AttackMontage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	const FName SectionName = GetAttackSectionNameByIndex(ComboIndex);
	if (SectionName.IsNone())
	{
		return;
	}

	if (!AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		AnimInstance->Montage_Play(AttackMontage);

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AKnight::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontage);
	}

	AnimInstance->Montage_JumpToSection(SectionName, AttackMontage);
}

void AKnight::ResetCombo()
{
	bIsAttacking = false;
	bComboInputQueued = false;
	bIsComboWindowActive = false;
	CurrentComboIndex = 0;
}

float AKnight::GetCurrentAttackDamage() const
{
	// 3번째 공격이면 1.5배 데미지
	if (CurrentComboIndex == 3)
	{
		return AttackPower * 1.5f;
	}

	return AttackPower;
}

void AKnight::OnAttackEnded()
{
	if (!AttackMontage)
	{
		bIsAttacking = false;
		bComboInputQueued = false;
		return;
	}

	// 다음 공격 예약되어 있으면 다음 섹션으로
	if (bComboInputQueued && CurrentComboIndex < 3)
	{
		if (!UseStamina(AttackStaminaCost))
		{
			bIsAttacking = false;
			bComboInputQueued = false;

			GetWorldTimerManager().ClearTimer(TimerHandle_ComboReset);
			GetWorldTimerManager().SetTimer(
				TimerHandle_ComboReset,
				this,
				&AKnight::ResetCombo,
				ComboResetTime,
				false
			);
			return;
		}

		bComboInputQueued = false;
		bIsAttacking = true;
		bIsComboWindowActive = false;
		CurrentComboIndex++;

		GetWorldTimerManager().ClearTimer(TimerHandle_ComboReset);

		PlayAttackSection(CurrentComboIndex);

		return;
	}

	// 예약된 다음타 없으면 공격 종료, 1초 안에 수동 후속입력 허용
	bIsAttacking = false;
	bComboInputQueued = false;
	bIsComboWindowActive = true;

	GetWorldTimerManager().ClearTimer(TimerHandle_ComboReset);
	GetWorldTimerManager().SetTimer(
		TimerHandle_ComboReset,
		this,
		&AKnight::ResetCombo,
		ComboResetTime,
		false
	);

	ScheduleAutoSheatheWeapon();
}

void AKnight::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != AttackMontage)
	{
		return;
	}

	bIsAttacking = false;
	StopWeaponAttackTrail();

	if (bIsComboWindowActive &&
		CurrentComboIndex > 0 &&
		GetWorldTimerManager().IsTimerActive(TimerHandle_ComboReset))
	{
		bIsAttacking = false;
		return;
	}

	bIsAttacking = false;
	bComboInputQueued = false;

	if (bInterrupted)
	{
		ResetCombo();
		return;
	}

	// 3타까지 끝났거나 콤보 윈도우가 없으면 완전히 초기화
	if (!bIsComboWindowActive)
	{
		ResetCombo();
	}
}

void AKnight::PlayerMove()
{
	if (bIsGrabbedByBoss || bIsDodging || bIsHitReacting || bIsUsingSkill || bIsBackAttacking || bIsKnockedDownByBoss ||
		bIsOpeningDoor || bIsGettingUpFromBossKnockDown || bIsWeaponBouncing || bIsOpeningChest || bIsPlayingItemPickupMontage)
	{
		MoveInput = FVector2D::ZeroVector;
		return;
	}

	FVector MoveDir = GetCameraRelativeMoveDirection(MoveInput);

	if (!MoveDir.IsNearlyZero())
	{
		AddMovementInput(MoveDir);
	}

	MoveInput = FVector2D::ZeroVector;
}

void AKnight::StartRun(const FInputActionValue& inputValue)
{
	if (bIsDead || bIsHitReacting || bIsExhausted || bIsAttacking || bIsDodging || bIsDrinking
		|| bIsUsingSkill || bIsKnockedDownByBoss || bIsOpeningDoor || bIsGettingUpFromBossKnockDown
		|| bIsWeaponBouncing || bIsWeaponBouncing || bIsOpeningChest || bIsPlayingItemPickupMontage)
	{
		bIsRunning = false;
		return;
	}

	bIsRunning = true;
	GetCharacterMovement()->MaxWalkSpeed = runSpeed;
}

void AKnight::StopRun(const FInputActionValue& inputValue)
{
	bIsRunning = false;

	if (bIsExhausted)
	{
		GetCharacterMovement()->MaxWalkSpeed = ExhaustedMoveSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	}
}

void AKnight::LockOn()
{
	// 현재 락온 중이면 다음 타겟으로 전환 시도
	if (bIsLockOnActive)
	{
		ABaseMonster* NextTarget = FindNextLockOnTarget();
		if (NextTarget)
		{
			SetLockOnTarget(NextTarget);
		}
		else
		{
			ClearLockOn();
		}
		return;
	}

	// 락온 중이 아니면 새 타겟 찾기
	ABaseMonster* NewTarget = FindBestInitialLockOnTarget();
	if (NewTarget)
	{
		SetLockOnTarget(NewTarget);
	}
}

FVector AKnight::GetCameraRelativeMoveDirection(const FVector2D& InputValue) const
{
	if (bIsLockOnActive)
	{
		return GetLockOnMoveDirection(InputValue);
	}

	FRotator ControlRot = GetControlRotation();
	FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	FVector MoveDir = Forward * InputValue.Y + Right * InputValue.X;
	return MoveDir.GetSafeNormal();
}

void AKnight::StartInvincible()
{
	bIsInvincible = true;
}

void AKnight::EndInvincible()
{
	bIsInvincible = false;
}

void AKnight::EndDodge()
{
	bIsDodging = false;
	bDodgeMoving = false;
	DodgeMoveElapsed = 0.f;
}

void AKnight::ResetDodgeCooldown()
{
	bCanDodge = true;
}

float AKnight::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bIsDead)
	{
		return 0.f;
	}

	// 구르기 / 백어택 / 기상 등 "진짜 무적"은 그대로 유지
	if (bIsInvincible)
	{
		return 0.f;
	}

	// 보스 공격 판정
	bool bIsBossAttack = false;

	if (DamageCauser)
	{
		if (Cast<ATutorialBossMonster>(DamageCauser))
		{
			bIsBossAttack = true;
		}
		else if (Cast<ATutorialBossMonster>(DamageCauser->GetOwner()))
		{
			bIsBossAttack = true;
		}
	}

	if (!bIsBossAttack && EventInstigator)
	{
		if (Cast<ATutorialBossMonster>(EventInstigator->GetPawn()))
		{
			bIsBossAttack = true;
		}
	}

	bool bProjectileAllowsDodgeDuringHitReaction = false;

	if (DamageCauser)
	{
		if (ABossNiagaraProjectile* BossProjectile = Cast<ABossNiagaraProjectile>(DamageCauser))
		{
			bProjectileAllowsDodgeDuringHitReaction =
				BossProjectile->ShouldAllowDodgeDuringPlayerHitReaction();
		}
	}

	// 강공 별도 판정은 기존 유지
	const bool bIsBossPowerAttack =
		DamageEvent.DamageTypeClass &&
		DamageEvent.DamageTypeClass->IsChildOf(UBossPowerAttackDamageType::StaticClass());

	// 일반 몬스터 공격에만 짧은 피격 무적 적용
	// 이 경우 데미지도 안 받고, 피격 모션도 안 나감
	if (!bIsBossAttack && bHitReactionInvincible)
	{
		return 0.f;
	}

	const float FinalDamage = FMath::Max(0.f, DamageAmount);
	CurrentHP = FMath::Clamp(CurrentHP - FinalDamage, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP);

	if (CurrentHP <= 0.f)
	{
		const bool bKilledByKillZone =
			DamageCauser && DamageCauser->ActorHasTag(TEXT("KillZone"));

		HandleDeath(bKilledByKillZone);
		return FinalDamage;
	}

	// 스킬 중 슈퍼아머
	if (bHasSuperArmor)
	{
		return FinalDamage;
	}

	if (bIsKnockedDownByBoss)
	{
		return FinalDamage;
	}

	// 보스 강공은 기존대로 녹다운 우선
	if (bIsBossPowerAttack)
	{
		EnterBossKnockDown(DamageCauser);
		return FinalDamage;
	}

	InterruptCurrentAction();
	bIsHitReacting = true;
	bCanDodgeDuringHitReaction = bProjectileAllowsDodgeDuringHitReaction;

	// 일반 몬스터 공격일 때만 짧은 피격 무적 시작
	if (!bIsBossAttack)
	{
		bHitReactionInvincible = true;

		GetWorldTimerManager().ClearTimer(TimerHandle_HitInvincible);
		GetWorldTimerManager().SetTimer(
			TimerHandle_HitInvincible,
			this,
			&AKnight::EndHitInvincible,
			HitInvincibleTime,
			false
		);
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (HitReactMontage)
		{
			AnimInstance->Montage_Play(HitReactMontage);

			GetWorldTimerManager().ClearTimer(TimerHandle_EndHitReact);
			GetWorldTimerManager().SetTimer(
				TimerHandle_EndHitReact,
				this,
				&AKnight::EndHitReact,
				HitReactMontage->GetPlayLength(),
				false
			);
		}
		else
		{
			bIsHitReacting = false;
			bCanDodgeDuringHitReaction = false;
		}
	}

	return FinalDamage;
}

void AKnight::Heal(float Amount)
{
	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.f, GetEffectiveMaxHP());
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
}

void AKnight::RecoverStamina(float Amount)
{
	if (bIsExhausted)
	{
		return;
	}

	CurrentStamina = FMath::Clamp(CurrentStamina + Amount, 0.f, MaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

bool AKnight::UseStamina(float Amount)
{
	if (CurrentStamina < Amount)
	{
		return false;
	}

	CurrentStamina = FMath::Clamp(CurrentStamina - Amount, 0.f, MaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	return true;
}

void AKnight::UpdateStamina(float DeltaTime)
{
	if (bIsExhausted)
	{
		return;
	}

	const bool bIsActuallyMoving = GetVelocity().SizeSquared2D() > 1.0f; // 이동 중인지 확인
	const float OldStamina = CurrentStamina; // 스태미나 변화 전 값 저장

	// 공격 중이나 구르기 중에는 회복/소모 x
	const bool bInAttackComboState =
		bIsAttacking ||
		bIsComboWindowActive ||
		GetWorldTimerManager().IsTimerActive(TimerHandle_ComboReset) ||
		CurrentComboIndex > 0;

	if (bInAttackComboState || bIsDodging)
	{
		return;
	}

	// 달리기 중이고 실제로 이동 중인 경우 스태미나 감소, 그렇지 않으면 스태미나 회복
	if (bIsRunning && bIsActuallyMoving)
	{
		CurrentStamina -= RunStaminaDrainRate * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);

		if (!FMath::IsNearlyEqual(OldStamina, CurrentStamina))
		{
			OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
		}

		if (CurrentStamina <= 0.f)
		{
			EnterExhaustedState();
		}
	}
	else
	{
		CurrentStamina += StaminaRegenRate * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);

		if (!FMath::IsNearlyEqual(OldStamina, CurrentStamina))
		{
			OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
		}
	}
}

void AKnight::EnterExhaustedState()
{
	if (bIsExhausted)
	{
		return;
	}

	bIsExhausted = true;
	bIsRunning = false;
	CurrentStamina = 0.f;
	ExhaustedRemainingTime = ExhaustedDuration;

	GetCharacterMovement()->MaxWalkSpeed = ExhaustedMoveSpeed;

	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	OnExhaustedChanged.Broadcast(true);
	OnExhaustedTimeChanged.Broadcast(ExhaustedRemainingTime);
	StartExhaustedEffect();

	GetWorldTimerManager().SetTimer(
		TimerHandle_Exhausted,
		this,
		&AKnight::EndExhaustedState,
		ExhaustedDuration,
		false
	);

	GetMesh()->GetAnimInstance()->Montage_Play(ExhaustedMontage);
}

void AKnight::EndExhaustedState()
{
	bIsExhausted = false;
	ExhaustedRemainingTime = 0.f;
	CurrentStamina = FMath::Clamp(ExhaustedRecoveryStamina, 0.f, MaxStamina);

	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	StopExhaustedEffect();

	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	OnExhaustedChanged.Broadcast(false);
	OnExhaustedTimeChanged.Broadcast(0.f);

	GetMesh()->GetAnimInstance()->Montage_Stop(0.1f, ExhaustedMontage);

	StartSheatheWeapon();
}

void AKnight::StartExhaustedEffect()
{
	if (!ExhaustedEffect)
	{
		return;
	}

	// 이미 켜져 있으면 중복 생성 방지
	if (ExhaustedEffectComp)
	{
		return;
	}

	ExhaustedEffectComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		ExhaustedEffect,
		GetMesh(),                 // 메시 기준으로 붙이기
		NAME_None,                 // 소켓 없으면 None
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true,                      // AutoDestroy
		true                       // AutoActivate
	);
}

void AKnight::StopExhaustedEffect()
{
	if (!ExhaustedEffectComp)
	{
		return;
	}

	ExhaustedEffectComp->Deactivate();
	ExhaustedEffectComp->DestroyComponent();
	ExhaustedEffectComp = nullptr;
}

void AKnight::StartAttackTrace()
{
	bAttackTraceActive = true;
	AlreadyHitActors.Empty();
}

void AKnight::EndAttackTrace()
{
	bAttackTraceActive = false;
	AlreadyHitActors.Empty();
}

void AKnight::PerformAttackTrace()
{
	if (!swordMeshComp)
	{
		return;
	}

	const FVector TraceStart = swordMeshComp->GetSocketLocation(TEXT("TraceStart"));
	const FVector TraceEnd = swordMeshComp->GetSocketLocation(TEXT("TraceEnd"));

	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	const bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		this,
		TraceStart,
		TraceEnd,
		AttackTraceRadius,
		UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel2), // Monster
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	float ClosestMonsterDistance = TNumericLimits<float>::Max();

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == this)
			{
				continue;
			}

			if (!HitActor->ActorHasTag(TEXT("Monster")))
			{
				continue;
			}

			ClosestMonsterDistance = FMath::Min(ClosestMonsterDistance, Hit.Distance);
		}
	}

	FHitResult WorldBounceHit;
	if (FindWeaponBounceHit(TraceStart, TraceEnd, ActorsToIgnore, WorldBounceHit))
	{
		if (WorldBounceHit.Distance <= ClosestMonsterDistance)
		{
			TriggerWeaponBounce(WorldBounceHit);
			return;
		}
	}

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActor == this)
			{
				continue;
			}

			if (!HitActor->ActorHasTag(TEXT("Monster")))
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
				GetCurrentAttackDamage(),
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

				PlayPlayerAttackHitSuccessSoundAtLocation(HitLocation);

				// 최종보스는 맞아도 피 이펙트 출력 X
				const bool bIsFinalBoss =
					Cast<AFinalBossPhase1>(HitActor) ||
					Cast<AFinalBossPhase2>(HitActor);

				if (!bIsFinalBoss)
				{
					SpawnPlayerAttackHitBloodEffect(HitLocation, HitRotation);
				}
			}
		}
	}
}

bool AKnight::FindWeaponBounceHit(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const TArray<AActor*>& ActorsToIgnore,
	FHitResult& OutHit
) const
{
	if (!bEnableWeaponBounce)
	{
		return false;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	// 벽, 바닥, 고정 오브젝트
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	// 움직이는 물건까지 튕기게 하고 싶으면 유지
	// 필요 없으면 이 줄은 지워도 됨
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	TArray<FHitResult> WorldHits;

	const bool bHitWorld = UKismetSystemLibrary::SphereTraceMultiForObjects(
		this,
		TraceStart,
		TraceEnd,
		AttackTraceRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		WorldHits,
		true
	);

	if (!bHitWorld)
	{
		return false;
	}

	float BestDistance = TNumericLimits<float>::Max();
	bool bFound = false;

	for (const FHitResult& Hit : WorldHits)
	{
		AActor* HitActor = Hit.GetActor();
		UPrimitiveComponent* HitComp = Hit.GetComponent();

		if (!HitActor || !HitComp || HitActor == this)
		{
			continue;
		}

		// 몬스터/플레이어는 튕김 대상에서 제외
		if (HitActor->ActorHasTag(TEXT("Monster")) || HitActor->ActorHasTag(TEXT("Player")))
		{
			continue;
		}

		// 튕기면 안 되는 액터는 태그로 제외 가능
		if (HitActor->ActorHasTag(TEXT("NoWeaponBounce")))
		{
			continue;
		}

		// 실제 막히는 충돌만 튕김 처리
		if (!Hit.bBlockingHit)
		{
			continue;
		}

		if (Hit.Distance < BestDistance)
		{
			BestDistance = Hit.Distance;
			OutHit = Hit;
			bFound = true;
		}
	}

	return bFound;
}

void AKnight::TriggerWeaponBounce(const FHitResult& Hit)
{
	if (bIsWeaponBouncing || bIsDead)
	{
		return;
	}

	bIsWeaponBouncing = true;
	PlayWeaponBounceSound();

	// 현재 공격 판정 중지
	bAttackTraceActive = false;
	AlreadyHitActors.Empty();

	// 콤보/공격 상태 초기화
	bIsAttacking = false;
	bComboInputQueued = false;
	CurrentComboIndex = 0;
	GetWorldTimerManager().ClearTimer(TimerHandle_ComboReset);

	// 이동 정지
	bIsRunning = false;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

	float BounceDuration = WeaponBounceFallbackTime;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		// 공격 몽타주 끊고 튕김 몽타주 재생
		if (AttackMontage && AnimInstance->Montage_IsPlaying(AttackMontage))
		{
			AnimInstance->Montage_Stop(0.05f, AttackMontage);
		}

		if (WeaponBounceMontage)
		{
			AnimInstance->Montage_Play(WeaponBounceMontage);
			BounceDuration = WeaponBounceMontage->GetPlayLength();
		}
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_WeaponBounce);
	GetWorldTimerManager().SetTimer(
		TimerHandle_WeaponBounce,
		this,
		&AKnight::EndWeaponBounce,
		BounceDuration,
		false
	);
}

void AKnight::EndWeaponBounce()
{
	if (bIsDead)
	{
		return;
	}

	bIsWeaponBouncing = false;
}

void AKnight::InterruptCurrentAction()
{
	// 이동, 행동 상태 강제 해제
	bIsRunning = false;
	bIsDodging = false;
	bIsAttacking = false;
	bComboInputQueued = false;
	bAttackTraceActive = false;
	bCanDodge = true;
	bDodgeMoving = false;
	DodgeMoveElapsed = 0.f;
	DodgeMoveDirection = FVector::ZeroVector;
	bIsDrinking = false;
	bIsUsingSkill = false;
	bHasSuperArmor = false;
	bSkillMoving = false;
	SkillMoveElapsed = 0.f;
	SkillMoveDirection = FVector::ZeroVector;
	ActiveSkillMontage = nullptr;
	ActiveSkillRadius = SkillRadius;
	ActiveSkillForwardDistance = SkillForwardDistance;
	ActiveSkillMoveDuration = SkillMoveDuration;
	ActiveSkillTriggerTimeRatio = 0.5f;
	bIsBackAttacking = false;
	BackAttackTarget = nullptr;
	bIsOpeningDoor = false;
	bCanGetUpFromBossKnockDown = false;
	StopWeaponAttackTrail();

	GetWorldTimerManager().ClearTimer(TimerHandle_BossGetUpDelay);
	GetWorldTimerManager().ClearTimer(TimerHandle_DoorInteraction);
	GetWorldTimerManager().ClearTimer(TimerHandle_Drink);
	GetWorldTimerManager().ClearTimer(TimerHandle_BackAttackInvincible);
	GetWorldTimerManager().ClearTimer(TimerHandle_BackAttackEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_ComboReset);
	GetWorldTimerManager().ClearTimer(TimerHandle_DodgeInvincible);
	GetWorldTimerManager().ClearTimer(TimerHandle_DodgeEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_DodgeCooldown);
	GetWorldTimerManager().ClearTimer(TimerHandle_Skill);
	GetWorldTimerManager().ClearTimer(TimerHandle_SkillEnd);
	StopShowDrinkEffect();
	AlreadyHitActors.Empty();
	GetWorldTimerManager().ClearTimer(TimerHandle_AutoSheatheWeapon);
	GetWorldTimerManager().ClearTimer(TimerHandle_SheatheWeapon);
	bIsSheathingWeapon = false;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (AttackMontage && AnimInstance->Montage_IsPlaying(AttackMontage))
		{
			AnimInstance->Montage_Stop(0.1f, AttackMontage);
		}

		if (DodgeForwardMontage && AnimInstance->Montage_IsPlaying(DodgeForwardMontage))
		{
			AnimInstance->Montage_Stop(0.1f, DodgeForwardMontage);
		}

		if (DodgeBackwardMontage && AnimInstance->Montage_IsPlaying(DodgeBackwardMontage))
		{
			AnimInstance->Montage_Stop(0.1f, DodgeBackwardMontage);
		}

		if (DodgeLeftMontage && AnimInstance->Montage_IsPlaying(DodgeLeftMontage))
		{
			AnimInstance->Montage_Stop(0.1f, DodgeLeftMontage);
		}

		if (DodgeRightMontage && AnimInstance->Montage_IsPlaying(DodgeRightMontage))
		{
			AnimInstance->Montage_Stop(0.1f, DodgeRightMontage);
		}

		if (ExhaustedMontage && AnimInstance->Montage_IsPlaying(ExhaustedMontage))
		{
			AnimInstance->Montage_Stop(0.1f, ExhaustedMontage);
		}

		if (DrinkMontage && AnimInstance->Montage_IsPlaying(DrinkMontage))
		{
			AnimInstance->Montage_Stop(0.1f, DrinkMontage);
		}

		if (ActiveSkillMontage && AnimInstance->Montage_IsPlaying(ActiveSkillMontage))
		{
			AnimInstance->Montage_Stop(0.1f, ActiveSkillMontage);
		}
		else if (SkillMontage && AnimInstance->Montage_IsPlaying(SkillMontage))
		{
			AnimInstance->Montage_Stop(0.1f, SkillMontage);
		}

		if (BackAttackMontage && AnimInstance->Montage_IsPlaying(BackAttackMontage))
		{
			AnimInstance->Montage_Stop(0.1f, BackAttackMontage);
		}
	}

	if (PendingChestToOpen)
	{
		PendingChestToOpen->CancelPendingChestOpen(this);
		PendingChestToOpen = nullptr;
	}

	const bool bWasOpeningChest = bIsOpeningChest;

	bIsOpeningChest = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_ChestOpen);
	GetWorldTimerManager().ClearTimer(TimerHandle_ChestOpenTrigger);
	bChestOpenTriggered = false;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}

	if (bWeaponDrawn)
	{
		ScheduleAutoSheatheWeapon();
	}

	UpdateSwordVisibilityByState();
}

void AKnight::EndHitReact()
{
	if (bIsDead)
	{
		return;
	}

	bIsHitReacting = false;
	bCanDodgeDuringHitReaction = false;
}

void AKnight::CancelHitReactionForDodge()
{
	if (!bIsHitReacting || !bCanDodgeDuringHitReaction)
	{
		return;
	}

	bIsHitReacting = false;
	bCanDodgeDuringHitReaction = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_EndHitReact);

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (HitReactMontage && AnimInstance->Montage_IsPlaying(HitReactMontage))
		{
			AnimInstance->Montage_Stop(0.05f, HitReactMontage);
		}
	}
}

void AKnight::HandleDeath(bool bUseSafeDropFallback)
{
	if (bIsDead)
	{
		return;
	}

	IncreaseDeathHPPenalty();
	ClearLockOn();

	bIsDead = true;
	bIsHitReacting = false;
	bIsInvincible = true;

	InterruptCurrentAction();
	DropWeaponOnDeath();
	DropMemoryShardsOnDeath(bUseSafeDropFallback);

	PlayDeathSound();

	// 몬스터가 더 이상 플레이어로 인식하지 않게
	Tags.Remove(TEXT("Player"));
	Tags.Add(TEXT("DeadPlayer"));

	GetCharacterMovement()->DisableMovement();
	DisableInput(Cast<APlayerController>(GetController()));
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (DeathMontage)
		{
			AnimInstance->Montage_Play(DeathMontage);
		}
	}

	if (GameOverWidgetClass && !GameOverWidget)
	{
		GameOverWidget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
		if (GameOverWidget)
		{
			GameOverWidget->AddToViewport(100);
		}
	}

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		if (!GI->HasSeenTutorial(DeathPenaltyTutorialID))
		{
			GI->SetPendingDeathPenaltyTutorial(true);
		}
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		if (GameOverWidget)
		{
			InputMode.SetWidgetToFocus(GameOverWidget->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);

		PC->SetIgnoreLookInput(true);
		PC->SetIgnoreMoveInput(true);
	}
}

void AKnight::EndHitInvincible()
{
	if (bIsDead)
	{
		return;
	}

	bHitReactionInvincible = false;
}

TArray<ABaseMonster*> AKnight::FindLockOnCandidates() const
{
	TArray<ABaseMonster*> Result;

	UWorld* World = GetWorld();
	if (!World)
	{
		return Result;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, ABaseMonster::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
		if (!Monster)
		{
			continue;
		}

		if (Monster->IsDead())
		{
			continue;
		}

		if (!Monster->ActorHasTag(TEXT("Monster")))
		{
			continue;
		}

		if (!Monster->CanBeLockedOn())
		{
			continue;
		}

		const float Dist = FVector::Dist(GetActorLocation(), Monster->GetActorLocation());
		if (Dist > LockOnSearchRadius)
		{
			continue;
		}

		Result.Add(Monster);
	}

	return Result;
}

ABaseMonster* AKnight::FindBestInitialLockOnTarget() const
{
	TArray<ABaseMonster*> Candidates = FindLockOnCandidates();

	ABaseMonster* BestTarget = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (ABaseMonster* Monster : Candidates)
	{
		const float DistSq = FVector::DistSquared(GetActorLocation(), Monster->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Monster;
		}
	}

	return BestTarget;
}

ABaseMonster* AKnight::FindNextLockOnTarget() const
{
	TArray<ABaseMonster*> Candidates = FindLockOnCandidates();

	ABaseMonster* BestTarget = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (ABaseMonster* Monster : Candidates)
	{
		if (Monster == LockOnTarget)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Monster->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Monster;
		}
	}

	return BestTarget;
}

void AKnight::SetLockOnTarget(ABaseMonster* NewTarget)
{
	if (LockOnTarget)
	{
		LockOnTarget->SetLockOnVisible(false);
	}

	LockOnTarget = NewTarget;
	bIsLockOnActive = (LockOnTarget != nullptr);

	if (LockOnTarget)
	{
		LockOnTarget->SetLockOnVisible(true);
		ApplyLockOnCamera();

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = false;
		}

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FRotator CRot = PC->GetControlRotation();
			CRot.Pitch = 0.f;
			CRot.Roll = 0.f;
			PC->SetControlRotation(CRot);
		}
	}
	else
	{
		ClearLockOn();
	}
}

void AKnight::ClearLockOn()
{
	if (LockOnTarget)
	{
		LockOnTarget->SetLockOnVisible(false);
	}

	LockOnTarget = nullptr;
	bIsLockOnActive = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	ApplyNormalCamera();
}

void AKnight::UpdateLockOn(float DeltaTime)
{
	if (!LockOnTarget || LockOnTarget->IsDead() || !LockOnTarget->CanBeLockedOn())
	{
		ClearLockOn();
		return;
	}

	const FVector MyLoc = GetActorLocation();
	const FVector MonsterLoc = LockOnTarget->GetActorLocation();
	const FVector MonsterForward = LockOnTarget->GetActorForwardVector();

	const FVector FocusPoint = MonsterLoc - MonsterForward * 40.f + FVector(0.f, 0.f, 40.f);

	FVector ToFocus = FocusPoint - MyLoc;
	ToFocus.Z = 0.f;

	// 락온 타겟이 너무 가까우면 회전 안 함
	if (!ToFocus.IsNearlyZero())
	{
		const FRotator TargetRot = ToFocus.Rotation();

		const FRotator NewActorRot = FMath::RInterpTo(
			GetActorRotation(),
			FRotator(0.f, TargetRot.Yaw, 0.f),
			DeltaTime,
			10.f
		);

		SetActorRotation(NewActorRot);

		// 컨트롤러 회전도 부드럽게 따라가도록
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			const FRotator CurrentControlRot = PC->GetControlRotation();
			const FRotator DesiredControlRot(0.f, TargetRot.Yaw, 0.f);

			const FRotator SmoothedControlRot = FMath::RInterpTo(
				CurrentControlRot,
				DesiredControlRot,
				DeltaTime,
				8.f
			);

			PC->SetControlRotation(SmoothedControlRot);
		}
	}
}

FVector AKnight::GetLockOnMoveDirection(const FVector2D& InputValue) const
{
	if (!LockOnTarget)
	{
		return FVector::ZeroVector;
	}

	FVector ToTarget = LockOnTarget->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	ToTarget = ToTarget.GetSafeNormal();

	if (ToTarget.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector Forward = ToTarget;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();

	FVector MoveDir = Forward * InputValue.Y + Right * InputValue.X;
	return MoveDir.GetSafeNormal();
}

void AKnight::OnLockOnPressed()
{
	bLockOnButtonHeld = true;
	bLockOnHoldTriggered = false;

	GetWorldTimerManager().SetTimer(
		TimerHandle_LockOnHold,
		this,
		&AKnight::HandleLockOnHold,
		0.35f,
		false
	);
}

void AKnight::OnLockOnReleased()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_LockOnHold);

	// 길게 누르기 발동 안 했으면 짧게 누른 것 = 기존 락온 동작
	if (!bLockOnHoldTriggered)
	{
		LockOn();
	}

	bLockOnButtonHeld = false;
}

void AKnight::HandleLockOnHold()
{
	if (!bLockOnButtonHeld)
	{
		return;
	}

	bLockOnHoldTriggered = true;
	ClearLockOn();
}

void AKnight::StartShowDrinkEffect()
{
	if (!DrinkEffect)
	{
		return;
	}

	if (DrinkEffectComp)
	{
		return;
	}

	DrinkEffectComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		DrinkEffect,
		GetMesh(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		true,  // AutoDestroy
		true   // AutoActivate
	);
}

void AKnight::StopShowDrinkEffect()
{
	if (!DrinkEffectComp)
	{
		return;
	}

	DrinkEffectComp->Deactivate();
	DrinkEffectComp->DestroyComponent();
	DrinkEffectComp = nullptr;
}

void AKnight::DrinkPotion()
{
	if (bIsDead || bIsHitReacting || bIsExhausted || bIsDodging || bIsAttacking || bIsDrinking
		|| bIsBackAttacking || bIsKnockedDownByBoss || bIsOpeningDoor || bIsGettingUpFromBossKnockDown
		|| bIsWeaponBouncing || bIsOpeningChest || bIsPlayingItemPickupMontage)
	{
		return;
	}
	if (CurrentPotionCount <= 0 || CurrentHP == MaxHP)
	{
		return;
	}
	bIsDrinking = true;
	UpdateSwordVisibilityByState();

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = DrinkSpeed;
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (DrinkMontage)
		{

			AnimInstance->Montage_Play(DrinkMontage);
			GetWorldTimerManager().SetTimer(
				TimerHandle_Drink,
				[this]()
				{
					Heal(PotionHealAmount);
					PlayPotionHealSound();
					StartShowDrinkEffect(); // 회복 이펙트

					FTimerHandle DrinkEffectHandle;
					GetWorldTimerManager().SetTimer(
						DrinkEffectHandle,
						this,
						&AKnight::StopShowDrinkEffect,
						1.0f, // 이펙트 지속 시간
						false
					);

					bIsDrinking = false;
					CurrentPotionCount--; // 포션 개수 감소
					GetCharacterMovement()->MaxWalkSpeed = walkSpeed; // 마신 후 이동 속도 원래대로

					UpdateSwordVisibilityByState();
				},
				DrinkMontage->GetPlayLength(),
				false
			);
		}
	}
}

void AKnight::Interact()
{
	if (bIsDead || bIsHitReacting || bIsDodging || bIsBonfireInteracting || bIsRestingAtBonfire
		|| bIsKnockedDownByBoss || bIsOpeningDoor || bIsGettingUpFromBossKnockDown
		|| bIsWeaponBouncing || bIsOpeningChest || bIsPlayingItemPickupMontage)
	{
		return;
	}

	if (!CurrentInteractActor)
	{
		return;
	}

	if (!CurrentInteractActor->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
	{
		return;
	}

	if (!IInteractInterface::Execute_CanInteract(CurrentInteractActor, this))
	{
		return;
	}

	IInteractInterface::Execute_Interact(CurrentInteractActor, this);
}

void AKnight::SetInteractActor(AActor* NewInteractActor)
{
	CurrentInteractActor = NewInteractActor;
}

void AKnight::ClearInteractActor(AActor* InteractActorToClear)
{
	if (CurrentInteractActor == InteractActorToClear)
	{
		CurrentInteractActor = nullptr;
	}
}

void AKnight::RefreshInteractActor()
{
	CurrentInteractActor = nullptr;

	if (!GetWorld())
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(InteractTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldDynamic),
		SphereShape,
		QueryParams
	);

	if (!bHit)
	{
		return;
	}

	AActor* BestActor = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Actor = Result.GetActor();
		if (!Actor || Actor == this || Actor->IsPendingKillPending())
		{
			continue;
		}

		if (!Actor->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
		{
			continue;
		}

		if (!IInteractInterface::Execute_CanInteract(Actor, this))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestActor = Actor;
		}
	}

	CurrentInteractActor = BestActor;
}

void AKnight::RefreshItemDropInteractActor()
{
	CurrentInteractActor = nullptr;

	if (!GetWorld())
	{
		return;
	}

	TArray<FOverlapResult> OverlapResults;

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(InteractTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldDynamic),
		SphereShape,
		QueryParams
	);

	if (!bHit)
	{
		return;
	}

	AActor* BestActor = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Actor = Result.GetActor();
		if (!Actor || Actor == this || Actor->IsPendingKillPending())
		{
			continue;
		}

		// 아이템 드랍 액터만 다시 잡는다.
		AItemDropActor* DropActor = Cast<AItemDropActor>(Actor);
		if (!DropActor)
		{
			continue;
		}

		if (!DropActor->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
		{
			continue;
		}

		if (!IInteractInterface::Execute_CanInteract(DropActor, this))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), DropActor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestActor = DropActor;
		}
	}

	CurrentInteractActor = BestActor;
}

void AKnight::BeginBonfireInteract(ABonfire* Bonfire)
{
	if (!Bonfire || bIsDead || bIsHitReacting || bIsDodging || bIsAttacking
		|| bIsDrinking || bIsBonfireInteracting || bIsRestingAtBonfire || bIsBackAttacking || bIsWeaponBouncing)
	{
		return;
	}

	bIsBonfireInteracting = true;
	CurrentBonfire = Bonfire;
	InterruptCurrentAction();

	UpdateSwordVisibilityByState();

	GetCharacterMovement()->StopMovementImmediately();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (BonfireInteractMontage)
		{
			AnimInstance->Montage_Play(BonfireInteractMontage);

			GetWorldTimerManager().SetTimer(
				TimerHandle_BonfireInteract,
				this,
				&AKnight::FinishBonfireInteract,
				BonfireInteractMontage->GetPlayLength(),
				false
			);
		}
		else
		{
			FinishBonfireInteract();
		}
	}
}

void AKnight::FinishBonfireInteract()
{
	if (!CurrentBonfire)
	{
		bIsBonfireInteracting = false;
		UpdateSwordVisibilityByState();
		return;
	}

	CurrentBonfire->OnPlayerFinishedInteract(this);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	CurrentBonfire = nullptr;
	bIsBonfireInteracting = false;
	UpdateSwordVisibilityByState();
}

void AKnight::StartBonfireRest(ABonfire* Bonfire)
{
	if (!Bonfire)
	{
		return;
	}

	bIsRestingAtBonfire = true;
	UpdateSwordVisibilityByState();
	CurrentBonfire = Bonfire;

	const FTransform RestTransform = Bonfire->GetRestTransform();
	const FRotator RestYawRot(0.f, RestTransform.GetRotation().Rotator().Yaw, 0.f);

	SetActorLocationAndRotation(RestTransform.GetLocation(), RestYawRot);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(RestYawRot);
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (BonfireSitMontage)
		{
			AnimInstance->Montage_Play(BonfireSitMontage);
		}
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetIgnoreLookInput(true);
		PC->SetIgnoreMoveInput(true);
	}

	SetPlayerHUDVisible(false);

	if (BonfireMenuWidgetClass && !BonfireMenuWidget)
	{
		BonfireMenuWidget = CreateWidget<UUserWidget>(GetWorld(), BonfireMenuWidgetClass);
		if (BonfireMenuWidget)
		{
			BonfireMenuWidget->AddToViewport(200);
		}
	}
}

void AKnight::RestAtBonfire()
{
	if (!bIsRestingAtBonfire)
	{
		return;
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AKnight::OnRestFinished);
}

void AKnight::OnRestFinished()
{
	CurrentHP = GetEffectiveMaxHP();
	CurrentStamina = MaxStamina;
	CurrentPotionCount = MaxPotionCount;

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	OnHPPenaltyChanged.Broadcast(CurrentHPPenaltyPercent);

	TArray<AActor*> FoundMonsters;
	UGameplayStatics::GetAllActorsOfClass(this, ABaseMonster::StaticClass(), FoundMonsters);

	for (AActor* Actor : FoundMonsters)
	{
		ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
		if (!Monster)
		{
			continue;
		}

		// 보스 제외
		if (Cast<ATutorialBossMonster>(Monster))
		{
			continue;
		}

		// 화톳불 휴식 시 부활 가능한 일반 몬스터만
		if (Monster->ShouldRespawnOnBonfireRest())
		{
			if (Monster->IsDead())
			{
				Monster->RespawnMonster();
			}
			else
			{
				Monster->ResetToInitialStateOnBonfireRest();
			}
		}
	}
}

void AKnight::TravelToBonfire(FName TargetLevelName, const FTransform& TargetTransform)
{
	if (TargetLevelName.IsNone())
	{
		return;
	}

	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));

	EndBonfireRest();

	// 같은 레벨 이동은 pending travel을 만들지 않고 즉시 이동
	if (TargetLevelName == CurrentLevelName)
	{
		const FRotator TravelYawRot(0.f, TargetTransform.GetRotation().Rotator().Yaw, 0.f);

		SetActorLocationAndRotation(
			TargetTransform.GetLocation(),
			TravelYawRot,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetControlRotation(TravelYawRot);
		}

		return;
	}

	// 다른 레벨 이동일 때만 pending travel 저장
	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		GI->SetPendingBonfireTravel(TargetLevelName, TargetTransform);
		GI->SetPendingLoadingScreen(2.0f);
	}

	SaveInventoryToGameInstance();
	SaveEnhancementToGameInstance();

	UGameplayStatics::OpenLevel(this, TargetLevelName);
}

void AKnight::EndBonfireRest()
{
	bIsRestingAtBonfire = false;
	bIsBonfireInteracting = false;
	CurrentBonfire = nullptr;

	if (BonfireMenuWidget)
	{
		BonfireMenuWidget->RemoveFromParent();
		BonfireMenuWidget = nullptr;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetIgnoreLookInput(false);
		PC->SetIgnoreMoveInput(false);
	}

	StopAnimMontage(BonfireSitMontage);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	SetPlayerHUDVisible(true);

	UpdateSwordVisibilityByState();
}

void AKnight::LeaveBonfireRest()
{
	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		GI->ClearPendingBonfireTravel();
	}

	EndBonfireRest();
}

void AKnight::SetPlayerHUDVisible(bool bVisible)
{
	if (!PlayerWidget)
	{
		return;
	}

	PlayerWidget->SetVisibility(
		bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed
	);
}

void AKnight::ResolveCurrentWeaponSkillSetting(
	UAnimMontage*& OutMontage,
	float& OutRadius,
	float& OutForwardDistance,
	float& OutMoveDuration,
	float& OutTriggerTimeRatio
) const
{
	// 기본값
	OutMontage = SkillMontage;
	OutRadius = SkillRadius;
	OutForwardDistance = SkillForwardDistance;
	OutMoveDuration = SkillMoveDuration;
	OutTriggerTimeRatio = 0.5f;

	if (!InventoryComp)
	{
		return;
	}

	const FInventoryItem& EquippedWeapon = InventoryComp->GetEquippedWeapon();

	if (!EquippedWeapon.IsValid() || EquippedWeapon.ItemData.EquipSlot != EEquipmentSlot::Weapon)
	{
		return;
	}

	const FItemData& ItemData = EquippedWeapon.ItemData;

	if (ItemData.WeaponSkillMontage)
	{
		OutMontage = ItemData.WeaponSkillMontage.Get();
	}

	if (ItemData.WeaponSkillRadius >= 0.f)
	{
		OutRadius = ItemData.WeaponSkillRadius;
	}

	if (ItemData.WeaponSkillForwardDistance >= 0.f)
	{
		OutForwardDistance = ItemData.WeaponSkillForwardDistance;
	}

	if (ItemData.WeaponSkillMoveDuration >= 0.f)
	{
		OutMoveDuration = ItemData.WeaponSkillMoveDuration;
	}

	OutTriggerTimeRatio = FMath::Clamp(ItemData.WeaponSkillTriggerTimeRatio, 0.f, 1.f);
}

void AKnight::UseSkill(const FInputActionValue& inputValue)
{
	if (bIsDead || bIsHitReacting || bIsDodging || bIsAttacking || bIsDrinking
		|| bIsBonfireInteracting || bIsRestingAtBonfire || bIsUsingSkill || bIsBackAttacking
		|| bIsKnockedDownByBoss || bIsOpeningDoor || bIsGettingUpFromBossKnockDown || bIsWeaponBouncing || bIsOpeningChest || bIsPlayingItemPickupMontage)
	{
		return;
	}

	if (!InventoryComp)
	{
		return;
	}

	UAnimMontage* MontageToPlay = nullptr;
	float ResolvedSkillRadius = 0.f;
	float ResolvedSkillForwardDistance = 0.f;
	float ResolvedSkillMoveDuration = 0.f;
	float ResolvedSkillTriggerTimeRatio = 0.5f;

	ResolveCurrentWeaponSkillSetting(
		MontageToPlay,
		ResolvedSkillRadius,
		ResolvedSkillForwardDistance,
		ResolvedSkillMoveDuration,
		ResolvedSkillTriggerTimeRatio
	);

	if (!MontageToPlay)
	{
		return;
	}

	// 스킬석 부족하면 사용 불가
	if (InventoryComp->GetItemCountByID(SkillStoneItemID) < SkillStoneCost)
	{
		return;
	}

	// 스태미나 부족하면 사용 불가
	if (!UseStamina(SkillStaminaCost))
	{
		return;
	}

	// 스킬석 소모
	if (!InventoryComp->ConsumeItemByID(SkillStoneItemID, SkillStoneCost))
	{
		return;
	}

	InterruptCurrentAction();

	DrawSwordForCombat();

	ActiveSkillMontage = MontageToPlay;
	ActiveSkillRadius = ResolvedSkillRadius;
	ActiveSkillForwardDistance = ResolvedSkillForwardDistance;
	ActiveSkillMoveDuration = FMath::Max(0.01f, ResolvedSkillMoveDuration);
	ActiveSkillTriggerTimeRatio = ResolvedSkillTriggerTimeRatio;

	bIsUsingSkill = true;
	bHasSuperArmor = true;

	PlaySkillCastSound();
	PlaySkillVoiceSound();

	GetCharacterMovement()->StopMovementImmediately();

	// 전진 방향 계산
	FVector SkillMoveDir = GetActorForwardVector();

	if (bIsLockOnActive && LockOnTarget && !LockOnTarget->IsDead())
	{
		FVector ToTarget = LockOnTarget->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;
		SkillMoveDir = ToTarget.GetSafeNormal();
	}

	// 전진 이동
	SkillMoveDirection = SkillMoveDir.GetSafeNormal();
	bSkillMoving = true;
	SkillMoveElapsed = 0.f;

	// 락온 중이면 대상 쪽으로 정렬
	if (!SkillMoveDir.IsNearlyZero())
	{
		const FRotator SkillYawRot = SkillMoveDir.Rotation();
		SetActorRotation(FRotator(0.f, SkillYawRot.Yaw, 0.f));

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetControlRotation(FRotator(0.f, SkillYawRot.Yaw, 0.f));
		}
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(MontageToPlay);

		const float SkillTriggerTime =
			MontageToPlay->GetPlayLength() * ActiveSkillTriggerTimeRatio;

		GetWorldTimerManager().SetTimer(
			TimerHandle_Skill,
			this,
			&AKnight::ActivateSkillEffectAndDamage,
			SkillTriggerTime,
			false
		);

		GetWorldTimerManager().SetTimer(
			TimerHandle_SkillEnd,
			[this]()
			{
				bIsUsingSkill = false;
				bHasSuperArmor = false;
				bSkillMoving = false;
				SkillMoveElapsed = 0.f;
				ActiveSkillMontage = nullptr;

				ScheduleAutoSheatheWeapon();
			},
			MontageToPlay->GetPlayLength(),
			false
		);
	}
}

void AKnight::ActivateSkillEffectAndDamage()
{
	const FVector EffectLocation = GetMesh()
		? GetMesh()->GetSocketLocation(SkillEffectSocketName)
		: GetActorLocation();

	PlaySkillImpactSound();

	SpawnCurrentWeaponSkillEffect(EffectLocation, GetActorRotation());

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	TArray<FOverlapResult> OverlapResults;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(ActiveSkillRadius);

	bool bHit = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		EffectLocation,
		FQuat::Identity,
		ECC_Pawn,
		SphereShape
	);

	if (!bHit)
	{
		return;
	}

	const float FinalSkillDamage = AttackPower * SkillDamageMultiplier;


	TSet<AActor*> DamagedActors;

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || HitActor == this)
		{
			continue;
		}

		if (!HitActor->ActorHasTag(TEXT("Monster")))
		{
			continue;
		}

		if (DamagedActors.Contains(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);

		UGameplayStatics::ApplyDamage(
			HitActor,
			FinalSkillDamage,
			GetController(),
			this,
			nullptr
		);
	}

	/*#if WITH_EDITOR
	DrawDebugSphere(GetWorld(), EffectLocation, ActiveSkillRadius, 24, FColor::Red, false, 1.5f);
	#endif*/
}

void AKnight::SpawnCurrentWeaponSkillEffect(const FVector& DefaultEffectLocation, const FRotator& EffectRotation)
{
	// 무기를 장착 중이면 무기 ItemData의 스킬 이펙트를 우선 사용
	if (InventoryComp)
	{
		const FInventoryItem& EquippedWeapon = InventoryComp->GetEquippedWeapon();

		if (EquippedWeapon.IsValid() && EquippedWeapon.ItemData.EquipSlot == EEquipmentSlot::Weapon)
		{
			const FItemData& ItemData = EquippedWeapon.ItemData;

			if (ItemData.WeaponSkillEffectType == EWeaponEffectType::None)
			{
				return;
			}

			FVector EffectLocation = DefaultEffectLocation;

			// 무기별 스킬 이펙트 소켓을 지정했으면 그 소켓 위치 사용
			if (!ItemData.WeaponSkillEffectSocketName.IsNone())
			{
				if (GetMesh() && GetMesh()->DoesSocketExist(ItemData.WeaponSkillEffectSocketName))
				{
					EffectLocation = GetMesh()->GetSocketLocation(ItemData.WeaponSkillEffectSocketName);
				}
				else if (swordMeshComp && swordMeshComp->DoesSocketExist(ItemData.WeaponSkillEffectSocketName))
				{
					EffectLocation = swordMeshComp->GetSocketLocation(ItemData.WeaponSkillEffectSocketName);
				}
				else
				{
					UE_LOG(
						LogTemp,
						Warning,
						TEXT("Weapon skill effect socket not found. Socket=%s, Weapon=%s"),
						*ItemData.WeaponSkillEffectSocketName.ToString(),
						*ItemData.ItemName.ToString()
					);
				}
			}

			if (ItemData.WeaponSkillEffectType == EWeaponEffectType::Niagara)
			{
				if (!ItemData.WeaponSkillNiagaraEffect)
				{
					return;
				}

				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					GetWorld(),
					ItemData.WeaponSkillNiagaraEffect.Get(),
					EffectLocation,
					EffectRotation
				);

				return;
			}

			if (ItemData.WeaponSkillEffectType == EWeaponEffectType::Particle)
			{
				if (!ItemData.WeaponSkillParticleEffect)
				{
					return;
				}

				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ItemData.WeaponSkillParticleEffect.Get(),
					EffectLocation,
					EffectRotation
				);

				return;
			}
		}
	}

	// 무기 미장착 상태일 때만 기존 기본 스킬 이펙트 사용
	if (SkillEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			SkillEffect,
			DefaultEffectLocation,
			EffectRotation
		);
	}
}

bool AKnight::TryStartBackAttack()
{
	if (!BackAttackMontage || bIsBackAttacking)
	{
		return false;
	}

	ABaseMonster* BestTarget = FindBestBackAttackTarget();
	if (!BestTarget)
	{
		return false;
	}

	InterruptCurrentAction();

	DrawSwordForCombat();

	BackAttackTarget = BestTarget;
	bIsBackAttacking = true;
	bIsInvincible = true;

	BackAttackTarget->FreezeForBackAttack();

	// 플레이어를 몬스터 뒤쪽 위치에 정렬하고 바라보게
	FVector ToMonster = BestTarget->GetActorLocation() - GetActorLocation();
	ToMonster.Z = 0.f;
	if (!ToMonster.IsNearlyZero())
	{
		const FRotator FaceRot = ToMonster.Rotation();
		SetActorRotation(FRotator(0.f, FaceRot.Yaw, 0.f));

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetControlRotation(FRotator(0.f, FaceRot.Yaw, 0.f));
		}
	}

	const float BackAttackDuration =
		BackAttackMontage ? BackAttackMontage->GetPlayLength() : BackAttackInvincibleTime;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(BackAttackMontage);
	}

	PlayBackAttackStartSound();

	GetWorldTimerManager().SetTimer(
		TimerHandle_BackAttackInvincible,
		this,
		&AKnight::EndInvincible,
		BackAttackDuration,
		false
	);

	// 플레이어 몽타주 끝나고 데미지/녹다운
	GetWorldTimerManager().SetTimer(
		TimerHandle_BackAttackEnd,
		this,
		&AKnight::FinishBackAttack,
		BackAttackDuration,
		false
	);

	return true;
}

void AKnight::FinishBackAttack()
{
	ApplyBackAttackDamage();

	if (BackAttackTarget && !BackAttackTarget->IsDead())
	{
		BackAttackTarget->SetBackAttackReaction(false);
 
		if (!BackAttackTarget->IsKnockedDown())
		{
			BackAttackTarget->UnfreezeFromBackAttack();
		}
	}

	bIsBackAttacking = false;
	bIsInvincible = false;
	BackAttackTarget = nullptr;

	ScheduleAutoSheatheWeapon();
}
void AKnight::RegisterBackAttackTarget(ABaseMonster* Monster)
{
	if (!Monster)
	{
		return;
	}

	if (!AvailableBackAttackTargets.Contains(Monster))
	{
		AvailableBackAttackTargets.Add(Monster);
	}
}

void AKnight::UnregisterBackAttackTarget(ABaseMonster* Monster)
{
	if (!Monster)
	{
		return;
	}

	AvailableBackAttackTargets.Remove(Monster);
}

ABaseMonster* AKnight::FindBestBackAttackTarget() const
{
	ABaseMonster* BestTarget = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (ABaseMonster* Monster : AvailableBackAttackTargets)
	{
		if (!Monster || Monster->IsDead() || !Monster->CanBeBackAttacked())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Monster->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Monster;
		}
	}

	return BestTarget;
}

void AKnight::ApplyBackAttackDamage()
{
	if (!BackAttackTarget || BackAttackTarget->IsDead())
	{
		return;
	}

	const float Damage = AttackPower * BackAttackDamageMultiplier;

	BackAttackTarget->SetBackAttackReaction(true);
	PlayBackAttackImpactSound();

	UGameplayStatics::ApplyDamage(
		BackAttackTarget,
		Damage,
		GetController(),
		this,
		nullptr
	);

	if (!BackAttackTarget || BackAttackTarget->IsDead())
	{
		return;
	}

	ATutorialBossMonster* BossTarget = Cast<ATutorialBossMonster>(BackAttackTarget);

	// 일반 몬스터만 기존 백어택 녹다운 처리
	if (!BossTarget)
	{
		BackAttackTarget->PlayKnockDownMontage();
		BackAttackTarget->EnterKnockDown(BackAttackKnockDownTime);
	}
	else
	{
		if (ATutorialBossMonster* RealBoss = Cast<ATutorialBossMonster>(BackAttackTarget))
		{
			RealBoss->FinishGroggyFromBackAttack();
		}
		else
		{
			BackAttackTarget->UnfreezeFromBackAttack();
			BackAttackTarget->SetBackAttackReaction(false);
		}
	}
}

void AKnight::EnterBossKnockDown(AActor* DamageCauser)
{
	if (bIsDead || bIsKnockedDownByBoss)
	{
		return;
	}

	InterruptCurrentAction();

	bIsGettingUpFromBossKnockDown = false;
	bCanGetUpFromBossKnockDown = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_BossGetUpEnd);
	GetWorldTimerManager().ClearTimer(TimerHandle_BossGetUpDelay);

	bIsKnockedDownByBoss = true;
	bIsHitReacting = false;
	bIsInvincible = false;
	bIsAttacking = false;
	bIsDodging = false;
	bIsRunning = false;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	//GetCharacterMovement()->DisableMovement();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (KnockedDownMontage)
		{
			AnimInstance->Montage_Play(KnockedDownMontage);
		}
	}

	GetWorldTimerManager().SetTimer(
		TimerHandle_BossGetUpDelay,
		this,
		&AKnight::EnableBossKnockDownGetUp,
		BossKnockDownGetUpDelay,
		false
	);

	FVector LaunchDir = -GetActorForwardVector();

	if (DamageCauser)
	{
		LaunchDir = GetActorLocation() - DamageCauser->GetActorLocation();
		LaunchDir.Z = 0.f;
		LaunchDir.Normalize();
	}

	if (LaunchDir.IsNearlyZero())
	{
		LaunchDir = -GetActorForwardVector();
	}

	LaunchDir.Z = 0.25f;
	LaunchDir.Normalize();

	LaunchCharacter(
		LaunchDir * KnockDownLaunchStrength + FVector(0.f, 0.f, KnockDownUpwardStrength),
		true,
		true
	);
}

void AKnight::TryGetUpFromBossKnockDown()
{
	if (!bIsKnockedDownByBoss || bIsDead || bIsGettingUpFromBossKnockDown)
	{
		return;
	}

	if (!bCanGetUpFromBossKnockDown)
	{
		return;
	}

	bCanGetUpFromBossKnockDown = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_BossGetUpDelay);

	bIsKnockedDownByBoss = false;
	bIsGettingUpFromBossKnockDown = true;
	bIsInvincible = true;

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (GetUpMontage)
		{
			AnimInstance->Montage_Play(GetUpMontage);

			GetWorldTimerManager().ClearTimer(TimerHandle_BossGetUpEnd);
			GetWorldTimerManager().SetTimer(
				TimerHandle_BossGetUpEnd,
				this,
				&AKnight::FinishBossGetUp,
				GetUpMontage->GetPlayLength(),
				false
			);
		}
		else
		{
			FinishBossGetUp();
		}
	}
	else
	{
		FinishBossGetUp();
	}
}

void AKnight::FinishBossGetUp()
{
	if (bIsDead)
	{
		return;
	}

	bCanGetUpFromBossKnockDown = false;
	GetWorldTimerManager().ClearTimer(TimerHandle_BossGetUpDelay);
	bIsGettingUpFromBossKnockDown = false;

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->StopMovementImmediately();

	// 기상 무적 종료 타이머는 진짜 무적 플래그를 끄는 함수로 연결
	GetWorldTimerManager().ClearTimer(TimerHandle_BossGetUpEnd);
	GetWorldTimerManager().SetTimer(
		TimerHandle_BossGetUpEnd,
		this,
		&AKnight::EndInvincible,
		BossGetUpInvincibleTime,
		false
	);
}

void AKnight::SetCurrentBossHUDTarget(ABaseMonster* NewTarget)
{
	CurrentBossHUDTarget = NewTarget;

	if (CurrentBossHUDTarget)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_AutoSheatheWeapon);
		GetWorldTimerManager().ClearTimer(TimerHandle_SheatheWeapon);

		if (bIsSheathingWeapon)
		{
			bIsSheathingWeapon = false;

			if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
			{
				if (SheatheWeaponMontage && AnimInstance->Montage_IsPlaying(SheatheWeaponMontage))
				{
					AnimInstance->Montage_Stop(0.1f, SheatheWeaponMontage);
				}
			}
		}
	}
}

float AKnight::GetBossHPPercent() const
{
	if (!CurrentBossHUDTarget || CurrentBossHUDTarget->IsDead())
	{
		return 0.f;
	}

	const float Max = CurrentBossHUDTarget->GetMaxHP();
	return Max > 0.f ? (CurrentBossHUDTarget->GetCurrentHP() / Max) : 0.f;
}

float AKnight::GetBossGroggyPercent() const
{
	const ATutorialBossMonster* Boss = Cast<ATutorialBossMonster>(CurrentBossHUDTarget);
	if (!Boss)
	{
		return 0.f;
	}

	const float Max = Boss->GetMaxGroggyGauge();
	return Max > 0.f ? (Boss->GetCurrentGroggyGauge() / Max) : 0.f;
}

FText AKnight::GetBossDisplayName() const
{
	if (!CurrentBossHUDTarget || CurrentBossHUDTarget->IsDead())
	{
		return FText::GetEmpty();
	}

	return CurrentBossHUDTarget->GetMonsterDisplayName();
}

void AKnight::SetMemoryShards(int32 NewValue)
{
	CurrentMemoryShardCount = FMath::Max(0, NewValue);
	OnMemoryShardChanged.Broadcast(CurrentMemoryShardCount);

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		GI->SetCurrentMemoryShards(CurrentMemoryShardCount);
	}
}

void AKnight::AddMemoryShards(int32 Amount, bool bShowGainPopup)
{
	if (Amount <= 0)
	{
		if (!bShowGainPopup)
		{
			OnMemoryShardChanged.Broadcast(CurrentMemoryShardCount);
		}
		return;
	}

	CurrentMemoryShardCount += Amount;
	OnMemoryShardChanged.Broadcast(CurrentMemoryShardCount);

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		GI->SetCurrentMemoryShards(CurrentMemoryShardCount);
	}

	if (bShowGainPopup)
	{
		RecentMemoryShardGain = Amount;
		OnMemoryShardGainChanged.Broadcast(RecentMemoryShardGain);

		GetWorldTimerManager().ClearTimer(TimerHandle_ClearMemoryShardGain);
		GetWorldTimerManager().SetTimer(
			TimerHandle_ClearMemoryShardGain,
			this,
			&AKnight::ClearRecentMemoryShardGain,
			1.2f,
			false
		);
	}
}

void AKnight::ClearRecentMemoryShardGain()
{
	RecentMemoryShardGain = 0;
	OnMemoryShardGainChanged.Broadcast(0);
}

void AKnight::SpawnPendingMemoryShardDropFromGI()
{
	if (!MemoryShardDropClass)
	{
		return;
	}

	UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI || !GI->HasPendingMemoryShardDrop())
	{
		return;
	}

	const FMemoryShardDropData& DropData = GI->GetPendingMemoryShardDrop();
	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));

	if (DropData.LevelName != CurrentLevelName)
	{
		return;
	}

	FTransform SpawnTransform = DropData.WorldTransform;

	if (!DropData.TargetPointTag.IsNone())
	{
		if (!FindMemoryShardDropPointTransform(DropData.TargetPointTag, SpawnTransform))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("MemoryShardDrop TargetPoint not found. Tag: %s"),
				*DropData.TargetPointTag.ToString()
			);
			return;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AMemoryShardDrop* Drop = GetWorld()->SpawnActor<AMemoryShardDrop>(
		MemoryShardDropClass,
		SpawnTransform,
		SpawnParams
	);

	if (Drop)
	{
		Drop->InitializeDrop(DropData.Amount);
		SpawnedMemoryShardDrop = Drop;
	}
}

void AKnight::DropMemoryShardsOnDeath(bool bUseSafeFallback)
{
	if (!MemoryShardDropClass)
	{
		return;
	}

	const int32 DroppedAmount = CurrentMemoryShardCount;
	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));

	FTransform DropTransform;
	if (bUseSafeFallback)
	{
		DropTransform = GetSafeMemoryShardDropTransform();
	}
	else
	{
		DropTransform = FTransform(GetActorRotation(), GetActorLocation());
	}

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		const bool bDiedInFinalBossPhase2 =
			CurrentLevelName == FinalBossPhase2LevelName &&
			!FinalBossPhase1LevelName.IsNone() &&
			!FinalBossPhase1DropPointTag.IsNone();

		if (bDiedInFinalBossPhase2)
		{
			GI->SetPendingMemoryShardDropAtTargetPoint(
				FinalBossPhase1LevelName,
				FinalBossPhase1DropPointTag,
				DroppedAmount
			);
		}
		else
		{
			GI->SetPendingMemoryShardDrop(
				CurrentLevelName,
				DropTransform,
				DroppedAmount
			);
		}

		GI->SetCurrentMemoryShards(0);
	}

	CurrentMemoryShardCount = 0;
	OnMemoryShardChanged.Broadcast(CurrentMemoryShardCount);
	ClearRecentMemoryShardGain();
}

void AKnight::ToggleInventory(const FInputActionValue& inputValue)
{
	if (bIsInventoryOpen)
	{
		CloseInventory();
	}
	else
	{
		OpenInventory();
	}
}

void AKnight::OpenInventory()
{
	if (bIsDead || bIsBonfireInteracting || bIsRestingAtBonfire || bIsOpeningDoor)
	{
		return;
	}

	if (!InventoryWidget && InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(GetWorld(), InventoryWidgetClass);
	}

	if (!InventoryWidget)
	{
		return;
	}

	InventoryWidget->AddToViewport(300);
	bIsInventoryOpen = true;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}
}

void AKnight::CloseInventory()
{
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}

	bIsInventoryOpen = false;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
}

void AKnight::ApplyEquipmentBonus(float HPBonus, float AttackBonus, float StaminaBonus)
{
	MaxHP += HPBonus;
	MaxStamina += StaminaBonus;
	AttackPower += AttackBonus;

	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
	CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void AKnight::RemoveEquipmentBonus(float HPBonus, float AttackBonus, float StaminaBonus)
{
	MaxHP -= HPBonus;
	MaxStamina -= StaminaBonus;
	AttackPower -= AttackBonus;

	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
	CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

int32 AKnight::GetSkillStoneCount() const
{
	if (!InventoryComp)
	{
		return 0;
	}

	return InventoryComp->GetItemCountByID(SkillStoneItemID);
}

FText AKnight::GetSkillStoneCountText() const
{
	return FText::AsNumber(GetSkillStoneCount());
}

void AKnight::SaveInventoryToGameInstance()
{
	if (!InventoryComp)
	{
		return;
	}

	UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	TArray<FInventoryItem> SavedItems;
	FInventoryItem Weapon;
	FInventoryItem Helmet;
	FInventoryItem Armor;
	FInventoryItem Gloves;
	FInventoryItem Boots;

	InventoryComp->GetInventorySnapshot(
		SavedItems,
		Weapon,
		Helmet,
		Armor,
		Gloves,
		Boots
	);

	GI->SaveInventoryData(
		SavedItems,
		Weapon,
		Helmet,
		Armor,
		Gloves,
		Boots
	);
}

void AKnight::LoadInventoryFromGameInstance()
{
	if (!InventoryComp)
	{
		return;
	}

	UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	TArray<FInventoryItem> SavedItems;
	FInventoryItem Weapon;
	FInventoryItem Helmet;
	FInventoryItem Armor;
	FInventoryItem Gloves;
	FInventoryItem Boots;

	GI->LoadInventoryData(
		SavedItems,
		Weapon,
		Helmet,
		Armor,
		Gloves,
		Boots
	);

	InventoryComp->RestoreInventorySnapshot(
		SavedItems,
		Weapon,
		Helmet,
		Armor,
		Gloves,
		Boots
	);

	ReapplyEquippedItemBonusesFromInventory();
	RefreshEquippedWeaponVisual();
}

void AKnight::ReapplyEquippedItemBonusesFromInventory()
{
	if (!InventoryComp)
	{
		return;
	}

	const FInventoryItem& Weapon = InventoryComp->GetEquippedWeapon();
	const FInventoryItem& Helmet = InventoryComp->GetEquippedHelmet();
	const FInventoryItem& Armor = InventoryComp->GetEquippedArmor();
	const FInventoryItem& Gloves = InventoryComp->GetEquippedGloves();
	const FInventoryItem& Boots = InventoryComp->GetEquippedBoots();

	auto ApplyIfValid = [this](const FInventoryItem& Item)
		{
			if (Item.ItemData.ItemID != NAME_None && Item.Count > 0)
			{
				ApplyEquipmentBonus(
					Item.ItemData.MaxHPBonus,
					Item.ItemData.AttackBonus,
					Item.ItemData.MaxStaminaBonus
				);
			}
		};

	ApplyIfValid(Weapon);
	ApplyIfValid(Helmet);
	ApplyIfValid(Armor);
	ApplyIfValid(Gloves);
	ApplyIfValid(Boots);
}

void AKnight::HandleInventoryUpdated()
{
	RefreshEquippedWeaponVisual();
	SaveInventoryToGameInstance();
}

bool AKnight::CanUpgradeHP() const
{
	return EnhancementData.HPLevel < MaxEnhanceLevel &&
		CurrentMemoryShardCount >= EnhancementData.NextSharedCost;
}

bool AKnight::CanUpgradeStamina() const
{
	return EnhancementData.StaminaLevel < MaxEnhanceLevel &&
		CurrentMemoryShardCount >= EnhancementData.NextSharedCost;
}

bool AKnight::CanUpgradeAttack() const
{
	return EnhancementData.AttackLevel < MaxEnhanceLevel &&
		CurrentMemoryShardCount >= EnhancementData.NextSharedCost;
}

bool AKnight::UpgradeHPStat()
{
	if (!CanUpgradeHP())
	{
		return false;
	}

	SetMemoryShards(CurrentMemoryShardCount - EnhancementData.NextSharedCost);
	EnhancementData.TotalSpentMemoryShards += EnhancementData.NextSharedCost;
	EnhancementData.HPLevel += 1;
	EnhancementData.NextSharedCost += 10;

	MaxHP += HPIncreasePerLevel;
	CurrentHP = GetEffectiveMaxHP();
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnHPPenaltyChanged.Broadcast(CurrentHPPenaltyPercent);

	SaveEnhancementToGameInstance();
	return true;
}

bool AKnight::UpgradeStaminaStat()
{
	if (!CanUpgradeStamina())
	{
		return false;
	}

	SetMemoryShards(CurrentMemoryShardCount - EnhancementData.NextSharedCost);
	EnhancementData.TotalSpentMemoryShards += EnhancementData.NextSharedCost;
	EnhancementData.StaminaLevel += 1;
	EnhancementData.NextSharedCost += 10;

	MaxStamina += StaminaIncreasePerLevel;
	CurrentStamina = MaxStamina;
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	SaveEnhancementToGameInstance();
	return true;
}

bool AKnight::UpgradeAttackStat()
{
	if (!CanUpgradeAttack())
	{
		return false;
	}

	SetMemoryShards(CurrentMemoryShardCount - EnhancementData.NextSharedCost);
	EnhancementData.TotalSpentMemoryShards += EnhancementData.NextSharedCost;
	EnhancementData.AttackLevel += 1;
	EnhancementData.NextSharedCost += 10;

	AttackPower += AttackIncreasePerLevel;
	SaveEnhancementToGameInstance();
	return true;
}

bool AKnight::ResetEnhancedStats()
{
	if (!InventoryComp)
	{
		return false;
	}

	if (InventoryComp->GetItemCountByID(StatusResetTicketItemID) < 1)
	{
		return false;
	}

	if (!InventoryComp->ConsumeItemByID(StatusResetTicketItemID, 1))
	{
		return false;
	}

	// 환급
	SetMemoryShards(CurrentMemoryShardCount + EnhancementData.TotalSpentMemoryShards);

	// 강화로 오른 수치 되돌리기
	const int32 AddedHP = (EnhancementData.HPLevel - 1) * HPIncreasePerLevel;
	const int32 AddedStamina = (EnhancementData.StaminaLevel - 1) * StaminaIncreasePerLevel;
	const int32 AddedAttack = (EnhancementData.AttackLevel - 1) * AttackIncreasePerLevel;

	MaxHP -= AddedHP;
	MaxStamina -= AddedStamina;
	AttackPower -= AddedAttack;

	CurrentHP = FMath::Clamp(CurrentHP, 0.f, MaxHP);
	CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	EnhancementData.HPLevel = 1;
	EnhancementData.StaminaLevel = 1;
	EnhancementData.AttackLevel = 1;
	EnhancementData.NextSharedCost = 10;
	EnhancementData.TotalSpentMemoryShards = 0;

	SaveEnhancementToGameInstance();
	return true;
}

bool AKnight::UpgradePotionCapacity()
{
	if (!InventoryComp)
	{
		return false;
	}

	if (InventoryComp->GetItemCountByID(MemoryFruitItemID) < 1)
	{
		return false;
	}

	if (!InventoryComp->ConsumeItemByID(MemoryFruitItemID, 1))
	{
		return false;
	}

	MaxPotionCount += 1;
	CurrentPotionCount = MaxPotionCount;
	EnhancementData.PotionUpgradeCount += 1;

	SaveEnhancementToGameInstance();
	return true;
}

void AKnight::SaveEnhancementToGameInstance()
{
	EnhancementData.HPPenaltyPercent = CurrentHPPenaltyPercent;

	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		GI->SaveEnhancementData(EnhancementData);
	}
}

void AKnight::LoadEnhancementFromGameInstance()
{
	if (UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance()))
	{
		EnhancementData = GI->LoadEnhancementData();
		CurrentHPPenaltyPercent = EnhancementData.HPPenaltyPercent;
	}
}

void AKnight::ApplyEnhancementDataToStats(bool bFillCurrentValues)
{
	MaxHP = BaseMaxHP + (EnhancementData.HPLevel - 1) * HPIncreasePerLevel;
	MaxStamina = BaseMaxStamina + (EnhancementData.StaminaLevel - 1) * StaminaIncreasePerLevel;
	AttackPower = BaseAttackPower + (EnhancementData.AttackLevel - 1) * AttackIncreasePerLevel;
	MaxPotionCount = BaseMaxPotionCount + EnhancementData.PotionUpgradeCount;

	if (bFillCurrentValues)
	{
		CurrentHP = GetEffectiveMaxHP();
		CurrentStamina = MaxStamina;
		CurrentPotionCount = MaxPotionCount;
	}
	else
	{
		CurrentHP = FMath::Clamp(CurrentHP, 0.f, GetEffectiveMaxHP());
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.f, MaxStamina);
		CurrentPotionCount = FMath::Clamp(CurrentPotionCount, 0, MaxPotionCount);
	}

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	OnHPPenaltyChanged.Broadcast(CurrentHPPenaltyPercent);
}

float AKnight::GetEffectiveMaxHP() const
{
	return MaxHP * (1.f - CurrentHPPenaltyPercent);
}

float AKnight::GetEffectiveHPPercentOfFullMax() const
{
	if (MaxHP <= 0.f)
	{
		return 0.f;
	}

	return GetEffectiveMaxHP() / MaxHP;
}

float AKnight::GetLostHPPenaltyPercentOfFullMax() const
{
	if (MaxHP <= 0.f)
	{
		return 0.f;
	}

	return (MaxHP - GetEffectiveMaxHP()) / MaxHP;
}

void AKnight::ClampHPToEffectiveMax()
{
	CurrentHP = FMath::Clamp(CurrentHP, 0.f, GetEffectiveMaxHP());
	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	OnHPPenaltyChanged.Broadcast(CurrentHPPenaltyPercent);
}

void AKnight::IncreaseDeathHPPenalty()
{
	CurrentHPPenaltyPercent = FMath::Clamp(
		CurrentHPPenaltyPercent + HPPenaltyStepPercent,
		0.f,
		MaxHPPenaltyPercent
	);

	EnhancementData.HPPenaltyPercent = CurrentHPPenaltyPercent;
	ClampHPToEffectiveMax();
	SaveEnhancementToGameInstance();
}

void AKnight::ClearAllHPPenalty()
{
	CurrentHPPenaltyPercent = 0.f;
	EnhancementData.HPPenaltyPercent = 0.f;

	ClampHPToEffectiveMax();
	SaveEnhancementToGameInstance();
}

void AKnight::HealByMaxHPPercent(float Percent)
{
	if (Percent <= 0.f)
	{
		return;
	}

	const float HealAmount = GetEffectiveMaxHP() * Percent;
	Heal(HealAmount);
}

void AKnight::BeginDoorInteraction(float Duration)
{
	if (bIsDead || bIsOpeningDoor)
	{
		return;
	}

	InterruptCurrentAction();
	bIsOpeningDoor = true;

	UpdateSwordVisibilityByState();

	GetCharacterMovement()->StopMovementImmediately();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_DoorInteraction);
	GetWorldTimerManager().SetTimer(
		TimerHandle_DoorInteraction,
		this,
		&AKnight::FinishDoorInteraction,
		FMath::Max(0.01f, Duration),
		false
	);
}

void AKnight::FinishDoorInteraction()
{
	bIsOpeningDoor = false;

	if (bIsDead)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	UpdateSwordVisibilityByState();
}

void AKnight::TryPlayTownIntro()
{
	UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	if (CurrentLevelName != TownLevelName)
	{
		return;
	}

	if (GI->HasPlayedTownIntro())
	{
		return;
	}

	if (!TownIntroWidgetClass)
	{
		return;
	}

	TownIntroWidget = CreateWidget<UUserWidget>(GetWorld(), TownIntroWidgetClass);
	if (!TownIntroWidget)
	{
		return;
	}

	// 플레이어 HUD 숨기기
	SetPlayerHUDVisible(false);

	TownIntroWidget->AddToViewport(500);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	GI->SetHasPlayedTownIntro(true);

	GetWorldTimerManager().SetTimer(
		TimerHandle_TownIntroEnd,
		this,
		&AKnight::FinishTownIntro,
		TownIntroDuration,
		false
	);
}

void AKnight::FinishTownIntro()
{
	if (TownIntroWidget)
	{
		TownIntroWidget->RemoveFromParent();
		TownIntroWidget = nullptr;
	}

	SetPlayerHUDVisible(true);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
}

void AKnight::UpdateRecentSafeTransforms(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 땅에 서 있을 때만 안전 위치로 기록
	if (!GetCharacterMovement() || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}

	FRecentSafeTransformEntry Entry;
	Entry.TimeSeconds = World->GetTimeSeconds();
	Entry.Transform = GetActorTransform();

	RecentSafeTransforms.Add(Entry);

	const float CutoffTime = World->GetTimeSeconds() - MemoryShardSafeHistoryLength;

	for (int32 i = RecentSafeTransforms.Num() - 1; i >= 0; --i)
	{
		if (RecentSafeTransforms[i].TimeSeconds < CutoffTime)
		{
			RecentSafeTransforms.RemoveAt(i);
		}
	}
}

FTransform AKnight::GetSafeMemoryShardDropTransform() const
{
	UWorld* World = GetWorld();
	if (!World || RecentSafeTransforms.Num() <= 0)
	{
		return FTransform(GetActorRotation(), GetActorLocation());
	}

	const float TargetTime = World->GetTimeSeconds() - MemoryShardSafeDropDelay;

	// TargetTime보다 이전인 것 중 가장 가까운 값 찾기
	for (int32 i = RecentSafeTransforms.Num() - 1; i >= 0; --i)
	{
		if (RecentSafeTransforms[i].TimeSeconds <= TargetTime)
		{
			return RecentSafeTransforms[i].Transform;
		}
	}

	// 없으면 가장 오래된 안전 위치 반환
	return RecentSafeTransforms[0].Transform;
}

void AKnight::TryShowDeathPenaltyTutorial()
{
	UFortressGameInstance* GI = Cast<UFortressGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	if (GI->HasSeenTutorial(DeathPenaltyTutorialID))
	{
		return;
	}

	if (!DeathPenaltyTutorialWidgetClass)
	{
		return;
	}

	if (!DeathPenaltyTutorialWidget)
	{
		DeathPenaltyTutorialWidget = CreateWidget<UUserWidget>(GetWorld(), DeathPenaltyTutorialWidgetClass);
	}

	if (!DeathPenaltyTutorialWidget)
	{
		return;
	}

	if (UTutorialPopupWidget* TutorialPopup = Cast<UTutorialPopupWidget>(DeathPenaltyTutorialWidget))
	{
		TutorialPopup->OwningKnight = this;
	}

	if (!DeathPenaltyTutorialWidget->IsInViewport())
	{
		DeathPenaltyTutorialWidget->AddToViewport(200);
	}

	ApplyDeathPenaltyTutorialInputMode();

	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&AKnight::ApplyDeathPenaltyTutorialInputMode
	);

	GI->MarkTutorialSeen(DeathPenaltyTutorialID);
}

void AKnight::CloseDeathPenaltyTutorial()
{
	if (DeathPenaltyTutorialWidget)
	{
		DeathPenaltyTutorialWidget->RemoveFromParent();
		DeathPenaltyTutorialWidget = nullptr;
	}

	RestoreGameplayInputFromUI(false);
}

void AKnight::ShowLoadingScreen(float Duration)
{
	if (!LoadingWidget && LoadingWidgetClass)
	{
		LoadingWidget = CreateWidget<UUserWidget>(GetWorld(), LoadingWidgetClass);
	}

	if (!LoadingWidget)
	{
		return;
	}

	if (!LoadingWidget->IsInViewport())
	{
		LoadingWidget->AddToViewport(1000);
	}

	SetPlayerHUDVisible(false);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;

		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_LoadingScreen);
	GetWorldTimerManager().SetTimer(
		TimerHandle_LoadingScreen,
		this,
		&AKnight::HideLoadingScreen,
		Duration,
		false
	);
}

void AKnight::HideLoadingScreen()
{
	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}

	SetPlayerHUDVisible(true);

	if (DeathPenaltyTutorialWidget && DeathPenaltyTutorialWidget->IsInViewport())
	{
		ApplyDeathPenaltyTutorialInputMode();
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);

		PC->bShowMouseCursor = false;

		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
}

void AKnight::SetGrabbedByBoss(bool bNewGrabbed)
{
	if (bIsGrabbedByBoss == bNewGrabbed)
	{
		return;
	}

	bIsGrabbedByBoss = bNewGrabbed;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	if (bIsGrabbedByBoss)
	{
		// 그랩 시작 순간 락온 강제 해제
		ClearLockOn();

		bLockOnButtonHeld = false;
		bLockOnHoldTriggered = false;
		GetWorldTimerManager().ClearTimer(TimerHandle_LockOnHold);

		MoveInput = FVector2D::ZeroVector;
		bIsRunning = false;
		bDodgeMoving = false;
		bSkillMoving = false;

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
			GetCharacterMovement()->bOrientRotationToMovement = false;
		}

		bUseControllerRotationYaw = false;

		SavedMonsterCollisionResponse =
			Capsule->GetCollisionResponseToChannel(ECC_GameTraceChannel2);

		Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	}
	else
	{
		Capsule->SetCollisionResponseToChannel(
			ECC_GameTraceChannel2,
			SavedMonsterCollisionResponse
		);

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
		}
	}
}

void AKnight::RestoreGameplayInputFromUI(bool bUnpauseGame)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		if (bUnpauseGame)
		{
			UGameplayStatics::SetGamePaused(this, false);
			PC->SetPause(false);
		}

		PC->bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);

		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();

		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);

		if (!bIsDead)
		{
			EnableInput(PC);
		}
	}

	if (!bIsDead && !bIsKnockedDownByBoss && !bIsGettingUpFromBossKnockDown && !bIsGrabbedByBoss)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			if (MoveComp->MovementMode == MOVE_None)
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}

			MoveComp->MaxWalkSpeed = bIsExhausted ? ExhaustedMoveSpeed : walkSpeed;
		}
	}
}

USoundBase* AKnight::GetRandomSound(const TArray<USoundBase*>& Sounds) const
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

void AKnight::PlayWeaponSwingSound()
{
	USoundBase* Sound = GetRandomSound(WeaponSwingSounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(0.96f, 1.04f);

	if (swordMeshComp)
	{
		UGameplayStatics::SpawnSoundAttached(
			Sound,
			swordMeshComp,
			TEXT("TraceEnd"),
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

void AKnight::PlayAttackVoiceSound()
{
	USoundBase* Sound = GetRandomSound(AttackVoiceSounds);
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

USoundBase* AKnight::GetRandomDeathSound() const
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

void AKnight::PlayDeathSound()
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

USoundBase* AKnight::GetRandomSkillSound(const TArray<USoundBase*>& Sounds) const
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

void AKnight::PlaySkillCastSound()
{
	USoundBase* Sound = GetRandomSkillSound(SkillCastSounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(0.96f, 1.04f);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		SkillCastSoundVolume,
		Pitch
	);
}

void AKnight::PlaySkillImpactSound()
{
	USoundBase* Sound = GetRandomSkillSound(SkillImpactSounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(0.96f, 1.04f);

	const FVector SoundLocation = GetMesh()
		? GetMesh()->GetSocketLocation(SkillEffectSocketName)
		: GetActorLocation();

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		SoundLocation,
		SkillImpactSoundVolume,
		Pitch
	);
}

void AKnight::PlaySkillVoiceSound()
{
	USoundBase* Sound = GetRandomSkillSound(SkillVoiceSounds);
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
		SkillVoiceVolume,
		Pitch
	);
}

bool AKnight::HasBossHUDTarget() const
{
	return CurrentBossHUDTarget != nullptr && !CurrentBossHUDTarget->IsDead();
}


USoundBase* AKnight::GetRandomPotionHealSound() const
{
	TArray<USoundBase*> ValidSounds;

	for (USoundBase* Sound : PotionHealSounds)
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

void AKnight::PlayPotionHealSound()
{
	USoundBase* Sound = GetRandomPotionHealSound();
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(PotionHealSoundPitchMin, PotionHealSoundPitchMax);

	UGameplayStatics::SpawnSoundAttached(
		Sound,
		GetMesh(),
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true,
		PotionHealSoundVolume,
		Pitch
	);
}

USoundBase* AKnight::GetRandomWeaponBounceSound() const
{
	TArray<USoundBase*> ValidSounds;

	for (USoundBase* Sound : WeaponBounceSounds)
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

void AKnight::PlayWeaponBounceSound()
{
	USoundBase* Sound = GetRandomWeaponBounceSound();
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(WeaponBounceSoundPitchMin, WeaponBounceSoundPitchMax);

	if (swordMeshComp)
	{
		UGameplayStatics::SpawnSoundAttached(
			Sound,
			swordMeshComp,
			TEXT("TraceEnd"),
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget,
			true,
			WeaponBounceSoundVolume,
			Pitch
		);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Sound,
			GetActorLocation(),
			WeaponBounceSoundVolume,
			Pitch
		);
	}
}

USoundBase* AKnight::GetRandomBackAttackSound(const TArray<USoundBase*>& Sounds) const
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

void AKnight::PlayBackAttackSound(const TArray<USoundBase*>& Sounds)
{
	USoundBase* Sound = GetRandomBackAttackSound(Sounds);
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(BackAttackSoundPitchMin, BackAttackSoundPitchMax);

	UGameplayStatics::SpawnSoundAttached(
		Sound,
		GetMesh(),
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true,
		BackAttackSoundVolume,
		Pitch
	);
}

void AKnight::PlayBackAttackStartSound()
{
	const bool bIsGroggyBossAttack = Cast<ATutorialBossMonster>(BackAttackTarget) != nullptr;

	if (bIsGroggyBossAttack)
	{
		PlayBackAttackSound(GroggyAttackStartSounds);
	}
	else
	{
		PlayBackAttackSound(BackAttackStartSounds);
	}
}

void AKnight::PlayBackAttackImpactSound()
{
	const bool bIsGroggyBossAttack = Cast<ATutorialBossMonster>(BackAttackTarget) != nullptr;

	if (bIsGroggyBossAttack)
	{
		PlayBackAttackSound(GroggyAttackImpactSounds);
	}
	else
	{
		PlayBackAttackSound(BackAttackImpactSounds);
	}
}

void AKnight::SetSwordVisible(bool bVisible)
{
	if (!swordMeshComp)
	{
		return;
	}

	if (!bVisible)
	{
		StopWeaponAttackTrail();
	}

	swordMeshComp->SetVisibility(bVisible, true);
	swordMeshComp->SetHiddenInGame(!bVisible, true);

	swordMeshComp->SetCollisionEnabled(
		bVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision
	);

	if (WeaponNiagaraEffectComp)
	{
		WeaponNiagaraEffectComp->SetVisibility(bVisible, true);
		WeaponNiagaraEffectComp->SetHiddenInGame(!bVisible, true);

		if (bVisible)
		{
			WeaponNiagaraEffectComp->Activate();
		}
		else
		{
			WeaponNiagaraEffectComp->Deactivate();
		}
	}

	if (WeaponParticleEffectComp)
	{
		WeaponParticleEffectComp->SetVisibility(bVisible, true);
		WeaponParticleEffectComp->SetHiddenInGame(!bVisible, true);

		if (bVisible)
		{
			WeaponParticleEffectComp->ActivateSystem();
		}
		else
		{
			WeaponParticleEffectComp->DeactivateSystem();
		}
	}

	UpdateWeaponLightVisibility();
}

void AKnight::ResetWeaponPhysicsForAttach()
{
	if (!swordMeshComp)
	{
		return;
	}

	bWeaponDroppedOnDeath = false;

	swordMeshComp->SetSimulatePhysics(false);
	swordMeshComp->SetEnableGravity(false);

	swordMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	swordMeshComp->SetCollisionObjectType(ECC_WorldDynamic);
	swordMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	swordMeshComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap); // Monster trace용
}

void AKnight::DropWeaponOnDeath()
{
	if (!swordMeshComp || bWeaponDroppedOnDeath)
	{
		return;
	}

	if (!swordMeshComp->GetStaticMesh())
	{
		return;
	}

	bWeaponDroppedOnDeath = true;

	StopWeaponAttackTrail();

	const FTransform CurrentWorldTransform = swordMeshComp->GetComponentTransform();

	swordMeshComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	swordMeshComp->SetWorldTransform(CurrentWorldTransform);

	swordMeshComp->SetVisibility(true, true);
	swordMeshComp->SetHiddenInGame(false, true);

	// 먼저 속도 초기화
	swordMeshComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
	swordMeshComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	// 충돌은 월드만 막고, Pawn/플레이어/몬스터 관련은 무시
	swordMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	swordMeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	swordMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	swordMeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	swordMeshComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	// 플레이어 시체, 몬스터, 공격 트레이스와 부딪혀 튀는 것 방지
	swordMeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	swordMeshComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore); // Player
	swordMeshComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore); // Monster

	// 물리 감쇠를 크게 줘서 데굴데굴 튀는 것 줄이기
	swordMeshComp->SetLinearDamping(DeathWeaponLinearDamping);
	swordMeshComp->SetAngularDamping(DeathWeaponAngularDamping);

	swordMeshComp->SetEnableGravity(true);
	swordMeshComp->SetSimulatePhysics(true);
	swordMeshComp->WakeRigidBody();

	// 살짝만 아래로 떨어지게 하고 싶으면 임펄스 없이 중력만 사용
	// 강제로 날아가는 연출을 원할 때만 아래 AddImpulse를 다시 사용

	bWeaponDrawn = false;
	bIsSheathingWeapon = false;
}

void AKnight::ApplyWeaponHandDefaultTransform()
{
	if (!swordMeshComp)
	{
		return;
	}

	swordMeshComp->SetRelativeLocation(WeaponHandRelativeLocation);
	swordMeshComp->SetRelativeRotation(WeaponHandRelativeRotation);
}

void AKnight::AttachSwordToHand()
{
	if (!swordMeshComp || !GetMesh())
	{
		return;
	}

	ResetWeaponPhysicsForAttach();

	swordMeshComp->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponHandSocketName
	);

	ApplyWeaponHandDefaultTransform();

	bWeaponDrawn = true;

	SetSwordVisible(true);
}

void AKnight::ApplyWeaponWaistDefaultTransform()
{
	if (!swordMeshComp)
	{
		return;
	}

	swordMeshComp->SetRelativeLocation(WeaponWaistRelativeLocation);
	swordMeshComp->SetRelativeRotation(WeaponWaistRelativeRotation);
}

void AKnight::AttachSwordToWaist()
{
	if (!swordMeshComp || !GetMesh())
	{
		return;
	}

	ResetWeaponPhysicsForAttach();

	swordMeshComp->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponWaistSocketName
	);

	ApplyWeaponWaistDefaultTransform();

	bWeaponDrawn = false;
	bIsSheathingWeapon = false;

	SetSwordVisible(true);
}

void AKnight::DrawSwordForCombat()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_AutoSheatheWeapon);
	GetWorldTimerManager().ClearTimer(TimerHandle_SheatheWeapon);

	if (bIsSheathingWeapon)
	{
		bIsSheathingWeapon = false;

		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			if (SheatheWeaponMontage && AnimInstance->Montage_IsPlaying(SheatheWeaponMontage))
			{
				AnimInstance->Montage_Stop(0.1f, SheatheWeaponMontage);
			}
		}
	}

	if (!bWeaponDrawn)
	{
		AttachSwordToHand();
	}
}

void AKnight::ScheduleAutoSheatheWeapon()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_AutoSheatheWeapon);

	if (!bWeaponDrawn)
	{
		return;
	}

	if(bIsDrinking || bIsBonfireInteracting || bIsRestingAtBonfire || bIsExhausted)
	{
		return;
	}

	// 보스 전투 중에는 칼 집어넣지 않음
	if (IsInBossBattle())
	{
		return;
	}

	if (bIsDead || bIsAttacking || bIsUsingSkill || bIsBackAttacking || bIsWeaponBouncing)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		TimerHandle_AutoSheatheWeapon,
		this,
		&AKnight::StartSheatheWeapon,
		AutoSheatheDelay,
		false
	);
}

void AKnight::StartSheatheWeapon()
{
	if (!bWeaponDrawn)
	{
		return;
	}

	if (bIsDrinking || bIsBonfireInteracting || bIsRestingAtBonfire || bIsExhausted)
	{
		return;
	}

	// 보스 전투 중이면 납도 시작 금지
	if (IsInBossBattle())
	{
		return;
	}

	if (bIsDead || bIsAttacking || bIsUsingSkill || bIsBackAttacking || bIsWeaponBouncing)
	{
		return;
	}

	bIsSheathingWeapon = true;

	PlaySheatheWeaponSound();

	float SheatheDuration = 0.1f;

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (SheatheWeaponMontage)
		{
			AnimInstance->Montage_Play(SheatheWeaponMontage);
			SheatheDuration = SheatheWeaponMontage->GetPlayLength();
		}
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_SheatheWeapon);
	GetWorldTimerManager().SetTimer(
		TimerHandle_SheatheWeapon,
		this,
		&AKnight::FinishSheatheWeapon,
		FMath::Max(0.01f, SheatheDuration *  0.75f),
		false
	);
}

void AKnight::FinishSheatheWeapon()
{
	if (bIsDead)
	{
		return;
	}

	bIsSheathingWeapon = false;

	if (IsInBossBattle())
	{
		DrawSwordForCombat();
		return;
	}

	if (bIsAttacking || bIsUsingSkill || bIsBackAttacking || bIsWeaponBouncing)
	{
		return;
	}

	AttachSwordToWaist();
}

void AKnight::UpdateSwordVisibilityByState()
{
	const bool bShouldHideSword =
		bIsDrinking ||
		bIsBonfireInteracting ||
		bIsRestingAtBonfire ||
		bIsOpeningDoor ||
		bIsOpeningChest ||
		bIsPlayingItemPickupMontage;

	SetSwordVisible(!bShouldHideSword);
}

void AKnight::BeginChestOpen(AMimicChestMonster* Chest)
{
	if (!Chest)
	{
		return;
	}

	if (bIsDead || bIsHitReacting || bIsDodging || bIsAttacking || bIsDrinking ||
		bIsBonfireInteracting || bIsRestingAtBonfire || bIsUsingSkill ||
		bIsBackAttacking || bIsKnockedDownByBoss || bIsGettingUpFromBossKnockDown ||
		bIsOpeningDoor || bIsOpeningChest || bIsWeaponBouncing || bIsPlayingItemPickupMontage)
	{
		Chest->CancelPendingChestOpen(this);
		return;
	}

	InterruptCurrentAction();

	bIsOpeningChest = true;
	PendingChestToOpen = Chest;

	UpdateSwordVisibilityByState();

	ClearInteractActor(Chest);

	bIsRunning = false;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

	// 상자 정면 Arrow 위치로 이동
	const FTransform OpenTransform = Chest->GetChestOpenPointTransform();

	const FVector OpenLocation = OpenTransform.GetLocation();
	const FRotator OpenRotation(0.f, OpenTransform.GetRotation().Rotator().Yaw, 0.f);

	SetActorLocationAndRotation(
		OpenLocation,
		OpenRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(false);
	}

	float MontageLength = 0.1f;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (ChestOpenMontage)
		{
			AnimInstance->Montage_Play(ChestOpenMontage);
			MontageLength = ChestOpenMontage->GetPlayLength();
		}
	}

	bChestOpenTriggered = false;

	const float OpenTriggerDelay =
		FMath::Clamp(ChestOpenTriggerTimeRatio, 0.05f, 0.95f) * MontageLength;

	// 몽타주 도중 상자 열림
	GetWorldTimerManager().ClearTimer(TimerHandle_ChestOpenTrigger);
	GetWorldTimerManager().SetTimer(
		TimerHandle_ChestOpenTrigger,
		this,
		&AKnight::TriggerChestOpenDuringMontage,
		FMath::Max(0.01f, OpenTriggerDelay),
		false
	);

	// 플레이어 몽타주 끝나면 플레이어 상태만 복구
	GetWorldTimerManager().ClearTimer(TimerHandle_ChestOpen);
	GetWorldTimerManager().SetTimer(
		TimerHandle_ChestOpen,
		this,
		&AKnight::FinishChestOpen,
		FMath::Max(0.01f, MontageLength),
		false
	);
}

void AKnight::FinishChestOpen()
{
	AMimicChestMonster* Chest = PendingChestToOpen;

	bIsOpeningChest = false;
	PendingChestToOpen = nullptr;

	RestoreGameplayInputFromUI(true);

	if (!bIsDead)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	}

	UpdateSwordVisibilityByState();

	if (Chest && !bChestOpenTriggered)
	{
		TriggerChestOpenDuringMontage();
	}

	bChestOpenTriggered = false;
}

void AKnight::TriggerChestOpenDuringMontage()
{
	if (bChestOpenTriggered)
	{
		return;
	}

	bChestOpenTriggered = true;

	AMimicChestMonster* Chest = PendingChestToOpen;
	if (!Chest)
	{
		return;
	}

	Chest->OpenChestAfterPlayerMontage(this);
}

bool AKnight::IsInBossBattle() const
{
	return CurrentBossHUDTarget != nullptr && !CurrentBossHUDTarget->IsDead();
}

USoundBase* AKnight::GetRandomSheatheWeaponSound() const
{
	TArray<USoundBase*> ValidSounds;

	for (USoundBase* Sound : SheatheWeaponSounds)
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

void AKnight::PlaySheatheWeaponSound()
{
	USoundBase* Sound = GetRandomSheatheWeaponSound();
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(
		SheatheWeaponSoundPitchMin,
		SheatheWeaponSoundPitchMax
	);

	if (swordMeshComp)
	{
		UGameplayStatics::SpawnSoundAttached(
			Sound,
			swordMeshComp,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			true,
			SheatheWeaponSoundVolume,
			Pitch
		);
	}
	else
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Sound,
			GetActorLocation(),
			SheatheWeaponSoundVolume,
			Pitch
		);
	}
}

void AKnight::ApplyNormalCamera()
{
	if (!springArmComp)
	{
		return;
	}

	springArmComp->SetRelativeLocation(NormalCameraRelativeLocation);
	springArmComp->TargetArmLength = NormalCameraArmLength;
	springArmComp->SocketOffset = NormalCameraSocketOffset;
}

void AKnight::ApplyLockOnCamera()
{
	if (!springArmComp)
	{
		return;
	}

	FVector LockOnRelativeLocation = NormalCameraRelativeLocation;
	LockOnRelativeLocation.Y += 30.f;
	LockOnRelativeLocation.X -= 50.f;
	LockOnRelativeLocation.Z += 30.f;

	springArmComp->SetRelativeLocation(LockOnRelativeLocation);
	springArmComp->TargetArmLength = NormalCameraArmLength;
	springArmComp->SocketOffset = NormalCameraSocketOffset;
}

void AKnight::ApplyChestOpenCamera()
{
	if (!springArmComp)
	{
		return;
	}

	springArmComp->SetRelativeLocation(ChestOpenCameraRelativeLocation);
	springArmComp->TargetArmLength = ChestOpenCameraArmLength;
	springArmComp->SocketOffset = ChestOpenCameraSocketOffset;
}

void AKnight::RestoreCameraAfterChestOpen()
{
	if (bIsLockOnActive)
	{
		ApplyLockOnCamera();
	}
	else
	{
		ApplyNormalCamera();
	}
}

void AKnight::ShowBossFelledMessage(const FText& Message, float Duration)
{
	if (!BossFelledWidgetClass)
	{
		return;
	}

	if (BossFelledWidget)
	{
		BossFelledWidget->RemoveFromParent();
		BossFelledWidget = nullptr;
	}

	BossFelledWidget = CreateWidget<UBossFelledWidget>(GetWorld(), BossFelledWidgetClass);
	if (!BossFelledWidget)
	{
		return;
	}

	BossFelledWidget->SetBossFelledText(Message);
	BossFelledWidget->AddToViewport(900);

	GetWorldTimerManager().ClearTimer(TimerHandle_BossFelledMessage);
	GetWorldTimerManager().SetTimer(
		TimerHandle_BossFelledMessage,
		this,
		&AKnight::HideBossFelledMessage,
		FMath::Max(0.1f, Duration),
		false
	);
}

void AKnight::HideBossFelledMessage()
{
	if (BossFelledWidget)
	{
		BossFelledWidget->RemoveFromParent();
		BossFelledWidget = nullptr;
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_BossFelledMessage);
}

bool AKnight::ShouldPlayFootstepSound() const
{
	if (bIsDead || bIsHitReacting || bIsDodging || bIsAttacking || bIsDrinking ||
		bIsUsingSkill || bIsBackAttacking || bIsOpeningDoor || bIsOpeningChest ||
		bIsBonfireInteracting || bIsRestingAtBonfire || bIsGrabbedByBoss ||
		bIsKnockedDownByBoss || bIsGettingUpFromBossKnockDown || bIsWeaponBouncing || bIsPlayingItemPickupMontage)
	{
		return false;
	}

	if (!GetCharacterMovement())
	{
		return false;
	}

	if (!GetCharacterMovement()->IsMovingOnGround())
	{
		return false;
	}

	const float Speed2D = GetVelocity().Size2D();
	if (Speed2D < FootstepMinSpeed)
	{
		return false;
	}

	return true;
}

USoundBase* AKnight::GetRandomFootstepSound(const TArray<USoundBase*>& Sounds) const
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

void AKnight::UpdateFootstepSound(float DeltaTime)
{
	if (!ShouldPlayFootstepSound())
	{
		LastFootstepLocation = GetActorLocation();
		FootstepDistanceAccumulator = 0.f;
		return;
	}

	const FVector CurrentLocation = GetActorLocation();

	FVector Delta = CurrentLocation - LastFootstepLocation;
	Delta.Z = 0.f;

	LastFootstepLocation = CurrentLocation;

	const float MovedDistance = Delta.Size();
	FootstepDistanceAccumulator += MovedDistance;

	const float CurrentSpeed = GetVelocity().Size2D();

	const bool bRunFootstep =
		bIsRunning || CurrentSpeed >= RunFootstepSpeedThreshold;

	const float RequiredDistance =
		bRunFootstep ? RunFootstepDistance : WalkFootstepDistance;

	if (FootstepDistanceAccumulator >= RequiredDistance)
	{
		FootstepDistanceAccumulator = 0.f;
		PlayFootstepSound(bRunFootstep);
	}
}

void AKnight::PlayFootstepSound(bool bRunFootstep)
{
	const TArray<USoundBase*>& SoundArray =
		bRunFootstep ? RunFootstepSounds : WalkFootstepSounds;

	USoundBase* Sound = GetRandomFootstepSound(SoundArray);
	if (!Sound)
	{
		return;
	}

	const float Volume =
		bRunFootstep ? RunFootstepVolume : WalkFootstepVolume;

	const float Pitch = bRunFootstep
		? FMath::FRandRange(RunFootstepPitchMin, RunFootstepPitchMax)
		: FMath::FRandRange(WalkFootstepPitchMin, WalkFootstepPitchMax);

	FVector SoundLocation = GetActorLocation();

	if (GetMesh())
	{
		const FName FootSocketName =
			bUseLeftFootForNextStep ? LeftFootSocketName : RightFootSocketName;

		if (!FootSocketName.IsNone() && GetMesh()->DoesSocketExist(FootSocketName))
		{
			SoundLocation = GetMesh()->GetSocketLocation(FootSocketName);
		}
	}

	bUseLeftFootForNextStep = !bUseLeftFootForNextStep;

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		SoundLocation,
		Volume,
		Pitch
	);
}

bool AKnight::FindMemoryShardDropPointTransform(FName PointTag, FTransform& OutTransform) const
{
	if (PointTag.IsNone() || !GetWorld())
	{
		return false;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this)
		{
			continue;
		}

		if (Actor->ActorHasTag(PointTag))
		{
			OutTransform = Actor->GetActorTransform();
			return true;
		}
	}

	return false;
}

void AKnight::EnableBossKnockDownGetUp()
{
	if (bIsDead || !bIsKnockedDownByBoss)
	{
		return;
	}

	bCanGetUpFromBossKnockDown = true;
}

void AKnight::ClearWeaponEffect()
{
	if (WeaponNiagaraEffectComp)
	{
		WeaponNiagaraEffectComp->Deactivate();
		WeaponNiagaraEffectComp->DestroyComponent();
		WeaponNiagaraEffectComp = nullptr;
	}

	if (WeaponParticleEffectComp)
	{
		WeaponParticleEffectComp->DeactivateSystem();
		WeaponParticleEffectComp->DestroyComponent();
		WeaponParticleEffectComp = nullptr;
	}

	ClearWeaponLight();
}

void AKnight::ApplyWeaponEffectFromItemData(const FItemData& ItemData)
{
	ClearWeaponEffect();

	if (!swordMeshComp)
	{
		return;
	}

	ApplyWeaponLightFromItemData(ItemData);

	if (ItemData.WeaponEffectType == EWeaponEffectType::None)
	{
		return;
	}

	const FName AttachSocketName = ItemData.WeaponEffectSocketName;

	if (ItemData.WeaponEffectType == EWeaponEffectType::Niagara)
	{
		if (!ItemData.WeaponNiagaraEffect)
		{
			return;
		}

		WeaponNiagaraEffectComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			ItemData.WeaponNiagaraEffect.Get(),
			swordMeshComp,
			AttachSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false,
			true
		);

		return;
	}

	if (ItemData.WeaponEffectType == EWeaponEffectType::Particle)
	{
		if (!ItemData.WeaponParticleEffect)
		{
			return;
		}

		WeaponParticleEffectComp = UGameplayStatics::SpawnEmitterAttached(
			ItemData.WeaponParticleEffect.Get(),
			swordMeshComp,
			AttachSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false
		);
	}
}

void AKnight::RefreshEquippedWeaponVisual()
{
	if (!InventoryComp)
	{
		ClearWeaponVisualToDefault();
		return;
	}

	const FInventoryItem& EquippedWeapon = InventoryComp->GetEquippedWeapon();

	if (EquippedWeapon.IsValid())
	{
		ApplyWeaponVisualFromItemData(EquippedWeapon.ItemData);
	}
	else
	{
		ClearWeaponVisualToDefault();
	}
}

void AKnight::ApplyWeaponVisualFromItemData(const FItemData& ItemData)
{
	if (!swordMeshComp)
	{
		return;
	}

	if (ItemData.EquipSlot != EEquipmentSlot::Weapon)
	{
		return;
	}

	UStaticMesh* NewWeaponMesh = ItemData.WeaponStaticMesh
		? ItemData.WeaponStaticMesh.Get()
		: DefaultWeaponStaticMesh;

	if (NewWeaponMesh)
	{
		swordMeshComp->SetStaticMesh(NewWeaponMesh);
	}

	ApplyWeaponEffectFromItemData(ItemData);

	UpdateSwordVisibilityByState();
}

void AKnight::ClearWeaponVisualToDefault()
{
	if (swordMeshComp && DefaultWeaponStaticMesh)
	{
		swordMeshComp->SetStaticMesh(DefaultWeaponStaticMesh);
	}

	ClearWeaponEffect();

	UpdateSwordVisibilityByState();
}

void AKnight::StartWeaponAttackTrail()
{
	StopWeaponAttackTrail();

	if (!swordMeshComp)
	{
		return;
	}

	EWeaponEffectType TrailType = DefaultWeaponAttackTrailEffectType;
	UNiagaraSystem* NiagaraTrail = DefaultWeaponAttackTrailEffect;
	UParticleSystem* ParticleTrail = DefaultWeaponAttackTrailParticleEffect;
	FName TrailSocketName = DefaultWeaponAttackTrailSocketName;

	if (InventoryComp)
	{
		const FInventoryItem& EquippedWeapon = InventoryComp->GetEquippedWeapon();

		if (EquippedWeapon.IsValid() && EquippedWeapon.ItemData.EquipSlot == EEquipmentSlot::Weapon)
		{
			const FItemData& ItemData = EquippedWeapon.ItemData;

			TrailType = ItemData.WeaponAttackTrailEffectType;
			NiagaraTrail = ItemData.WeaponAttackTrailEffect.Get();
			ParticleTrail = ItemData.WeaponAttackTrailParticleEffect.Get();
			TrailSocketName = ItemData.WeaponAttackTrailSocketName;
		}
	}

	if (TrailType == EWeaponEffectType::None)
	{
		return;
	}

	if (TrailSocketName.IsNone() || !swordMeshComp->DoesSocketExist(TrailSocketName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Weapon attack trail socket not found. Socket=%s, Mesh=%s"),
			*TrailSocketName.ToString(),
			swordMeshComp->GetStaticMesh()
			? *swordMeshComp->GetStaticMesh()->GetName()
			: TEXT("None")
		);

		return;
	}

	if (TrailType == EWeaponEffectType::Niagara)
	{
		if (!NiagaraTrail)
		{
			return;
		}

		WeaponAttackTrailComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraTrail,
			swordMeshComp,
			TrailSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false,
			true
		);

		return;
	}

	if (TrailType == EWeaponEffectType::Particle)
	{
		if (!ParticleTrail)
		{
			return;
		}

		WeaponAttackTrailParticleComp = UGameplayStatics::SpawnEmitterAttached(
			ParticleTrail,
			swordMeshComp,
			TrailSocketName,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);

		return;
	}
}

void AKnight::StopWeaponAttackTrail()
{
	if (WeaponAttackTrailComp)
	{
		WeaponAttackTrailComp->Deactivate();
		WeaponAttackTrailComp->DestroyComponent();
		WeaponAttackTrailComp = nullptr;
	}

	if (WeaponAttackTrailParticleComp)
	{
		WeaponAttackTrailParticleComp->DeactivateSystem();
		WeaponAttackTrailParticleComp->DestroyComponent();
		WeaponAttackTrailParticleComp = nullptr;
	}
}

bool AKnight::StartItemPickupMontage()
{
	if (bIsDead || bIsPlayingItemPickupMontage)
	{
		return false;
	}

	// 다른 행동 중이면 아이템 줍기 불가
	if (bIsHitReacting || bIsDodging || bIsAttacking || bIsDrinking ||
		bIsUsingSkill || bIsBackAttacking || bIsKnockedDownByBoss ||
		bIsOpeningDoor || bIsGettingUpFromBossKnockDown ||
		bIsWeaponBouncing || bIsOpeningChest || bIsBonfireInteracting ||
		bIsRestingAtBonfire || bIsGrabbedByBoss)
	{
		return false;
	}

	bIsPlayingItemPickupMontage = true;

	// 이동/달리기 정지
	bIsRunning = false;
	MoveInput = FVector2D::ZeroVector;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

	// 무기 자동 집어넣기 타이머 같은 것 정리
	GetWorldTimerManager().ClearTimer(TimerHandle_AutoSheatheWeapon);
	GetWorldTimerManager().ClearTimer(TimerHandle_SheatheWeapon);
	bIsSheathingWeapon = false;

	float Duration = 0.4f;

	if (ItemPickupMontage)
	{
		const float PlayedDuration = PlayAnimMontage(ItemPickupMontage);
		if (PlayedDuration > 0.f)
		{
			Duration = PlayedDuration;
		}
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_ItemPickupMontage);
	GetWorldTimerManager().SetTimer(
		TimerHandle_ItemPickupMontage,
		this,
		&AKnight::FinishItemPickupMontage,
		Duration,
		false
	);

	UpdateSwordVisibilityByState();

	return true;
}

void AKnight::FinishItemPickupMontage()
{
	if (bIsDead)
	{
		return;
	}

	bIsPlayingItemPickupMontage = false;
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;

	UpdateSwordVisibilityByState();

	RefreshItemDropInteractActor();
}

void AKnight::PlayPlayerAttackHitSuccessSoundAtLocation(const FVector& Location)
{
	if (PlayerAttackHitSuccessSounds.Num() <= 0)
	{
		return;
	}

	TArray<USoundBase*> ValidSounds;
	for (USoundBase* Sound : PlayerAttackHitSuccessSounds)
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
	const float Pitch = FMath::RandRange(
		PlayerAttackHitSuccessPitchMin,
		PlayerAttackHitSuccessPitchMax
	);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		ValidSounds[RandIndex],
		Location,
		PlayerAttackHitSuccessVolume,
		Pitch
	);
}

void AKnight::SpawnPlayerAttackHitBloodEffect(const FVector& Location, const FRotator& Rotation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector EffectScale(PlayerAttackHitBloodEffectScale);

	if (PlayerAttackHitBloodNiagaraEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			PlayerAttackHitBloodNiagaraEffect,
			Location,
			Rotation,
			EffectScale
		);
	}

	if (PlayerAttackHitBloodParticleEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			PlayerAttackHitBloodParticleEffect,
			Location,
			Rotation,
			EffectScale,
			true
		);
	}
}

void AKnight::ClearWeaponLight()
{
	if (WeaponLightComp)
	{
		WeaponLightComp->DestroyComponent();
		WeaponLightComp = nullptr;
	}
}

void AKnight::ApplyWeaponLightFromItemData(const FItemData& ItemData)
{
	ClearWeaponLight();

	if (!swordMeshComp)
	{
		return;
	}

	if (!ItemData.bUseWeaponLight)
	{
		return;
	}

	WeaponLightComp = NewObject<UPointLightComponent>(this, UPointLightComponent::StaticClass());
	if (!WeaponLightComp)
	{
		return;
	}

	WeaponLightComp->RegisterComponent();

	const FName AttachSocketName = ItemData.WeaponLightSocketName;

	if (!AttachSocketName.IsNone() && swordMeshComp->DoesSocketExist(AttachSocketName))
	{
		WeaponLightComp->AttachToComponent(
			swordMeshComp,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName
		);
	}
	else
	{
		WeaponLightComp->AttachToComponent(
			swordMeshComp,
			FAttachmentTransformRules::KeepRelativeTransform
		);
	}

	WeaponLightComp->SetRelativeLocation(ItemData.WeaponLightRelativeLocation);
	WeaponLightComp->SetLightColor(ItemData.WeaponLightColor);
	WeaponLightComp->SetIntensity(ItemData.WeaponLightIntensity);
	WeaponLightComp->SetAttenuationRadius(ItemData.WeaponLightRadius);

	WeaponLightComp->SetCastShadows(false);

	WeaponLightComp->SetVisibility(!swordMeshComp->bHiddenInGame);
	WeaponLightComp->SetHiddenInGame(swordMeshComp->bHiddenInGame);
}

void AKnight::UpdateWeaponLightVisibility()
{
	if (!WeaponLightComp || !swordMeshComp)
	{
		return;
	}

	const bool bVisible = !swordMeshComp->bHiddenInGame && swordMeshComp->IsVisible();

	WeaponLightComp->SetVisibility(bVisible, true);
	WeaponLightComp->SetHiddenInGame(!bVisible, true);

	if (bVisible)
	{
		WeaponLightComp->Activate();
	}
	else
	{
		WeaponLightComp->Deactivate();
	}
}

void AKnight::HandleEscapeInput(const FInputActionValue& InputValue)
{
	// 사망 패널티 튜토리얼이 떠 있으면 최우선으로 닫기
	if (DeathPenaltyTutorialWidget && DeathPenaltyTutorialWidget->IsInViewport())
	{
		BlockPauseMenuOpenForOneFrame();
		CloseDeathPenaltyTutorial();
		return;
	}

	// 상자 열기 중 ESC로 튜토리얼만 닫히고 상태가 꼬인 경우 강제 복구
	if (bIsOpeningChest)
	{
		BlockPauseMenuOpenForOneFrame();
		FinishChestOpen();
		return;
	}

	// 여기에서만 Pause 메뉴 열기
	// TogglePauseMenu();
}

void AKnight::OnTutorialPopupClosedFromWidget()
{
	BlockPauseMenuOpenForOneFrame();

	// 상자 여는 중 튜토리얼을 ESC로 닫으면 bIsOpeningChest 때문에 PlayerMove가 계속 막힐 수 있음
	if (bIsOpeningChest)
	{
		FinishChestOpen();
		return;
	}

	RestoreGameplayInputFromUI(true);
}

void AKnight::BlockPauseMenuOpenForOneFrame()
{
	bBlockPauseMenuOpenThisFrame = true;

	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&AKnight::ClearPauseMenuOpenBlock
	);
}

void AKnight::ClearPauseMenuOpenBlock()
{
	bBlockPauseMenuOpenThisFrame = false;
}

void AKnight::ApplyDeathPenaltyTutorialInputMode()
{
	if (!DeathPenaltyTutorialWidget || !DeathPenaltyTutorialWidget->IsInViewport())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;

	InputMode.SetWidgetToFocus(DeathPenaltyTutorialWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);

	PC->SetInputMode(InputMode);

	DeathPenaltyTutorialWidget->SetUserFocus(PC);
	DeathPenaltyTutorialWidget->SetKeyboardFocus();

	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);

	// SetInputMode 이후에 한 번 더 켜주는 게 안전함
	PC->bShowMouseCursor = true;
}