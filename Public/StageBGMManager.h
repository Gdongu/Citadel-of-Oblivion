#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageBGMManager.generated.h"

class USoundBase;
class UAudioComponent;

UCLASS()
class COO_API AStageBGMManager : public AActor
{
	GENERATED_BODY()

public:
	AStageBGMManager();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage BGM")
	USoundBase* StageBGMSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage BGM")
	float StageBGMVolume = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage BGM")
	float FadeInTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage BGM")
	float FadeOutTime = 1.0f;

	UPROPERTY()
	UAudioComponent* StageBGMComponent = nullptr;

	FTimerHandle TimerHandle_PauseAfterFade;

	int32 BossBGMBlockCount = 0;

public:
	UFUNCTION(BlueprintCallable, Category = "Stage BGM")
	void PlayStageBGM();

	UFUNCTION(BlueprintCallable, Category = "Stage BGM")
	void PauseStageBGMForBoss();

	UFUNCTION(BlueprintCallable, Category = "Stage BGM")
	void ResumeStageBGMAfterBoss();

	UFUNCTION(BlueprintCallable, Category = "Stage BGM")
	void StopStageBGM();

	static AStageBGMManager* FindStageBGMManager(UWorld* World);
};