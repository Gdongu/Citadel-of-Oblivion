#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "Bonfire.generated.h"

class UAudioComponent;
class USoundBase;

UCLASS()
class COO_API ABonfire : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	ABonfire();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bonfire")
	class USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bonfire")
	class UStaticMeshComponent* BonfireMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bonfire")
	class USphereComponent* InteractSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bonfire")
	class UNiagaraComponent* FireEffectComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	class UNiagaraSystem* FireEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bonfire|Audio")
	UAudioComponent* FireAudioComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire|Audio")
	USoundBase* FireLoopSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire|Audio")
	float FireSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire|Audio")
	float FireSoundFadeTime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	FName RespawnPointTag = TEXT("RespawnPoint");

	UPROPERTY(BlueprintReadOnly, Category = "Bonfire")
	bool bActivated = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	FName BonfireID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	int32 BonfireNumber = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	FText BonfireDisplayName;

public:
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void ActivateBonfire();

	UFUNCTION(BlueprintPure, Category = "Bonfire")
	FTransform GetRespawnTransform() const;

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void OnPlayerFinishedInteract(AActor* Interactor);

	UFUNCTION(BlueprintPure, Category = "Bonfire")
	bool IsActivated() const { return bActivated; }

	UFUNCTION(BlueprintPure, Category = "Bonfire")
	FTransform GetRestTransform() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bonfire")
	FName RestPointTag = TEXT("RestPoint");

protected:
	USceneComponent* FindRespawnPointComponent() const;

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

	void StartFireSoundIfPlayerInside();
	void StopFireSound();
};