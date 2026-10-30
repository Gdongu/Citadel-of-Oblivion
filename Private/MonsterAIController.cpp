#include "MonsterAIController.h"
#include "BaseMonster.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Actor.h"

const FName AMonsterAIController::TargetActorKey(TEXT("TargetActor"));
const FName AMonsterAIController::IsDeadKey(TEXT("IsDead"));
const FName AMonsterAIController::IsAttackingKey(TEXT("IsAttacking"));
const FName AMonsterAIController::IsKnockedDownKey(TEXT("IsKnockedDown"));
const FName AMonsterAIController::IsReturningHomeKey(TEXT("IsReturningHome"));

AMonsterAIController::AMonsterAIController()
{
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	// 처음 발견은 정면 시야
	SightConfig->SightRadius = 1200.f;
	SightConfig->LoseSightRadius = 1500.f;
	SightConfig->PeripheralVisionAngleDegrees = 120.f; // 좌우 60 = 총 120도 느낌
	SightConfig->SetMaxAge(2.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
		this,
		&AMonsterAIController::OnTargetPerceptionUpdated
	);
}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABaseMonster* Monster = Cast<ABaseMonster>(InPawn);
	if (!Monster)
	{
		return;
	}

	if (Monster->GetBehaviorTreeAsset())
	{
		RunBehaviorTree(Monster->GetBehaviorTreeAsset());
	}
}

//void AMonsterAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
//{
//	ABaseMonster* Monster = Cast<ABaseMonster>(GetPawn());
//	if (!Monster || !Actor)
//	{
//		return;
//	}
//
//	if (!Actor->ActorHasTag(TEXT("Player")))
//	{
//		return;
//	}
//
//	if (Monster->IsDead())
//	{
//		return;
//	}
//
//	if (Stimulus.WasSuccessfullySensed())
//	{
//		Monster->SetHasDetectedTarget(true);
//		Monster->SetTargetActor(Actor);
//
//		SetFocus(Actor);
//
//		SightConfig->PeripheralVisionAngleDegrees = 180.f;
//		AIPerceptionComp->RequestStimuliListenerUpdate();
//	}
//}

void AMonsterAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	ABaseMonster* Monster = Cast<ABaseMonster>(GetPawn());
	if (!Monster || !Actor)
	{
		return;
	}

	if (!Actor->ActorHasTag(TEXT("Player")))
	{
		return;
	}

	if (Monster->IsDead())
	{
		return;
	}

	if (!Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	// 보스만 AIPerception으로 즉시 타겟팅
	if (Monster->IsBossMonsterForAI())
	{
		Monster->SetHasDetectedTarget(true);
		Monster->SetTargetActor(Actor);

		SetFocus(Actor);

		if (SightConfig && AIPerceptionComp)
		{
			SightConfig->PeripheralVisionAngleDegrees = 180.f;
			AIPerceptionComp->RequestStimuliListenerUpdate();
		}
	}
}

void AMonsterAIController::SetTargetActor(AActor* NewTarget)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(TargetActorKey, NewTarget);
	}
}

void AMonsterAIController::SetIsDead(bool bNewIsDead)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(IsDeadKey, bNewIsDead);
	}
}

void AMonsterAIController::SetIsAttacking(bool bNewIsAttacking)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(IsAttackingKey, bNewIsAttacking);
	}
}

void AMonsterAIController::SetIsKnockedDown(bool bNewIsKnockedDown)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(IsKnockedDownKey, bNewIsKnockedDown);
	}
}

void AMonsterAIController::EnableFullAlertSight(AActor* FocusActor)
{
	if (FocusActor)
	{
		SetFocus(FocusActor);
	}

	if (SightConfig && AIPerceptionComp)
	{
		SightConfig->PeripheralVisionAngleDegrees = 180.f;
		AIPerceptionComp->RequestStimuliListenerUpdate();
	}
}

void AMonsterAIController::SetIsReturningHome(bool bNewIsReturningHome)
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(IsReturningHomeKey, bNewIsReturningHome);
	}
}

void AMonsterAIController::ResetSightToNormal()
{
	ClearFocus(EAIFocusPriority::Default);

	if (SightConfig && AIPerceptionComp)
	{
		SightConfig->PeripheralVisionAngleDegrees = 120.f;
		AIPerceptionComp->RequestStimuliListenerUpdate();
	}
}