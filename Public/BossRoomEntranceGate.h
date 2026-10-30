#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "BossRoomEntranceGate.generated.h"

class USphereComponent;

UCLASS()
class COO_API ABossRoomEntranceGate : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	ABossRoomEntranceGate();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* GateFrameMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* BarrierMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USphereComponent* InteractSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* TeleportTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossGate")
	FName BossUniqueIDToCheck = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BossGate")
	bool bBarrierRemoved = false;

	UPROPERTY(BlueprintReadOnly, Category = "BossGate")
	bool bEnteredOnce = false;

	UPROPERTY()
	class ATutorialBossMonster* BoundBoss = nullptr;

	UFUNCTION()
	void HandleObservedBossDied();

	void DisableInteraction();
	void TryBindBossDeathEvent();

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

	void RefreshBarrierState();

public:
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;
};