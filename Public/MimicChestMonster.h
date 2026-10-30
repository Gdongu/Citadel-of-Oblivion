#pragma once

#include "CoreMinimal.h"
#include "BaseMonster.h"
#include "InteractInterface.h"
#include "ItemTypes.h"
#include "MimicChestMonster.generated.h"

class USphereComponent;
class UNiagaraSystem;
class AItemDropActor;
class USoundBase;
class UArrowComponent;

UENUM(BlueprintType)
enum class EChestType : uint8
{
	NormalChest UMETA(DisplayName = "Normal Chest"),
	Mimic UMETA(DisplayName = "Mimic")
};

UCLASS()
class COO_API AMimicChestMonster : public ABaseMonster, public IInteractInterface
{
	GENERATED_BODY()

public:
	AMimicChestMonster();

protected:
	virtual void BeginPlay() override;

public:
	// InteractInterface
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

	// 상자 상태일 땐 공격 못하게
	virtual bool CanAttack() const override;
	virtual void PerformAttack() override;

	virtual bool CanBeBackAttacked() const override;
	virtual bool CanBeLockedOn() const override;

	void MarkChestAsConsumed();
	bool IsAlreadyConsumed() const;
	virtual void Die() override;
	virtual void ResetToInitialStateOnBonfireRest() override;

protected:
	// ===== Chest Common =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest")
	EChestType ChestType = EChestType::NormalChest;

	UPROPERTY(BlueprintReadOnly, Category = "Chest")
	bool bOpened = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chest")
	bool bAwakenedAsMimic = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest")
	float OpenResultDelay = 1.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest")
	USphereComponent* InteractSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest")
	UArrowComponent* ChestOpenPoint;

	UPROPERTY()
	AActor* CachedInteractor = nullptr;

	FTimerHandle TimerHandle_OpenResolve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest")
	FName ChestUniqueID = NAME_None;

	// ===== Mesh / Anim =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Visual")
	USkeletalMesh* ClosedChestMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Visual")
	USkeletalMesh* MimicSkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Visual")
	TSubclassOf<UAnimInstance> ClosedChestAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Visual")
	TSubclassOf<UAnimInstance> MimicAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Animation")
	UAnimMontage* ChestOpenMontage = nullptr;

	// ===== Chest Audio =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Audio")
	TArray<USoundBase*> ChestOpenSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Audio")
	float ChestOpenSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Audio")
	float ChestOpenSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Audio")
	float ChestOpenSoundPitchMax = 1.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Audio")
	TArray<USoundBase*> MimicTransformSounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Audio")
	float MimicTransformSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Audio")
	float MimicTransformSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Audio")
	float MimicTransformSoundPitchMax = 1.04f;

	USoundBase* GetRandomChestSound(const TArray<USoundBase*>& Sounds) const;
	void PlayChestOpenSound();
	void PlayMimicTransformSound();

	// ===== VFX =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Effect")
	UNiagaraSystem* MimicTransformEffect = nullptr;

	// ===== Reward =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Drop")
	TSubclassOf<AItemDropActor> ChestDropActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Drop")
	FItemData ChestRewardItemData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Drop")
	int32 ChestRewardCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Mimic")
	bool bMimicCapsuleBlocksPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Mimic")
	float MimicPlayerCollisionBlockDelay = 0.5f;

	FTimerHandle TimerHandle_EnableMimicPlayerBlock;

	void SetMimicPlayerCollisionToOverlap();
	void EnableMimicPlayerBlockCollision();

protected:
	void SetInitialChestState();
	void ResolveOpenResult();
	void OpenAsNormalChest();
	void OpenAsMimic();
	void ActivateMimic(AActor* Interactor);
	void SpawnChestReward();

	UFUNCTION()
	void OnInteractSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnInteractSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Tutorial")
	FName ChestTutorialID = TEXT("ChestTutorial_01");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Tutorial")
	TSubclassOf<UUserWidget> ChestTutorialWidgetClass;

	UPROPERTY()
	UUserWidget* ChestTutorialWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Chest|Tutorial")
	bool bWaitingForChestTutorialClose = false;

	void StartChestOpenFlow(AActor* Interactor);
	void ShowChestTutorial(AActor* Interactor);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Visual")
	USkeletalMesh* OpenedChestMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest|Visual")
	TSubclassOf<UAnimInstance> OpenedChestAnimClass;

	UPROPERTY(BlueprintReadOnly, Category = "Chest")
	bool bOpeningInProgress = false;

public:
	UFUNCTION(BlueprintCallable, Category = "Chest|Tutorial")
	void CloseChestTutorialAndContinue();
	void SetOpenedChestState();

	UFUNCTION(BlueprintCallable, Category = "Chest")
	void OpenChestAfterPlayerMontage(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Chest")
	void CancelPendingChestOpen(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Chest")
	FTransform GetChestOpenPointTransform() const;

	virtual bool CanAutoAcquireTarget() const override;
};