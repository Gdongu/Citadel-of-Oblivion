#include "StageBGMManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"

AStageBGMManager::AStageBGMManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStageBGMManager::BeginPlay()
{
	Super::BeginPlay();

	PlayStageBGM();
}

void AStageBGMManager::PlayStageBGM()
{
	if (!StageBGMSound)
	{
		return;
	}

	if (!StageBGMComponent)
	{
		StageBGMComponent = UGameplayStatics::CreateSound2D(
			this,
			StageBGMSound,
			StageBGMVolume,
			1.0f,
			0.0f,
			nullptr,
			true,
			false
		);
	}

	if (!StageBGMComponent)
	{
		return;
	}

	StageBGMComponent->SetPaused(false);

	if (!StageBGMComponent->IsPlaying())
	{
		StageBGMComponent->Play();
	}

	StageBGMComponent->AdjustVolume(FadeInTime, StageBGMVolume);
}

void AStageBGMManager::PauseStageBGMForBoss()
{
	BossBGMBlockCount++;

	if (!StageBGMComponent)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_PauseAfterFade);

	StageBGMComponent->AdjustVolume(FadeOutTime, 0.0f);

	GetWorldTimerManager().SetTimer(
		TimerHandle_PauseAfterFade,
		[this]()
		{
			if (StageBGMComponent && BossBGMBlockCount > 0)
			{
				StageBGMComponent->SetPaused(true);
			}
		},
		FadeOutTime,
		false
	);
}

void AStageBGMManager::ResumeStageBGMAfterBoss()
{
	BossBGMBlockCount = FMath::Max(0, BossBGMBlockCount - 1);

	if (BossBGMBlockCount > 0)
	{
		return;
	}

	if (!StageBGMComponent)
	{
		PlayStageBGM();
		return;
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_PauseAfterFade);

	StageBGMComponent->SetPaused(false);

	if (!StageBGMComponent->IsPlaying())
	{
		StageBGMComponent->Play();
	}

	StageBGMComponent->AdjustVolume(FadeInTime, StageBGMVolume);
}

void AStageBGMManager::StopStageBGM()
{
	BossBGMBlockCount = 0;

	if (StageBGMComponent)
	{
		StageBGMComponent->FadeOut(FadeOutTime, 0.0f);
	}
}

AStageBGMManager* AStageBGMManager::FindStageBGMManager(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AStageBGMManager> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}