#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "EndingCinematicActor.generated.h"

UCLASS()
class COO_API AEndingCinematicActor : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	AEndingCinematicActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending")
	class USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending")
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending")
	class USphereComponent* InteractSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
	TSubclassOf<class UUserWidget> EndingCinematicWidgetClass;

	UPROPERTY()
	UUserWidget* EndingCinematicWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
	float EndingCinematicDuration = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ending")
	FName StartLevelName = TEXT("MainMenu_NSM_T3");

	UPROPERTY(BlueprintReadOnly, Category = "Ending")
	bool bTriggered = false;

	FTimerHandle TimerHandle_EndingFinish;

	void FinishEndingAndReturnToStart();

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

public:
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;
};