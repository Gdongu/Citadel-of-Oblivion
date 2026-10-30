#include "FinalBossPhase1.h"
#include "Knight.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "MonsterAIController.h"

float AFinalBossPhase1::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bPhaseTransitionStarted || bIsDead)
	{
		return 0.f;
	}

	const float FinalDamage = FMath::Max(0.f, DamageAmount);
	CurrentHP = FMath::Clamp(CurrentHP - FinalDamage, 0.f, MaxHP);

	if (DamageCauser && DamageCauser->ActorHasTag(TEXT("Player")))
	{
		SetHasDetectedTarget(true);
		SetTargetActor(DamageCauser);

		if (AMonsterAIController* MonsterAI = Cast<AMonsterAIController>(GetController()))
		{
			MonsterAI->SetTargetActor(DamageCauser);
			MonsterAI->EnableFullAlertSight(DamageCauser);
		}

		if (AKnight* Knight = Cast<AKnight>(DamageCauser))
		{
			LastDamagingKnight = Knight;
		}
	}

	OnHPChanged.Broadcast(CurrentHP, MaxHP);
	UpdateHPWidget();
	ShowHPBar();

	if (CurrentHP <= 0.f)
	{
		StartPhaseTransition(DamageCauser);
		return FinalDamage;
	}

	const bool bWasGroggyBeforeHit = bIsGroggy;
	ConsumeGroggyGauge(FinalDamage);

	if (bWasGroggyBeforeHit && bIsGroggy && !bInBackAttackReaction)
	{
		ExitGroggy();
	}

	return FinalDamage;
}

void AFinalBossPhase1::StartPhaseTransition(AActor* DamageCauser)
{
	if (bPhaseTransitionStarted)
	{
		return;
	}

	bPhaseTransitionStarted = true;
	bIsDead = true;
	bIsAttacking = false;
	bAttackTraceActive = false;
	TargetActor = nullptr;

	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
		AICon->ClearFocus(EAIFocusPriority::Default);
	}

	GetCharacterMovement()->DisableMovement();

	AKnight* Knight = Cast<AKnight>(DamageCauser);
	if (!Knight)
	{
		Knight = Cast<AKnight>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	if (Knight)
	{
		if (APlayerController* PC = Cast<APlayerController>(Knight->GetController()))
		{
			PC->SetIgnoreMoveInput(true);
			PC->SetIgnoreLookInput(true);
			PC->bShowMouseCursor = false;
		}
	}

	// 1초 뒤 위젯 표시
	GetWorldTimerManager().SetTimer(
		TimerHandle_ShowPhaseTransitionWidget,
		this,
		&AFinalBossPhase1::ShowPhaseTransitionWidget,
		PhaseTransitionWidgetDelay,
		false
	);

	// 전체 딜레이 후 2페이즈 이동
	GetWorldTimerManager().SetTimer(
		TimerHandle_PhaseTransition,
		this,
		&AFinalBossPhase1::TravelToPhase2,
		PhaseTransitionDelay,
		false
	);
}

void AFinalBossPhase1::TravelToPhase2()
{
	if (PhaseTransitionVideoWidget)
	{
		PhaseTransitionVideoWidget->RemoveFromParent();
		PhaseTransitionVideoWidget = nullptr;
	}

	UGameplayStatics::OpenLevel(this, Phase2LevelName);
}

void AFinalBossPhase1::ShowPhaseTransitionWidget()
{
	if (!PhaseTransitionVideoWidgetClass || PhaseTransitionVideoWidget)
	{
		return;
	}

	PhaseTransitionVideoWidget = CreateWidget<UUserWidget>(
		GetWorld(),
		PhaseTransitionVideoWidgetClass
	);

	if (PhaseTransitionVideoWidget)
	{
		PhaseTransitionVideoWidget->AddToViewport(500);
	}
}