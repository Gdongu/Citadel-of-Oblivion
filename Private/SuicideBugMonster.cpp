#include "SuicideBugMonster.h"
#include "MonsterAIController.h"
#include "Knight.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"

ASuicideBugMonster::ASuicideBugMonster()
{
	ExplosionDelay = 3.0f;
	ExplosionRadius = 250.0f;
	ExplosionDamage = 20.0f;
	bHasExploded = false;
}

void ASuicideBugMonster::BeginPlay()
{
	Super::BeginPlay();
}

void ASuicideBugMonster::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bIsAttacking = false;
	bAttackTraceActive = false;
	TargetActor = nullptr;
	bHasExploded = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_HideHPBar);

	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}

	if (LockOnWidgetComp)
	{
		LockOnWidgetComp->SetVisibility(false);
	}

	GetCharacterMovement()->DisableMovement();

	if (AggroSphere)
	{
		AggroSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
	{
		MonsterAI->StopMovement();
		MonsterAI->SetTargetActor(nullptr);
		MonsterAI->SetIsDead(true);
		MonsterAI->SetIsAttacking(false);
		MonsterAI->SetIsKnockedDown(false);
	}

	TrySpawnRandomDrop();
	PlayDeathMontage();
	OnDied.Broadcast();

	// 3ÃÊ µÚ Æø¹ß
	GetWorldTimerManager().SetTimer(
		TimerHandle_Explosion,
		this,
		&ASuicideBugMonster::Explode,
		ExplosionDelay,
		false
	);
}

void ASuicideBugMonster::Explode()
{
	if (bHasExploded)
	{
		return;
	}

	bHasExploded = true;

	PlayExplosionSound();

	ApplyExplosionDamage();

	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	if (bRespawnOnBonfireRest)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetVisibility(false, true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		Destroy();
	}
}

void ASuicideBugMonster::ApplyExplosionDamage()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, AKnight::StaticClass(), FoundActors);

	TSubclassOf<UDamageType> DamageTypeToUse = ExplosionDamageTypeClass;
	if (!DamageTypeToUse)
	{
		DamageTypeToUse = UDamageType::StaticClass();
	}

	for (AActor* Actor : FoundActors)
	{
		AKnight* Knight = Cast<AKnight>(Actor);
		if (!Knight)
		{
			continue;
		}

		const float Dist = FVector::Dist(GetActorLocation(), Knight->GetActorLocation());
		if (Dist > ExplosionRadius)
		{
			continue;
		}

		UGameplayStatics::ApplyDamage(
			Knight,
			ExplosionDamage,
			GetController(),
			this,
			DamageTypeToUse
		);
	}
}

void ASuicideBugMonster::RespawnMonster()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Explosion);
	bHasExploded = false;

	Super::RespawnMonster();
}

USoundBase* ASuicideBugMonster::GetRandomExplosionSound() const
{
	TArray<USoundBase*> ValidSounds;

	for (USoundBase* Sound : ExplosionSounds)
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

void ASuicideBugMonster::PlayExplosionSound()
{
	USoundBase* Sound = GetRandomExplosionSound();
	if (!Sound)
	{
		return;
	}

	const float Pitch = FMath::FRandRange(
		ExplosionSoundPitchMin,
		ExplosionSoundPitchMax
	);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		ExplosionSoundVolume,
		Pitch
	);
}