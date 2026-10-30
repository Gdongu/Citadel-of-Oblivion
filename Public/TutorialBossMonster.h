#pragma once

#include "CoreMinimal.h"
#include "BaseMonster.h"
#include "ItemTypes.h"
#include "TutorialBossMonster.generated.h"

class UChildActorComponent;
class AKnight;
class USoundBase;

UENUM(BlueprintType)
enum class EBossAttackState : uint8
{
	Idle,
	NormalCombo,
	PowerAttack,
	AreaNiagara,
	RangedNiagara,
	Groggy,
	Grab
};

USTRUCT(BlueprintType)
struct FBossGuaranteedRewardEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Reward")
	FItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Reward")
	int32 Count = 1;
};

UCLASS()
class COO_API ATutorialBossMonster : public ABaseMonster
{
	GENERATED_BODY()

public:
	ATutorialBossMonster();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

	virtual bool ShouldUseBossHUD() const override { return true; }

	UFUNCTION(BlueprintPure, Category = "Boss|Stat")
	float GetCurrentGroggyGauge() const { return CurrentGroggyGauge; }

	UFUNCTION(BlueprintPure, Category = "Boss|Stat")
	float GetMaxGroggyGauge() const { return MaxGroggyGauge; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	FName GetBossUniqueID() const { return BossUniqueID; }

	virtual bool CanAttack() const override;
	virtual void PerformAttack() override;
	virtual void ResetAttackState() override;
	virtual bool CanBeBackAttacked() const override;

protected:
	virtual void PerformAttackTrace() override;
	virtual void PlayHitReaction_Implementation() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Stat")
	float MaxGroggyGauge = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Stat")
	float CurrentGroggyGauge = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Stat")
	float GroggyDuration = 5.f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsGroggy = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	EBossAttackState BossAttackState = EBossAttackState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Normal")
	TArray<UAnimMontage*> NormalComboMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Normal")
	float NormalAttackInterval = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Normal")
	float NextComboDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Normal")
	float ComboContinueRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Power")
	TArray<UAnimMontage*> PowerAttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Power")
	float PowerAttackInterval = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Power")
	float PowerAttackDamageMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Animation")
	UAnimMontage* GroggyMontageAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Animation")
	UAnimMontage* GroggyExitMontageAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|AI")
	float PowerAttackRangeMultiplier = 1.5f;

	FTimerHandle TimerHandle_GroggyEnd;
	FTimerHandle TimerHandle_NormalAttackLoop;
	FTimerHandle TimerHandle_PowerAttackLoop;
	FTimerHandle TimerHandle_NextCombo;
	FTimerHandle TimerHandle_CurrentAttackEnd;

	int32 CurrentComboStep = 0;

protected:
	void ConsumeGroggyGauge(float DamageAmount);
	void EnterGroggy();
	void ExitGroggy();

	virtual void TryStartNormalCombo();
	void StartNextNormalComboStep();
	void FinishNormalCombo();

	virtual void TryStartPowerAttack();
	void FinishPowerAttack();

	void PauseAttackLoops();
	void ResumeAttackLoops();
	bool IsBusy() const;
	float GetMontageLength(UAnimMontage* Montage) const;
	void FaceTarget();

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bPowerAttackPending = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Reward")
	TSubclassOf<class AItemDropActor> BossRewardDropClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Reward")
	TArray<FBossGuaranteedRewardEntry> GuaranteedRewards;

	void SpawnGuaranteedBossReward();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	FName BossUniqueID = NAME_None;

	bool IsAlreadyDefeated() const;
	void MarkBossAsDefeated();

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsExitingGroggy = false;

	FTimerHandle TimerHandle_GroggyExitEnd;

	void FinishGroggyExit();

public:
	virtual void Die() override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Groggy")
	void FinishGroggyFromBackAttack();

	UFUNCTION(BlueprintCallable, Category = "Boss|Attack|Grab")
	void StartGrabCheck();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	TArray<UAnimMontage*> GrabAttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	TArray<UAnimMontage*> GrabSuccessMontages;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Attack|Grab")
	bool bGrabSucceeded = false;

	FTimerHandle TimerHandle_GrabSuccessEnd;

	void PlayGrabSuccessMontage();
	void FinishGrabSuccessAttack();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	float GrabAttackInterval = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	float GrabAttackRange = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	float GrabDamagePerTick = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	float GrabDamageTickInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	float GrabDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	FName GrabSocketName = TEXT("AttackTraceStart");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	FVector GrabbedPlayerRelativeLocation = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	FRotator GrabbedPlayerRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Attack|Grab")
	bool bIsGrabAttacking = false;

	UPROPERTY()
	AKnight* GrabbedKnight = nullptr;

	FTimerHandle TimerHandle_GrabAttackLoop;
	FTimerHandle TimerHandle_GrabDamageTick;
	FTimerHandle TimerHandle_GrabEnd;

	virtual void TryStartGrabAttack();
	void FinishGrabAttack();

	UFUNCTION(BlueprintCallable, Category = "Boss|Attack|Grab")
	void TryGrabPlayer();

	void AttachKnightToHand(AKnight* Knight);
	void ReleaseGrabbedKnight(bool bKnockDownPlayer);
	void ApplyGrabDamageTick();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	float GrabCameraBlendTime = 0.2f;

	void SwitchToGrabCamera(AKnight* Knight);
	void RestoreGrabbedPlayerCamera(AKnight* Knight);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	float GrabCheckRadius = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	FName GrabCheckSocketName = TEXT("AttackTraceStart");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Grab")
	UChildActorComponent* GrabCameraActorComp = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	TArray<UAnimMontage*> AreaNiagaraMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	TArray<UAnimMontage*> RangedNiagaraMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	float AreaNiagaraInterval = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	float RangedNiagaraInterval = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	float AreaNiagaraRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	float AreaNiagaraDamage = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	float RangedNiagaraDamage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	FName AreaNiagaraSpawnSocket = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	FName RangedNiagaraSpawnSocket = TEXT("AttackTraceStart");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	class UNiagaraSystem* AreaNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	float AreaNiagaraDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	float AreaNiagaraDamageTickInterval = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	FVector ActiveAreaNiagaraLocation = FVector::ZeroVector;

	FTimerHandle TimerHandle_AreaNiagaraDamageTick;
	FTimerHandle TimerHandle_AreaNiagaraDamageEnd;

	UFUNCTION()
	void ApplyAreaNiagaraDamageTick();

	UFUNCTION()
	void EndAreaNiagaraDamage();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	class UNiagaraSystem* RangedNiagaraMuzzleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Attack|Niagara")
	TSubclassOf<class ABossNiagaraProjectile> RangedProjectileClass;

	FTimerHandle TimerHandle_AreaNiagaraLoop;
	FTimerHandle TimerHandle_RangedNiagaraLoop;
	FTimerHandle TimerHandle_SpecialAttackEnd;

	virtual void TryStartAreaNiagaraAttack();
	virtual void TryStartRangedNiagaraAttack();

	// ===== Boss Attack Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Power")
	TArray<USoundBase*> PowerAttackStartSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Power")
	float PowerAttackSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Area")
	TArray<USoundBase*> AreaNiagaraStartSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Area")
	TArray<USoundBase*> AreaNiagaraActivateSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Area")
	float AreaNiagaraSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Ranged")
	TArray<USoundBase*> RangedNiagaraStartSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Ranged")
	TArray<USoundBase*> RangedNiagaraFireSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Ranged")
	float RangedNiagaraSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Grab")
	TArray<USoundBase*> GrabAttackStartSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Grab")
	TArray<USoundBase*> GrabSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Grab")
	float GrabSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio")
	float BossAttackSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio")
	float BossAttackSoundPitchMax = 1.04f;

	// ===== Boss Hit Success Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Hit|Normal")
	TArray<USoundBase*> NormalAttackHitSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Hit|Power")
	TArray<USoundBase*> PowerAttackHitSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Hit|Area")
	TArray<USoundBase*> AreaNiagaraHitSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Hit|Ranged")
	TArray<USoundBase*> RangedNiagaraHitSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Hit|Grab")
	TArray<USoundBase*> GrabAttackHitSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Audio|Hit")
	float BossHitSuccessVolume = 1.0f;

	void PlayBossHitSuccessSoundAtLocation(
		const TArray<USoundBase*>& Sounds,
		const FVector& Location,
		float Volume
	);

	USoundBase* GetRandomBossSound(const TArray<USoundBase*>& Sounds) const;

	void PlayBossSoundAtActor(const TArray<USoundBase*>& Sounds, float Volume);
	void PlayBossSoundAtSocket(const TArray<USoundBase*>& Sounds, FName SocketName, float Volume);
	void PlayBossSoundAtLocation(const TArray<USoundBase*>& Sounds, const FVector& Location, float Volume);

public:
	UFUNCTION(BlueprintCallable, Category = "Boss|Attack|Niagara")
	void ActivateAreaNiagaraAttack();

	UFUNCTION(BlueprintCallable, Category = "Boss|Attack|Niagara")
	void FireRangedNiagaraProjectile();

	void FinishSpecialAttack();

	UFUNCTION(BlueprintCallable, Category = "Boss|Intro")
	virtual void StartBossAfterIntroMovie();

	virtual bool IsWaitingForIntroMovie() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Intro")
	bool bWaitForIntroMovie = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Intro")
	bool bBossIntroFinished = false;

	void StartBossAttackLoops();
};