#pragma once

#include "CoreMinimal.h"
#include "BaseMonster.h"
#include "SuicideBugMonster.generated.h"

class UNiagaraSystem;
class USphereComponent;
class USoundBase;

UCLASS()
class COO_API ASuicideBugMonster : public ABaseMonster
{
	GENERATED_BODY()

public:
	ASuicideBugMonster();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Die() override;
	virtual void RespawnMonster() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SuicideBug|Explosion")
	float ExplosionDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SuicideBug|Explosion")
	float ExplosionRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SuicideBug|Explosion")
	float ExplosionDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SuicideBug|Explosion")
	TSubclassOf<UDamageType> ExplosionDamageTypeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SuicideBug|Explosion")
	UNiagaraSystem* ExplosionEffect = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "SuicideBug|Explosion")
	bool bHasExploded = false;

	FTimerHandle TimerHandle_Explosion;

	UFUNCTION()
	void Explode();

	void ApplyExplosionDamage();

	// ===== Explosion Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion|Audio")
	TArray<USoundBase*> ExplosionSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion|Audio")
	float ExplosionSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion|Audio")
	float ExplosionSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion|Audio")
	float ExplosionSoundPitchMax = 1.04f;

	USoundBase* GetRandomExplosionSound() const;
	void PlayExplosionSound();
};