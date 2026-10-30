#pragma once

#include "CoreMinimal.h"
#include "TutorialBossMonster.h"
#include "FinalBossPhase2.generated.h"

class USoundBase;

UCLASS()
class COO_API AFinalBossPhase2 : public ATutorialBossMonster
{
	GENERATED_BODY()

public:
	AFinalBossPhase2();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// 다중 발사체
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile")
	int32 MultiProjectileCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile")
	float MultiProjectileAngleStep = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile")
	float MultiProjectileSpawnHeightOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile")
	TArray<FName> MultiProjectileSocketNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile|Multi|Audio")
	TArray<USoundBase*> MultiRangedNiagaraStartSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile|Multi|Audio")
	TArray<USoundBase*> MultiRangedNiagaraFireSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile|Multi|Audio")
	float MultiRangedNiagaraSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile")
	bool bUseSocketCountAsProjectileCount = true;

	// 순간이동
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport")
	float TeleportDistance = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport")
	float TeleportIfCloseDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport")
	float TeleportCooldown = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport")
	float AutoTeleportInterval = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport")
	UNiagaraSystem* TeleportStartEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport")
	UNiagaraSystem* TeleportEndEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport")
	bool bUseRandomSideTeleportOnAuto = true;

	UPROPERTY(BlueprintReadOnly, Category = "Phase2|Teleport")
	bool bCanTeleport = true;

	// ===== Teleport Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport|Audio")
	TArray<USoundBase*> TeleportStartSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport|Audio")
	TArray<USoundBase*> TeleportEndSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport|Audio")
	float TeleportSoundVolume = 1.0f;

	FTimerHandle TimerHandle_AutoTeleport;
	FTimerHandle TimerHandle_TeleportCooldown;

protected:
	void TryTeleportAwayFromPlayer();
	void AutoTeleport();
	void ResetTeleportCooldown();

	bool CanPhase2Teleport() const;

	bool FindTeleportLocation(const FVector& DesiredLocation, FVector& OutLocation) const;
	void DoTeleportTo(const FVector& NewLocation);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile|Multi")
	TArray<UAnimMontage*> MultiRangedNiagaraMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile|Multi")
	float MultiRangedNiagaraInterval = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile|Multi")
	UNiagaraSystem* MultiRangedNiagaraMuzzleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Projectile|Multi")
	TSubclassOf<class ABossNiagaraProjectile> MultiRangedProjectileClass;

	FTimerHandle TimerHandle_MultiRangedNiagaraLoop;

	void TryStartMultiRangedNiagaraAttack();

public:
	UFUNCTION(BlueprintCallable, Category = "Phase2|Projectile")
	void FireMultiRangedProjectiles();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|AI")
	float MeleeAttackRange = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|AI")
	float RangedAttackMinRange = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|AI")
	float RangedAttackMaxRange = 9999.f;

	UFUNCTION(BlueprintPure, Category = "Phase2|AI")
	bool IsTargetInCloseRange() const;

	UFUNCTION(BlueprintPure, Category = "Phase2|AI")
	bool IsTargetInRangedRange() const;

protected:
	virtual void TryStartNormalCombo() override;
	virtual void TryStartPowerAttack() override;
	virtual void TryStartAreaNiagaraAttack() override;
	virtual void TryStartRangedNiagaraAttack() override;
	virtual void TryStartGrabAttack() override;

	UPROPERTY(BlueprintReadOnly, Category = "Phase2|Attack")
	bool bNormalComboPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "Phase2|Attack")
	bool bAreaAttackPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "Phase2|Attack")
	bool bRangedAttackPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "Phase2|Attack")
	bool bMultiRangedAttackPending = false;

	UPROPERTY(BlueprintReadOnly, Category = "Phase2|Attack")
	bool bGrabAttackPending = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|Teleport")
	float EngageTeleportDistanceFromPlayer = 180.f;

	bool HasPendingCloseRangeAttack() const;
	bool HasPendingRangedAttack() const;
	bool TryFindEngageTeleportLocation(FVector& OutLocation) const;
	bool TryFindFarTeleportLocation(FVector& OutLocation) const;

	void ClearCloseRangePendings();
	void ClearRangedPendings();

	void MoveTowardTargetForMelee();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase2|AI")
	float MeleeMoveAcceptanceRadius = 160.f;
};