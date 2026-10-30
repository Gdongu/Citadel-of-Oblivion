#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialTriggerZone.generated.h"

class UBoxComponent;
class UUserWidget;
class AKnight;

UCLASS()
class COO_API ATutorialTriggerZone : public AActor
{
	GENERATED_BODY()

public:
	ATutorialTriggerZone();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tutorial")
	UBoxComponent* TriggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TSubclassOf<UUserWidget> TutorialWidgetClass;

	UPROPERTY()
	UUserWidget* SpawnedTutorialWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bTriggerOnlyOnce = true;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	bool bAlreadyTriggered = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FName TutorialID = NAME_None;

	UPROPERTY()
	AKnight* CurrentShowingKnight = nullptr;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void ShowTutorialWidget(AKnight* Knight);

public:
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void HideTutorialWidget();
};