#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "DoorInteractActor.generated.h"

class USphereComponent;
class UTimelineComponent;
class UCurveFloat;
class USoundBase;

UCLASS()
class COO_API ADoorInteractActor : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	ADoorInteractActor();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* DoorFrameMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* LeftDoorPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* RightDoorPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* LeftDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* RightDoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float LeftOpenAngle = -90.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float RightOpenAngle = 90.f;

	UPROPERTY()
	FRotator LeftClosedRotation;

	UPROPERTY()
	FRotator RightClosedRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USphereComponent* InteractSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UTimelineComponent* DoorTimeline;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	UCurveFloat* DoorOpenCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bOpened = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bOpening = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	UAnimMontage* PlayerOpenMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Audio")
	USoundBase* DoorOpenSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Audio")
	float DoorOpenSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Audio")
	float DoorOpenSoundPitchMin = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Audio")
	float DoorOpenSoundPitchMax = 1.04f;

	void PlayDoorOpenSound();

	UFUNCTION()
	void UpdateDoorRotation(float Alpha);

	UFUNCTION()
	void OnDoorTimelineFinished();

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

	void StartOpenDoor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	FName DoorUniqueID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Lock")
	bool bRequireBossDefeat = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Lock")
	FName RequiredBossID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Lock")
	FText LockedMessage = FText::FromString(TEXT("문이 굳게 닫혀 있다."));

public:
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;
	bool IsAlreadyOpened() const;
	void MarkDoorAsOpened();
	void ApplyOpenedStateInstantly();
	bool IsDoorUnlocked() const;
};