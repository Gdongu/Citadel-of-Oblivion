#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossNiagaraProjectile.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystem;

UCLASS()
class COO_API ABossNiagaraProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABossNiagaraProjectile();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* CollisionComp;

	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* NiagaraComp;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMoveComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	float Damage = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	float LifeSeconds = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	float HomingAcceleration = 6000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	TWeakObjectPtr<AActor> HomingTargetActor;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	bool bHasHit = false;

	// 맞은 뒤 Projectile 본체가 사라지기까지의 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Hit")
	float HitDestroyDelay = 0.1f;

	// 맞은 순간 나올 Niagara 피격 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Hit")
	UNiagaraSystem* HitNiagaraEffect = nullptr;

	// 맞은 순간 나올 Cascade Particle 피격 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Hit")
	UParticleSystem* HitParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Hit")
	float HitEffectScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Hit")
	bool bAllowDodgeDuringPlayerHitReaction = false;

	UFUNCTION()
	void OnProjectileBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void HandleProjectileHit(AActor* HitActor, const FHitResult& SweepResult);
	void SpawnHitEffect(const FHitResult& SweepResult);
	void DisableProjectileAfterHit();

	FTimerHandle TimerHandle_DestroyAfterHit;
	void DestroyAfterHitDelay();

public:
	void SetHomingTarget(AActor* NewTarget);
	void SetProjectileDamage(float NewDamage) { Damage = NewDamage; }

	void SetAllowDodgeDuringPlayerHitReaction(bool bNewAllow)
	{
		bAllowDodgeDuringPlayerHitReaction = bNewAllow;
	}

	bool ShouldAllowDodgeDuringPlayerHitReaction() const
	{
		return bAllowDodgeDuringPlayerHitReaction;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Audio")
	TArray<USoundBase*> HitSuccessSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Audio")
	float HitSuccessSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Audio")
	float HitSuccessPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Audio")
	float HitSuccessPitchMax = 1.04f;

	UFUNCTION(BlueprintCallable, Category = "Projectile|Audio")
	void SetHitSuccessSounds(
		const TArray<USoundBase*>& InSounds,
		float InVolume,
		float InPitchMin,
		float InPitchMax
	);

	void PlayHitSuccessSoundAtLocation(const FVector& Location);
};