#include "BossNiagaraProjectile.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

ABossNiagaraProjectile::ABossNiagaraProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->SetupAttachment(RootComponent);
	CollisionComp->SetSphereRadius(24.f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	NiagaraComp->SetupAttachment(RootComponent);

	ProjectileMoveComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMoveComp"));
	ProjectileMoveComp->InitialSpeed = 900.f;
	ProjectileMoveComp->MaxSpeed = 900.f;
	ProjectileMoveComp->ProjectileGravityScale = 0.f;
}

void ABossNiagaraProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComp)
	{
		CollisionComp->OnComponentBeginOverlap.AddDynamic(
			this,
			&ABossNiagaraProjectile::OnProjectileBeginOverlap
		);
	}

	SetLifeSpan(LifeSeconds);
}

void ABossNiagaraProjectile::OnProjectileBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (bHasHit)
	{
		return;
	}

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (!OtherActor->ActorHasTag(TEXT("Player")))
	{
		return;
	}

	HandleProjectileHit(OtherActor, SweepResult);
}

void ABossNiagaraProjectile::HandleProjectileHit(AActor* HitActor, const FHitResult& SweepResult)
{
	if (bHasHit || !HitActor)
	{
		return;
	}

	bHasHit = true;

	// 데미지는 맞는 순간 바로 적용
	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		HitActor,
		Damage,
		GetInstigatorController(),
		this,
		UDamageType::StaticClass()
	);

	if (AppliedDamage > 0.f)
	{
		PlayHitSuccessSoundAtLocation(HitActor->GetActorLocation());
	}

	// 맞은 위치에 피격 이펙트 생성
	SpawnHitEffect(SweepResult);

	// 더 이상 충돌/이동하지 않게 처리
	DisableProjectileAfterHit();

	// Projectile Niagara는 바로 없애지 않고 잠깐 남긴 뒤 삭제
	GetWorldTimerManager().ClearTimer(TimerHandle_DestroyAfterHit);
	GetWorldTimerManager().SetTimer(
		TimerHandle_DestroyAfterHit,
		this,
		&ABossNiagaraProjectile::DestroyAfterHitDelay,
		HitDestroyDelay,
		false
	);
}

void ABossNiagaraProjectile::SpawnHitEffect(const FHitResult& SweepResult)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector EffectLocation = GetActorLocation();

	if (!SweepResult.ImpactPoint.IsNearlyZero())
	{
		EffectLocation = SweepResult.ImpactPoint;
	}
	else if (!SweepResult.Location.IsNearlyZero())
	{
		EffectLocation = SweepResult.Location;
	}

	FRotator EffectRotation = GetActorRotation();

	if (ProjectileMoveComp && !ProjectileMoveComp->Velocity.IsNearlyZero())
	{
		EffectRotation = ProjectileMoveComp->Velocity.Rotation();
	}

	const FVector EffectScale(HitEffectScale);

	if (HitNiagaraEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			HitNiagaraEffect,
			EffectLocation,
			EffectRotation,
			EffectScale
		);
	}

	if (HitParticleEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			HitParticleEffect,
			EffectLocation,
			EffectRotation,
			EffectScale,
			true
		);
	}
}

void ABossNiagaraProjectile::DisableProjectileAfterHit()
{
	if (CollisionComp)
	{
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	if (ProjectileMoveComp)
	{
		ProjectileMoveComp->StopMovementImmediately();
		ProjectileMoveComp->Velocity = FVector::ZeroVector;
		ProjectileMoveComp->bIsHomingProjectile = false;
		ProjectileMoveComp->HomingTargetComponent = nullptr;
		ProjectileMoveComp->Deactivate();
	}
}

void ABossNiagaraProjectile::DestroyAfterHitDelay()
{
	Destroy();
}

void ABossNiagaraProjectile::SetHomingTarget(AActor* NewTarget)
{
	HomingTargetActor = NewTarget;

	if (!ProjectileMoveComp)
	{
		return;
	}

	if (NewTarget && NewTarget->GetRootComponent())
	{
		ProjectileMoveComp->bIsHomingProjectile = true;
		ProjectileMoveComp->HomingTargetComponent = NewTarget->GetRootComponent();
		ProjectileMoveComp->HomingAccelerationMagnitude = HomingAcceleration;
	}
	else
	{
		ProjectileMoveComp->bIsHomingProjectile = false;
		ProjectileMoveComp->HomingTargetComponent = nullptr;
	}
}

void ABossNiagaraProjectile::SetHitSuccessSounds(
	const TArray<USoundBase*>& InSounds,
	float InVolume,
	float InPitchMin,
	float InPitchMax
)
{
	HitSuccessSounds = InSounds;
	HitSuccessSoundVolume = InVolume;
	HitSuccessPitchMin = InPitchMin;
	HitSuccessPitchMax = InPitchMax;
}

void ABossNiagaraProjectile::PlayHitSuccessSoundAtLocation(const FVector& Location)
{
	if (HitSuccessSounds.Num() <= 0)
	{
		return;
	}

	TArray<USoundBase*> ValidSounds;
	for (USoundBase* Sound : HitSuccessSounds)
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
	const float Pitch = FMath::RandRange(HitSuccessPitchMin, HitSuccessPitchMax);

	UGameplayStatics::PlaySoundAtLocation(
		this,
		ValidSounds[RandIndex],
		Location,
		HitSuccessSoundVolume,
		Pitch
	);
}