#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerEnhancementTypes.h"
#include "ItemTypes.h"
#include "Knight.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UPrimitiveComponent;
class ABonfire;
class UInventoryComponent;
class UFortressGameInstance;
class USoundBase;
class ABaseMonster;
class AMimicChestMonster;
class UBossFelledWidget;
class UParticleSystem;
class UParticleSystemComponent;
class UPointLightComponent;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChanged, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExhaustedChanged, bool, bNewExhausted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExhaustedTimeChanged, float, RemainingTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryShardChanged, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryShardGainChanged, int32, GainValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHPPenaltyChanged, float, PenaltyPercent);

UENUM(BlueprintType)
enum class EDodgeDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

USTRUCT()
struct FRecentSafeTransformEntry
{
	GENERATED_BODY()

	UPROPERTY()
	float TimeSeconds = 0.f;

	UPROPERTY()
	FTransform Transform = FTransform::Identity;
};

UCLASS()
class COO_API AKnight : public ACharacter
{
	GENERATED_BODY()

public:
	AKnight();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Camera
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* springArmComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* playerCamComp;

	// ===== Camera Settings =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Normal")
	FVector NormalCameraRelativeLocation = FVector(0.f, 0.f, 160.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Normal")
	float NormalCameraArmLength = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Normal")
	FVector NormalCameraSocketOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	FVector LockOnCameraRelativeLocation = FVector(0.f, 0.f, 190.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	float LockOnCameraArmLength = 540.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|LockOn")
	FVector LockOnCameraSocketOffset = FVector(0.f, 0.f, 25.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Chest")
	FVector ChestOpenCameraRelativeLocation = FVector(0.f, 0.f, 240.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Chest")
	float ChestOpenCameraArmLength = 430.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Chest")
	FVector ChestOpenCameraSocketOffset = FVector(0.f, 0.f, 45.f);

	void ApplyNormalCamera();
	void ApplyLockOnCamera();
	void ApplyChestOpenCamera();
	void RestoreCameraAfterChestOpen();

	// 걷기 속도
	UPROPERTY(EditAnywhere, Category = PlayerSetting)
	float walkSpeed = 300.0f;
	// 달리기 속도
	UPROPERTY(EditAnywhere, Category = PlayerSetting)
	float runSpeed = 600.0f;
	// 포션 마실 때 걷기 속도
	UPROPERTY(EditAnywhere, Category = PlayerSetting)
	float DrinkSpeed = 150.0f;
	// 이동 방향
	FVector2D MoveInput = FVector2D::ZeroVector;
	// 마지막 이동 입력값 저장용
	FVector2D LastMoveInput = FVector2D::ZeroVector;

	// Input
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* imc_Player;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_LookUp;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Turn;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Move; // 이동
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Dodge; // 구르기
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Drink; // 포션 마시기
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Run; // 달리기
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Attack; // 공격
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Skill; // 스킬
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_LockOn; // 락온
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Inventory; // 인벤토리 열기
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Pause; // 일시정지
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* ia_Interact; // 상호작용

	void Turn(const struct FInputActionValue& inputValue);
	void LookUp(const struct FInputActionValue& inputValue);
	void Move(const struct FInputActionValue& inputValue);
	void Dodge(const struct FInputActionValue& inputValue);
	void Attack(const struct FInputActionValue& inputValue);
	void PlayerMove();
	void StartRun(const struct FInputActionValue& inputValue);
	void StopRun(const struct FInputActionValue& inputValue);
	void LockOn();

	// 검 스켈레탈메시
	UPROPERTY(VisibleAnywhere, Category = SwordMesh)
	class UStaticMeshComponent* swordMeshComp;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachSwordToHand();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachSwordToWaist();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DrawSwordForCombat();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ScheduleAutoSheatheWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartSheatheWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FinishSheatheWeapon();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsWeaponDrawn() const { return bWeaponDrawn; }

	UFUNCTION(BlueprintPure, Category = "BossHUD")
	bool IsInBossBattle() const;

	UFUNCTION(BlueprintPure, Category = "Dodge")
	bool IsInvincible() const { return bIsInvincible; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	float BaseMaxHP = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	float BaseMaxStamina = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	float BaseAttackPower = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	int32 BaseMaxPotionCount = 3;

	void ApplyEnhancementDataToStats(bool bFillCurrentValues);

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetAttackPower() const { return AttackPower; }

	UFUNCTION(BlueprintCallable, Category = "Status")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void RecoverStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Status")
	bool UseStamina(float Amount);

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FOnStatChanged OnHPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FOnStatChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FOnExhaustedChanged OnExhaustedChanged;

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FOnExhaustedTimeChanged OnExhaustedTimeChanged;

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetExhaustedRemainingTime() const { return ExhaustedRemainingTime; }

	UFUNCTION(BlueprintCallable, Category = "Attack")
	float GetCurrentAttackDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void OnAttackEnded();

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void StartDodgeMove();

	UFUNCTION(BlueprintCallable, Category = "Dodge")
	void StopDodgeMove();

	void InterruptCurrentAction(); // 현재 행동을 즉시 중단
protected:

	// ===== Weapon Draw / Sheathe =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Socket")
	FName WeaponHandSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Socket")
	FName WeaponWaistSocketName = TEXT("WeaponWaistSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Transform")
	FVector WeaponHandRelativeLocation = FVector(-9.f, 3.f, -2.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Transform")
	FRotator WeaponHandRelativeRotation = FRotator(-90.f, 0.f, 90.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Transform")
	FVector WeaponWaistRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Transform")
	FRotator WeaponWaistRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	UAnimMontage* SheatheWeaponMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Timing")
	float AutoSheatheDelay = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bWeaponDrawn = false;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bIsSheathingWeapon = false;

	FTimerHandle TimerHandle_AutoSheatheWeapon;
	FTimerHandle TimerHandle_SheatheWeapon;

	// 구르기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	UAnimMontage* DodgeForwardMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	UAnimMontage* DodgeBackwardMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	UAnimMontage* DodgeLeftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	UAnimMontage* DodgeRightMontage;

	EDodgeDirection GetDodgeDirection(const FVector2D& InputValue) const;
	UAnimMontage* GetDodgeMontageByDirection(EDodgeDirection DodgeDir) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeInvincibleTime = 0.4f; // 구르기 무적 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeCooldown = 0.6f; // 구르기 쿨타임

	UPROPERTY(BlueprintReadOnly, Category = "Dodge")
	bool bIsDodging = false; // 구르기 상태 여부
	UPROPERTY(BlueprintReadOnly, Category = "Dodge")
	bool bIsInvincible = false; // 무적	상태 여부
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	//float DodgeDistance = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeMoveDuration = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge")
	float DodgeMoveDistance = 500.f;

	bool bDodgeMoving = false;
	float DodgeMoveElapsed = 0.f;

	FVector DodgeMoveDirection = FVector::ZeroVector;

	bool bCanDodge = true; // 구르기 가능 여부

	FTimerHandle TimerHandle_DodgeInvincible;
	FTimerHandle TimerHandle_DodgeEnd;
	FTimerHandle TimerHandle_DodgeCooldown;

	FVector GetCameraRelativeMoveDirection(const FVector2D& InputValue) const;

	void StartInvincible();
	void EndInvincible();
	void EndDodge();
	void ResetDodgeCooldown();

	// 스테이터스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float MaxHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float CurrentHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float MaxStamina = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float CurrentStamina = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float AttackPower = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float StaminaRegenRate = 3.f;

	// 탈진/달리기 상태
	UPROPERTY(BlueprintReadOnly, Category = "Status")
	bool bIsRunning = false;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	bool bIsExhausted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float ExhaustedRemainingTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float RunStaminaDrainRate = 3.33f; // 초당 기력 소모

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float ExhaustedMoveSpeed = 0.f; // 탈진 시 이동속도

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float ExhaustedDuration = 4.f; // 탈진 지속 시간

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float ExhaustedRecoveryStamina = 25.f; // 탈진 종료 후 회복 기력

	FTimerHandle TimerHandle_Exhausted;

	void UpdateStamina(float DeltaTime);
	void EnterExhaustedState();
	void EndExhaustedState();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exhausted")
	UAnimMontage* ExhaustedMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	UNiagaraSystem* ExhaustedEffect;
	UPROPERTY()
	UNiagaraComponent* ExhaustedEffectComp;

	void StartExhaustedEffect();
	void StopExhaustedEffect();

	UFUNCTION(BlueprintPure, Category = "Status")
	bool IsRunning() const { return bIsRunning; }

	UFUNCTION(BlueprintPure, Category = "Status")
	bool IsExhausted() const {
		return bIsExhausted;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> PlayerWidgetClass;

	UPROPERTY()
	class UUserWidget* PlayerWidget;

	// 일반 공격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	class UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float AttackStaminaCost = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float ComboResetTime = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bComboInputQueued = false;

	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	int32 CurrentComboIndex = 0; // 0이면 대기, 1~3 콤보 진행
	
	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bIsComboWindowActive = false;

	FTimerHandle TimerHandle_ComboReset;

	void ResetCombo();

	FName GetAttackSectionNameByIndex(int32 ComboIndex) const;
	void PlayAttackSection(int32 ComboIndex);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Attack")
	bool bAttackTraceActive = false;

	void PerformAttackTrace();

	UPROPERTY()
	TArray<AActor*> AlreadyHitActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float AttackTraceRadius = 12.f;

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void StartAttackTrace();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void EndAttackTrace();

	FVector PrevTraceEnd = FVector::ZeroVector;

protected:
	// Hit, Death
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float HitInvincibleTime = 0.2f; // 피격 후 무적 시간

	FTimerHandle TimerHandle_HitInvincible;

	void EndHitInvincible();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death")
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death|Audio")
	TArray<USoundBase*> DeathSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death|Audio")
	float DeathSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death|Audio")
	float DeathSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death|Audio")
	float DeathSoundPitchMax = 1.04f;

	USoundBase* GetRandomDeathSound() const;
	void PlayDeathSound();

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsHitReacting = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;

	UPROPERTY()
	class UUserWidget* GameOverWidget;

	FTimerHandle TimerHandle_EndHitReact;

	void EndHitReact();

	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bHitReactionInvincible = false;

	UPROPERTY(BlueprintReadOnly, Category = "Hit")
	bool bCanDodgeDuringHitReaction = false;

	void CancelHitReactionForDodge();

protected:
	// 락온
	FVector GetLockOnMoveDirection(const FVector2D& InputValue) const;

	UPROPERTY(BlueprintReadOnly, Category = "LockOn")
	bool bIsLockOnActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "LockOn")
	class ABaseMonster* LockOnTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float LockOnSearchRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LockOn")
	float LockOnRotateInterpSpeed = 12.f;

	void UpdateLockOn(float DeltaTime);
	void SetLockOnTarget(class ABaseMonster* NewTarget);
	void ClearLockOn();
	TArray<ABaseMonster*> FindLockOnCandidates() const;
	ABaseMonster* FindBestInitialLockOnTarget() const;
	ABaseMonster* FindNextLockOnTarget() const;

	bool bLockOnButtonHeld = false;
	bool bLockOnHoldTriggered = false;
	FTimerHandle TimerHandle_LockOnHold;

	void OnLockOnPressed();
	void OnLockOnReleased();
	void HandleLockOnHold();

protected:
	// 체력 회복 (포션 마시기)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potion")
	UAnimMontage* DrinkMontage;

	UPROPERTY(BlueprintReadOnly, Category = "Potion")
	bool bIsDrinking = false;
	UPROPERTY(BlueprintReadOnly, Category = "Potion")
	float PotionHealAmount = MaxHP / 2; // 포션 1개당 회복량 (최대 체력의 절반)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potion")
	int32 MaxPotionCount = 3; // 최대 포션 개수
	UPROPERTY(BlueprintReadOnly, Category = "Potion")
	int32 CurrentPotionCount = MaxPotionCount; // 현재 포션 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	UNiagaraSystem* DrinkEffect; // 포션 마실 때 효과

	UNiagaraComponent* DrinkEffectComp;

	void StartShowDrinkEffect();
	void StopShowDrinkEffect();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potion|Audio")
	TArray<USoundBase*> PotionHealSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potion|Audio")
	float PotionHealSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potion|Audio")
	float PotionHealSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Potion|Audio")
	float PotionHealSoundPitchMax = 1.04f;

	USoundBase* GetRandomPotionHealSound() const;
	void PlayPotionHealSound();

	FTimerHandle TimerHandle_Drink;
public:
	UFUNCTION(BlueprintCallable, Category = "Potion")
	void DrinkPotion();

protected:
	// 상호작용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interact")
	float InteractTraceDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interact")
	float InteractTraceRadius = 25.f;

	UPROPERTY(BlueprintReadOnly, Category = "Interact")
	AActor* CurrentInteractActor = nullptr;

	void Interact();

public:
	UFUNCTION(BlueprintPure, Category = "Interact")
	AActor* GetCurrentInteractActor() const { return CurrentInteractActor; }

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void SetInteractActor(AActor* NewInteractActor);

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void ClearInteractActor(AActor* InteractActorToClear);

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void RefreshInteractActor();

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void RefreshItemDropInteractActor();

protected:
	// 기억의 불씨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	UAnimMontage* BonfireInteractMontage;

	UPROPERTY(BlueprintReadOnly, Category = "Bonfire")
	bool bIsRestingAtBonfire = false;

	UPROPERTY()
	ABonfire* CurrentBonfire = nullptr;

	FTimerHandle TimerHandle_BonfireInteract;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	UAnimMontage* BonfireSitMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	TSubclassOf<UUserWidget> BonfireMenuWidgetClass;

	UPROPERTY()
	UUserWidget* BonfireMenuWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Bonfire")
	bool bIsBonfireInteracting = false;

public:
	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void BeginBonfireInteract(ABonfire* Bonfire);

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void FinishBonfireInteract();

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void StartBonfireRest(ABonfire* Bonfire);

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void EndBonfireRest();

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void LeaveBonfireRest();

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void RestAtBonfire();

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void OnRestFinished();

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void TravelToBonfire(FName TargetLevelName, const FTransform& TargetTransform);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetPlayerHUDVisible(bool bVisible);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void ShowItemPickupNotice(const FItemData& ItemData, int32 Amount);

protected:
	// 스킬
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	UAnimMontage* SkillMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	UNiagaraSystem* SkillEffect;

	// 스킬 시전할 때 나는 소리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Audio")
	TArray<USoundBase*> SkillCastSounds;

	// 스킬 발동/폭발/타격 순간 소리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Audio")
	TArray<USoundBase*> SkillImpactSounds;

	// 스킬 쓸 때 플레이어 목소리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Audio")
	TArray<USoundBase*> SkillVoiceSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Audio")
	float SkillCastSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Audio")
	float SkillImpactSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Audio")
	float SkillVoiceVolume = 1.0f;

	USoundBase* GetRandomSkillSound(const TArray<USoundBase*>& Sounds) const;

	void PlaySkillCastSound();
	void PlaySkillImpactSound();
	void PlaySkillVoiceSound();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	float SkillDamageMultiplier = 5.f; // 현재 공격력 배수

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	float SkillRadius = 250.f; // 스킬 범위

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FName SkillEffectSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	float SkillStaminaCost = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	float SkillForwardDistance = 250.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	bool bIsUsingSkill = false;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	bool bHasSuperArmor = false;

	FTimerHandle TimerHandle_Skill;
	FTimerHandle TimerHandle_SkillEnd;

	void UseSkill(const struct FInputActionValue& inputValue);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	float SkillMoveDuration = 0.18f;

	// 현재 사용 중인 스킬 설정값
	UPROPERTY(BlueprintReadOnly, Category = "Skill|Runtime")
	UAnimMontage* ActiveSkillMontage = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Runtime")
	float ActiveSkillRadius = 250.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Runtime")
	float ActiveSkillForwardDistance = 250.f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Runtime")
	float ActiveSkillMoveDuration = 0.18f;

	UPROPERTY(BlueprintReadOnly, Category = "Skill|Runtime")
	float ActiveSkillTriggerTimeRatio = 0.5f;

	bool bSkillMoving = false;
	float SkillMoveElapsed = 0.f;
	FVector SkillMoveDirection = FVector::ZeroVector;

	void ResolveCurrentWeaponSkillSetting(
		UAnimMontage*& OutMontage,
		float& OutRadius,
		float& OutForwardDistance,
		float& OutMoveDuration,
		float& OutTriggerTimeRatio
	) const;

public:
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ActivateSkillEffectAndDamage();

	void RegisterBackAttackTarget(ABaseMonster* Monster);
	void UnregisterBackAttackTarget(ABaseMonster* Monster);

protected:
	void SpawnCurrentWeaponSkillEffect(const FVector& DefaultEffectLocation, const FRotator& EffectRotation);

	// 백어택
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack")
	UAnimMontage* BackAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack")
	float BackAttackRange = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack")
	float BackAttackInvincibleTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack")
	float BackAttackDamageMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack")
	float BackAttackKnockDownTime = 1.5f;

	UPROPERTY(BlueprintReadOnly, Category = "BackAttack")
	bool bIsBackAttacking = false;

	UPROPERTY()
	ABaseMonster* BackAttackTarget = nullptr;

	FTimerHandle TimerHandle_BackAttackInvincible;
	FTimerHandle TimerHandle_BackAttackEnd;

	bool TryStartBackAttack();
	void FinishBackAttack();

	UPROPERTY()
	TArray<ABaseMonster*> AvailableBackAttackTargets;

	ABaseMonster* FindBestBackAttackTarget() const;
	void ApplyBackAttackDamage();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "BossHit")
	bool bIsKnockedDownByBoss = false;

	UPROPERTY(BlueprintReadOnly, Category = "BossHit")
	bool bIsGettingUpFromBossKnockDown = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossHit")
	float BossGetUpInvincibleTime = 0.8f;

	FTimerHandle TimerHandle_BossGetUpEnd;

	void FinishBossGetUp();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossHit")
	UAnimMontage* KnockedDownMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossHit")
	UAnimMontage* GetUpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossHit")
	float KnockDownLaunchStrength = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossHit")
	float KnockDownUpwardStrength = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossHit")
	float BossKnockDownGetUpDelay = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "BossHit")
	bool bCanGetUpFromBossKnockDown = false;

	FTimerHandle TimerHandle_BossGetUpDelay;

	void EnableBossKnockDownGetUp();

public:
	UFUNCTION(BlueprintCallable, Category = "BossHit")
	void EnterBossKnockDown(AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "BossHit")
	void TryGetUpFromBossKnockDown();

	UFUNCTION(BlueprintPure, Category = "BossHit")
	bool IsBossKnockedDown() const { return bIsKnockedDownByBoss; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "BossHUD")
	ABaseMonster* CurrentBossHUDTarget = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RestoreGameplayInputFromUI(bool bUnpauseGame);

	UFUNCTION(BlueprintCallable, Category = "BossHUD")
	void SetCurrentBossHUDTarget(ABaseMonster* NewTarget);

	UFUNCTION(BlueprintPure, Category = "BossHUD")
	ABaseMonster* GetCurrentBossHUDTarget() const { return CurrentBossHUDTarget; }

	UFUNCTION(BlueprintPure, Category = "BossHUD")
	bool HasBossHUDTarget() const;

	UFUNCTION(BlueprintPure, Category = "BossHUD")
	float GetBossHPPercent() const;

	UFUNCTION(BlueprintPure, Category = "BossHUD")
	float GetBossGroggyPercent() const;

	UFUNCTION(BlueprintPure, Category = "BossHUD")
	FText GetBossDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	FText GetMemoryShardCountText() const
	{
		return FText::AsNumber(CurrentMemoryShardCount);
	}

	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	FText GetMemoryShardGainText() const
	{
		if (RecentMemoryShardGain <= 0)
		{
			return FText::GetEmpty();
		}

		return FText::Format(
			FText::FromString(TEXT("+{0}")),
			FText::AsNumber(RecentMemoryShardGain)
		);
	}

	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	bool HasRecentMemoryShardGain() const
	{
		return RecentMemoryShardGain > 0;
	}

public:
	UPROPERTY(BlueprintAssignable, Category = "MemoryShard")
	FOnMemoryShardChanged OnMemoryShardChanged;

	UPROPERTY(BlueprintAssignable, Category = "MemoryShard")
	FOnMemoryShardGainChanged OnMemoryShardGainChanged;

	bool FindMemoryShardDropPointTransform(FName PointTag, FTransform& OutTransform) const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "MemoryShard")
	int32 CurrentMemoryShardCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "MemoryShard")
	int32 RecentMemoryShardGain = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MemoryShard|FinalBoss")
	FName FinalBossPhase2LevelName = TEXT("FinalBossPhase2");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MemoryShard|FinalBoss")
	FName FinalBossPhase1LevelName = TEXT("FinalBossPhase1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MemoryShard|FinalBoss")
	FName FinalBossPhase1DropPointTag = TEXT("FinalBossPhase1_MemoryShardDrop");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MemoryShard")
	TSubclassOf<class AMemoryShardDrop> MemoryShardDropClass;

	UPROPERTY()
	class AMemoryShardDrop* SpawnedMemoryShardDrop = nullptr;

	FTimerHandle TimerHandle_ClearMemoryShardGain;

public:
	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	int32 GetCurrentMemoryShardCount() const { return CurrentMemoryShardCount; }

	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	int32 GetRecentMemoryShardGain() const { return RecentMemoryShardGain; }

	UFUNCTION(BlueprintCallable, Category = "MemoryShard")
	void AddMemoryShards(int32 Amount, bool bShowGainPopup = true);

	UFUNCTION(BlueprintCallable, Category = "MemoryShard")
	void SetMemoryShards(int32 NewValue);

protected:
	void SpawnPendingMemoryShardDropFromGI();
	void ClearRecentMemoryShardGain();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	UInventoryComponent* InventoryComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> InventoryWidgetClass;

	UPROPERTY()
	class UUserWidget* InventoryWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bIsInventoryOpen = false;

	void ToggleInventory(const struct FInputActionValue& inputValue);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FName SkillStoneItemID = TEXT("SkillStone");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 SkillStoneCost = 5;

	void LoadInventoryFromGameInstance();
	void ReapplyEquippedItemBonusesFromInventory();

	UFUNCTION()
	void HandleInventoryUpdated();

public:
	void SaveInventoryToGameInstance();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenInventory();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseInventory();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UInventoryComponent* GetInventoryComp() const { return InventoryComp; }

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsInventoryOpen() const { return bIsInventoryOpen; }

	UFUNCTION(BlueprintCallable, Category = "Status")
	void ApplyEquipmentBonus(float HPBonus, float AttackBonus, float StaminaBonus);

	UFUNCTION(BlueprintCallable, Category = "Status")
	void RemoveEquipmentBonus(float HPBonus, float AttackBonus, float StaminaBonus);

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetAttackPowerValue() const { return AttackPower; }

	UFUNCTION(BlueprintPure, Category = "Skill")
	int32 GetSkillStoneCount() const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	FText GetSkillStoneCountText() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Enhance")
	FPlayerEnhancementData EnhancementData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	int32 MaxEnhanceLevel = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	int32 HPIncreasePerLevel = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	int32 StaminaIncreasePerLevel = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	int32 AttackIncreasePerLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	FName StatusResetTicketItemID = TEXT("StatusResetTicket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhance")
	FName MemoryFruitItemID = TEXT("MemoryFruit");

public:
	UFUNCTION(BlueprintPure, Category = "Enhance")
	const FPlayerEnhancementData& GetEnhancementData() const { return EnhancementData; }

	UFUNCTION(BlueprintPure, Category = "Enhance")
	bool CanUpgradeHP() const;

	UFUNCTION(BlueprintPure, Category = "Enhance")
	bool CanUpgradeStamina() const;

	UFUNCTION(BlueprintPure, Category = "Enhance")
	bool CanUpgradeAttack() const;

	UFUNCTION(BlueprintCallable, Category = "Enhance")
	bool UpgradeHPStat();

	UFUNCTION(BlueprintCallable, Category = "Enhance")
	bool UpgradeStaminaStat();

	UFUNCTION(BlueprintCallable, Category = "Enhance")
	bool UpgradeAttackStat();

	UFUNCTION(BlueprintCallable, Category = "Enhance")
	bool ResetEnhancedStats();

	UFUNCTION(BlueprintCallable, Category = "Enhance")
	bool UpgradePotionCapacity();

	void SaveEnhancementToGameInstance();
	void LoadEnhancementFromGameInstance();

	UFUNCTION(BlueprintPure, Category = "Enhance")
	int32 GetNextEnhanceCost() const { return EnhancementData.NextSharedCost; }

public:
	UPROPERTY(BlueprintAssignable, Category = "Status")
	FOnHPPenaltyChanged OnHPPenaltyChanged;

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetCurrentHPPenaltyPercent() const { return CurrentHPPenaltyPercent; }

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetEffectiveMaxHP() const;

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetEffectiveHPPercentOfFullMax() const;

	UFUNCTION(BlueprintPure, Category = "Status")
	float GetLostHPPenaltyPercentOfFullMax() const;

	UFUNCTION(BlueprintCallable, Category = "Status")
	void ClearAllHPPenalty();

	UFUNCTION(BlueprintCallable, Category = "Status")
	void HealByMaxHPPercent(float Percent);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float HPPenaltyStepPercent = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float MaxHPPenaltyPercent = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Status")
	float CurrentHPPenaltyPercent = 0.f;

	void IncreaseDeathHPPenalty();
	void ClampHPToEffectiveMax();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Door")
	bool bIsOpeningDoor = false;

	FTimerHandle TimerHandle_DoorInteraction;

public:
	UFUNCTION(BlueprintCallable, Category = "Door")
	void BeginDoorInteraction(float Duration);

	UFUNCTION(BlueprintCallable, Category = "Door")
	void FinishDoorInteraction();

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsOpeningDoor() const { return bIsOpeningDoor; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "BossHit")
	bool bIsGrabbedByBoss = false;

	UPROPERTY()
	TEnumAsByte<ECollisionResponse> SavedMonsterCollisionResponse = ECR_Block;

public:
	UFUNCTION(BlueprintCallable, Category = "BossHit")
	void SetGrabbedByBoss(bool bNewGrabbed);

	UFUNCTION(BlueprintPure, Category = "BossHit")
	bool IsGrabbedByBoss() const { return bIsGrabbedByBoss; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro")
	FName TownLevelName = TEXT("Stage0");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro")
	TSubclassOf<UUserWidget> TownIntroWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intro")
	float TownIntroDuration = 8.0f;

	UPROPERTY()
	UUserWidget* TownIntroWidget = nullptr;

	FTimerHandle TimerHandle_TownIntroEnd;

	void TryPlayTownIntro();
	void FinishTownIntro();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MemoryShard")
	float MemoryShardSafeDropDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MemoryShard")
	float MemoryShardSafeHistoryLength = 3.0f;

	UPROPERTY()
	TArray<FRecentSafeTransformEntry> RecentSafeTransforms;

	void UpdateRecentSafeTransforms(float DeltaTime);
	FTransform GetSafeMemoryShardDropTransform() const;

	void DropMemoryShardsOnDeath(bool bUseSafeFallback);

	void HandleDeath(bool bUseSafeDropFallback = false);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TSubclassOf<UUserWidget> DeathPenaltyTutorialWidgetClass;

	UPROPERTY()
	UUserWidget* DeathPenaltyTutorialWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FName DeathPenaltyTutorialID = TEXT("DeathPenalty");

	void TryShowDeathPenaltyTutorial();

	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> LoadingWidgetClass;

	UPROPERTY()
	UUserWidget* LoadingWidget = nullptr;

	FTimerHandle TimerHandle_LoadingScreen;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLoadingScreen(float Duration);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideLoadingScreen();

protected:
	// 검 튕김
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Bounce")
	bool bEnableWeaponBounce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Bounce")
	UAnimMontage* WeaponBounceMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Bounce")
	float WeaponBounceFallbackTime = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "Attack|Bounce")
	bool bIsWeaponBouncing = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Bounce|Audio")
	TArray<USoundBase*> WeaponBounceSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Bounce|Audio")
	float WeaponBounceSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Bounce|Audio")
	float WeaponBounceSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Bounce|Audio")
	float WeaponBounceSoundPitchMax = 1.04f;

	USoundBase* GetRandomWeaponBounceSound() const;
	void PlayWeaponBounceSound();

	FTimerHandle TimerHandle_WeaponBounce;

	bool FindWeaponBounceHit(
		const FVector& TraceStart,
		const FVector& TraceEnd,
		const TArray<AActor*>& ActorsToIgnore,
		FHitResult& OutHit
	) const;

	void TriggerWeaponBounce(const FHitResult& Hit);
	void EndWeaponBounce();

	bool TryMoveToPendingPortalArrival(UFortressGameInstance* GI, FName CurrentLevelName);
	bool FindPortalArrivalTransform(FName ArrivalTag, FTransform& OutTransform) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Attack")
	TArray<USoundBase*> WeaponSwingSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Attack")
	TArray<USoundBase*> AttackVoiceSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Attack")
	float AttackSFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Attack")
	float VoiceVolume = 1.0f;

	USoundBase* GetRandomSound(const TArray<USoundBase*>& Sounds) const;

	// ===== BackAttack Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack|Audio")
	TArray<USoundBase*> BackAttackStartSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack|Audio")
	TArray<USoundBase*> BackAttackImpactSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack|Audio")
	TArray<USoundBase*> GroggyAttackStartSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack|Audio")
	TArray<USoundBase*> GroggyAttackImpactSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack|Audio")
	float BackAttackSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack|Audio")
	float BackAttackSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BackAttack|Audio")
	float BackAttackSoundPitchMax = 1.04f;

	USoundBase* GetRandomBackAttackSound(const TArray<USoundBase*>& Sounds) const;
	void PlayBackAttackSound(const TArray<USoundBase*>& Sounds);
	void PlayBackAttackStartSound();
	void PlayBackAttackImpactSound();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest")
	UAnimMontage* ChestOpenMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest")
	float ChestOpenTriggerTimeRatio = 0.45f;

	UPROPERTY(BlueprintReadOnly, Category = "Chest")
	bool bChestOpenTriggered = false;

	FTimerHandle TimerHandle_ChestOpenTrigger;

	UPROPERTY(BlueprintReadOnly, Category = "Chest")
	bool bIsOpeningChest = false;

	UPROPERTY()
	AMimicChestMonster* PendingChestToOpen = nullptr;

	FTimerHandle TimerHandle_ChestOpen;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Audio")
	TArray<USoundBase*> SheatheWeaponSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Audio")
	float SheatheWeaponSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Audio")
	float SheatheWeaponSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Audio")
	float SheatheWeaponSoundPitchMax = 1.04f;

	USoundBase* GetRandomSheatheWeaponSound() const;
	void PlaySheatheWeaponSound();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Boss")
	TSubclassOf<UBossFelledWidget> BossFelledWidgetClass;

	UPROPERTY()
	UBossFelledWidget* BossFelledWidget = nullptr;

	FTimerHandle TimerHandle_BossFelledMessage;

	// ===== Footstep Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Audio")
	TArray<USoundBase*> WalkFootstepSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Audio")
	TArray<USoundBase*> RunFootstepSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Audio")
	float WalkFootstepVolume = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Audio")
	float RunFootstepVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Audio")
	float WalkFootstepPitchMin = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Audio")
	float WalkFootstepPitchMax = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Audio")
	float RunFootstepPitchMin = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Audio")
	float RunFootstepPitchMax = 1.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Timing")
	float WalkFootstepDistance = 155.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Timing")
	float RunFootstepDistance = 105.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Timing")
	float FootstepMinSpeed = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Timing")
	float RunFootstepSpeedThreshold = 430.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Socket")
	FName LeftFootSocketName = TEXT("foot_l");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep|Socket")
	FName RightFootSocketName = TEXT("foot_r");

	FVector LastFootstepLocation = FVector::ZeroVector;
	float FootstepDistanceAccumulator = 0.f;
	bool bUseLeftFootForNextStep = true;

	void UpdateFootstepSound(float DeltaTime);
	void PlayFootstepSound(bool bRunFootstep);
	bool ShouldPlayFootstepSound() const;
	USoundBase* GetRandomFootstepSound(const TArray<USoundBase*>& Sounds) const;

	// ===== Player Attack Hit Success Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Audio|Hit")
	TArray<USoundBase*> PlayerAttackHitSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Audio|Hit")
	float PlayerAttackHitSuccessVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Audio|Hit")
	float PlayerAttackHitSuccessPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Audio|Hit")
	float PlayerAttackHitSuccessPitchMax = 1.04f;

	void PlayPlayerAttackHitSuccessSoundAtLocation(const FVector& Location);

	// ===== Player Attack Blood Effect =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Effect|Hit")
	UNiagaraSystem* PlayerAttackHitBloodNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Effect|Hit")
	UParticleSystem* PlayerAttackHitBloodParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Effect|Hit")
	float PlayerAttackHitBloodEffectScale = 1.0f;

	void SpawnPlayerAttackHitBloodEffect(const FVector& Location, const FRotator& Rotation);

	// ===== Weapon Visual =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	UStaticMesh* DefaultWeaponStaticMesh = nullptr;

	UPROPERTY()
	UNiagaraComponent* WeaponNiagaraEffectComp = nullptr;

	UPROPERTY()
	UParticleSystemComponent* WeaponParticleEffectComp = nullptr;

	UPROPERTY()
	UPointLightComponent* WeaponLightComp = nullptr;

	void ClearWeaponLight();
	void ApplyWeaponLightFromItemData(const FItemData& ItemData);
	void UpdateWeaponLightVisibility();

	void ClearWeaponEffect();

	void ApplyWeaponEffectFromItemData(const FItemData& ItemData);

	void ApplyWeaponHandDefaultTransform();
	void ApplyWeaponWaistDefaultTransform();

	UPROPERTY()
	UNiagaraComponent* WeaponAttackTrailComp = nullptr;

	UPROPERTY()
	UParticleSystemComponent* WeaponAttackTrailParticleComp = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|AttackTrail")
	EWeaponEffectType DefaultWeaponAttackTrailEffectType = EWeaponEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|AttackTrail")
	UNiagaraSystem* DefaultWeaponAttackTrailEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|AttackTrail")
	UParticleSystem* DefaultWeaponAttackTrailParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|AttackTrail")
	FName DefaultWeaponAttackTrailSocketName = TEXT("TraceStart");

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bBlockPauseMenuOpenThisFrame = false;

public:
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CloseDeathPenaltyTutorial();

	void ApplyDeathPenaltyTutorialInputMode();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool ShouldBlockPauseMenuOpen() const { return bBlockPauseMenuOpenThisFrame; }

	void BlockPauseMenuOpenForOneFrame();
	void ClearPauseMenuOpenBlock();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HandleEscapeInput(const FInputActionValue& InputValue);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void OnTutorialPopupClosedFromWidget();

	UFUNCTION(BlueprintCallable, Category = "Audio|Attack")
	void PlayWeaponSwingSound();

	UFUNCTION(BlueprintCallable, Category = "Audio|Attack")
	void PlayAttackVoiceSound();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetSwordVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UpdateSwordVisibilityByState();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DropWeaponOnDeath();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ResetWeaponPhysicsForAttach();

	UFUNCTION(BlueprintCallable, Category = "Chest")
	void BeginChestOpen(AMimicChestMonster* Chest);

	UFUNCTION(BlueprintCallable, Category = "Chest")
	void FinishChestOpen();

	UFUNCTION(BlueprintCallable, Category = "Chest")
	void TriggerChestOpenDuringMontage();

	UFUNCTION(BlueprintCallable, Category = "UI|Boss")
	void ShowBossFelledMessage(const FText& Message, float Duration);

	UFUNCTION(BlueprintCallable, Category = "UI|Boss")
	void HideBossFelledMessage();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	void RefreshEquippedWeaponVisual();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	void ApplyWeaponVisualFromItemData(const FItemData& ItemData);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	void ClearWeaponVisualToDefault();

	UFUNCTION(BlueprintCallable, Category = "Weapon|AttackTrail")
	void StartWeaponAttackTrail();

	UFUNCTION(BlueprintCallable, Category = "Weapon|AttackTrail")
	void StopWeaponAttackTrail();

public:
	UFUNCTION(BlueprintCallable, Category = "Item|Pickup")
	bool StartItemPickupMontage();

	UFUNCTION(BlueprintCallable, Category = "Item|Pickup")
	void FinishItemPickupMontage();

	UFUNCTION(BlueprintPure, Category = "Item|Pickup")
	bool IsPlayingItemPickupMontage() const { return bIsPlayingItemPickupMontage; }

protected:
	// 아이템 획득 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Pickup")
	UAnimMontage* ItemPickupMontage = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Item|Pickup")
	bool bIsPlayingItemPickupMontage = false;

	FTimerHandle TimerHandle_ItemPickupMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Death Drop")
	float DeathWeaponDropImpulseStrength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Death Drop")
	float DeathWeaponDropUpImpulse = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Death Drop")
	float DeathWeaponAngularImpulseStrength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Death Drop")
	float DeathWeaponLinearDamping = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Death Drop")
	float DeathWeaponAngularDamping = 8.f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Death Drop")
	bool bWeaponDroppedOnDeath = false;
};
