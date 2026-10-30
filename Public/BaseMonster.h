#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.h"
#include "GameFramework/Character.h"
#include "BaseMonster.generated.h"

class UPrimitiveComponent;
class UBehaviorTree;
class USoundBase;
class UAudioComponent;
class AStageBGMManager;
class UNiagaraSystem;
class UParticleSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMonsterHPChanged, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMonsterDied);

USTRUCT(BlueprintType)
struct FMonsterRandomDropEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Drop")
	FItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Drop")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Monster|Drop")
	float Chance = 0.3f;
};

UCLASS()
class COO_API ABaseMonster : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseMonster();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Component")
	class USphereComponent* AggroSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Component")
	class UWidgetComponent* HPBarWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stat")
	float MaxHP = 10.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stat")
	float CurrentHP = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stat")
	float AttackPower = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stat")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stat")
	float AggroRange = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stat")
	float MoveSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stat")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	float AcceptanceRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	float SightAngleDegrees = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	float SightForwardYawOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	float HomeYawOffset = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Monster|AI")
	bool CanSeeTargetInFront(AActor* OtherActor) const;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|State")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|State")
	bool bIsAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|State")
	AActor* TargetActor = nullptr;

	FTimerHandle TimerHandle_AttackCooldown;
	FTimerHandle TimerHandle_DestroyAfterDeath;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|State")
	bool bIsHitReactionLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Hit")
	float HitReactionAttackBlockFallbackTime = 0.35f;

	FTimerHandle TimerHandle_HitReactionAttackBlock;

	void CancelAttackForPlayerHit();
	void StartHitReactionAttackBlock();
	void EndHitReactionAttackBlock();

	UPROPERTY(BlueprintReadOnly, Category = "Monster|AI")
	bool bHasDetectedTarget = false;

	void CheckInitialBossHUDOverlap();

public:
	UPROPERTY(BlueprintAssignable, Category = "Monster|Event")
	FOnMonsterHPChanged OnHPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Monster|Event")
	FOnMonsterDied OnDied;

	UFUNCTION(BlueprintPure, Category = "Monster|Stat")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category = "Monster|Stat")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category = "Monster|State")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Monster|State")
	AActor* GetTargetActor() const { return TargetActor; }

	UFUNCTION(BlueprintPure, Category = "Monster|Intro")
	virtual bool IsWaitingForIntroMovie() const { return false; }

	UFUNCTION(BlueprintCallable, Category = "Monster|Target")
	void SetTargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Monster|Attack")
	virtual bool CanAttack() const;

	UFUNCTION(BlueprintCallable, Category = "Monster|Attack")
	virtual void PerformAttack();

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	virtual void ResetAttackState();

	UFUNCTION(BlueprintCallable, Category = "Monster|UI")
	void UpdateHPWidget();

	UFUNCTION(BlueprintPure, Category = "Monster|UI")
	FText GetMonsterDisplayName() const { return MonsterDisplayName; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|UI")
	FText MonsterDisplayName = FText::FromString(TEXT("Monster"));

	UFUNCTION(BlueprintCallable, Category = "Monster|Respawn")
	virtual void ResetToInitialStateOnBonfireRest();

	UFUNCTION(BlueprintCallable, Category = "Monster|LockOn")
	virtual bool CanBeLockedOn() const;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Monster|Animation")
	void PlayAttackMontage();
	virtual void PlayAttackMontage_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Monster|Animation")
	void PlayHitReaction();
	virtual void PlayHitReaction_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Monster|Animation")
	void PlayDeathMontage();
	virtual void PlayDeathMontage_Implementation();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Monster|Attack")
	bool bAttackTraceActive = false;

	UPROPERTY()
	TArray<AActor*> AlreadyHitActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Attack")
	float AttackTraceRadius = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Attack")
	FName AttackTraceStartSocketName = TEXT("AttackTraceStart");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Attack")
	FName AttackTraceEndSocketName = TEXT("AttackTraceEnd");

public:
	UFUNCTION(BlueprintCallable, Category = "Monster|Attack")
	void StartAttackTrace();

	UFUNCTION(BlueprintCallable, Category = "Monster|Attack")
	void EndAttackTrace();

	UFUNCTION(BlueprintCallable, Category = "Monster|AI")
	void SetHasDetectedTarget(bool bNewDetected) { bHasDetectedTarget = bNewDetected; }

	UFUNCTION(BlueprintPure, Category = "Monster|State")
	bool IsKnockedDown() const { return bIsKnockedDown; }

protected:
	virtual void PerformAttackTrace();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation")
	UAnimMontage* AttackMontageAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation")
	TArray<UAnimMontage*> AttackMontageVariants;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation")
	UAnimMontage* HitReactionMontageAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation")
	UAnimMontage* DeathMontageAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio|Death")
	TArray<USoundBase*> DeathSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio|Death")
	float DeathSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio|Death")
	float DeathSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio|Death")
	float DeathSoundPitchMax = 1.04f;

	USoundBase* GetRandomDeathSound() const;
	void PlayDeathSound();

	// ===== Attack Hit Success Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio|Hit")
	TArray<USoundBase*> AttackHitSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio|Hit")
	float AttackHitSuccessVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio|Hit")
	float AttackHitSuccessPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio|Hit")
	float AttackHitSuccessPitchMax = 1.04f;

	void PlayAttackHitSuccessSoundAtLocation(const FVector& Location);

	// ===== Hit Blood Effect =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Effect|Hit")
	UNiagaraSystem* AttackHitBloodNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Effect|Hit")
	UParticleSystem* AttackHitBloodParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Effect|Hit")
	float AttackHitBloodEffectScale = 1.0f;

	void SpawnAttackHitBloodEffect(const FVector& Location, const FRotator& Rotation);

public:
	UFUNCTION(BlueprintPure, Category = "Monster|AI")
	UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|UI")
	float HPBarVisibleTime = 10.0f;

	FTimerHandle TimerHandle_HideHPBar;

	UFUNCTION()
	void OnAggroSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnAggroSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	virtual bool ShouldUseBossHUD() const { return false; }

	void ShowHPBar();
	void HideHPBar();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Component")
	class UWidgetComponent* LockOnWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|UI")
	FName LockOnAttachBoneName = TEXT("spine_03");

public:
	UFUNCTION(BlueprintCallable, Category = "Monster|LockOn")
	void SetLockOnVisible(bool bVisible);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Monster|State")
	bool bIsKnockedDown = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|State")
	float KnockDownDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Animation")
	UAnimMontage* KnockDownMontageAsset;

	virtual void PlayKnockDownMontage_Implementation();

	FTimerHandle TimerHandle_KnockDown;

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Monster|Animation")
	void PlayKnockDownMontage();

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	void EnterKnockDown(float Duration);

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	void EndKnockDown();

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	void FreezeForBackAttack();

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	void UnfreezeFromBackAttack();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Monster|State")
	bool bInBackAttackReaction = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|BackAttack")
	class UBoxComponent* BackAttackTrigger;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|State")
	bool bIsFrozenForBackAttack = false;

	UFUNCTION()
	void OnBackAttackTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnBackAttackTriggerEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Reward")
	int32 MemoryShardReward = 10;

	UPROPERTY()
	TWeakObjectPtr<class AKnight> LastDamagingKnight;

public:
	UFUNCTION(BlueprintPure, Category = "Monster|BackAttack")
	virtual bool CanBeBackAttacked() const;

	UFUNCTION(BlueprintCallable, Category = "Monster|State")
	void SetBackAttackReaction(bool bEnable);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Drop")
	TSubclassOf<class AItemDropActor> ItemDropActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Drop")
	TArray<FMonsterRandomDropEntry> RandomDropList;

	void TrySpawnRandomDrop();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Respawn")
	bool bRespawnOnBonfireRest = true;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Respawn")
	FTransform InitialSpawnTransform = FTransform::Identity;

public:
	UFUNCTION(BlueprintCallable, Category = "Monster|Respawn")
	virtual void RespawnMonster();

	UFUNCTION(BlueprintPure, Category = "Monster|Respawn")
	bool ShouldRespawnOnBonfireRest() const { return bRespawnOnBonfireRest; }

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Monster|Patrol")
	bool bUsePatrol = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Monster|Patrol")
	TArray<AActor*> PatrolPoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Patrol")
	float PatrolAcceptanceRadius = 80.f;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Patrol")
	int32 CurrentPatrolIndex = 0;

public:
	UFUNCTION(BlueprintCallable, Category = "Monster|Patrol")
	bool ShouldUsePatrol() const { return bUsePatrol && PatrolPoints.Num() > 0; }

	UFUNCTION(BlueprintCallable, Category = "Monster|Patrol")
	AActor* GetCurrentPatrolPoint() const;

	UFUNCTION(BlueprintCallable, Category = "Monster|Patrol")
	void AdvancePatrolPoint();

	UFUNCTION(BlueprintCallable, Category = "Monster|Patrol")
	float GetPatrolAcceptanceRadius() const { return PatrolAcceptanceRadius; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio")
	USoundBase* BossBGMSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio")
	float BossBGMVolume = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio")
	float BossBGMFadeInTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio")
	float BossBGMFadeOutTime = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Audio")
	bool bBossBGMActive = false;

	UPROPERTY()
	UAudioComponent* BossBGMComponent = nullptr;

	void StartBossBGM();

public:
	UFUNCTION(BlueprintCallable, Category = "Boss|Audio")
	void StopBossBGM();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	TArray<USoundBase*> WeaponSwingSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	TArray<USoundBase*> AttackVoiceSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	float AttackSFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	float VoiceVolume = 1.0f;

	USoundBase* GetRandomMonsterSound(const TArray<USoundBase*>& Sounds) const;

	// ===== Boss Felled UI =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Felled UI")
	bool bShowBossFelledMessageOnDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Felled UI")
	FText BossFelledMessage = FText::FromString(TEXT("ENEMY FELLED"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Felled UI")
	float BossFelledMessageDuration = 4.0f;

public:
	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	void PlayWeaponSwingSound();

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	void PlayAttackVoiceSound();

	UFUNCTION(BlueprintPure, Category = "Monster|AI")
	bool IsBossMonsterForAI() const { return ShouldUseBossHUD(); }

	UFUNCTION(BlueprintCallable, Category = "Monster|AI")
	bool TryAcquireTargetFromAI(AActor* OtherActor);

protected:
	// ===== Return Home / Re-Aggro Control =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	float LoseTargetDistance = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	float ReturnHomeAcceptanceRadius = 80.f;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	bool bReturningHome = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|AI")
	bool bPlayerInsideAggroSphere = false;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	FTransform ActiveReturnHomeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	float ReturnHomeRotationLockTime = 0.25f;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	bool bReturnHomeRotationLockActive = false;

	FTimerHandle TimerHandle_ReturnHomeFinalize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	bool bIgnoreFrontSightAfterReturnHome = true;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	bool bCanReacquireWithoutFrontSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|AI|ReturnHome")
	float ReturnHomeSnapRadius = 250.f;

	void ApplyReturnHomeTransformLocked();
	void FinishReturnHomeIdleState();

	void LoseTargetAndReturnHome();
	void MoveBackToReturnHome();
	void UpdateReturnHome();

	bool CanAcquireTarget(AActor* OtherActor) const;
	void TryAcquireTarget(AActor* OtherActor);
	virtual bool CanAutoAcquireTarget() const { return true; }

	void ClearCombatTargetForReturnHome();
	void RestoreHomeIdleState();

	bool IsActorInsideAggroRange(AActor* OtherActor) const;
	void TryAcquireCurrentPlayer();
	FTransform BuildReturnHomeTransform();

	void FaceActorImmediately(AActor* OtherActor);
	bool TryAcquireTargetAfterReturnHome(AActor* OtherActor);
};