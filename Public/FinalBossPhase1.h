#pragma once

#include "CoreMinimal.h"
#include "TutorialBossMonster.h"
#include "FinalBossPhase1.generated.h"

class UUserWidget;

UCLASS()
class COO_API AFinalBossPhase1 : public ATutorialBossMonster
{
	GENERATED_BODY()

public:
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	bool bPhaseTransitionStarted = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	FName Phase2LevelName = TEXT("FinalBossPhase2Map");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	float PhaseTransitionDelay = 11.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Video")
	TSubclassOf<UUserWidget> PhaseTransitionVideoWidgetClass;

	UPROPERTY()
	UUserWidget* PhaseTransitionVideoWidget = nullptr;

	FTimerHandle TimerHandle_PhaseTransition;
	FTimerHandle TimerHandle_ShowPhaseTransitionWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	float PhaseTransitionWidgetDelay = 1.0f;

	void StartPhaseTransition(AActor* DamageCauser);
	void ShowPhaseTransitionWidget();
	void TravelToPhase2();
};