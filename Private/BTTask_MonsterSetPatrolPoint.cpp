#include "BTTask_MonsterSetPatrolPoint.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BaseMonster.h"

UBTTask_MonsterSetPatrolPoint::UBTTask_MonsterSetPatrolPoint()
{
	NodeName = TEXT("Set Patrol Point");
}

EBTNodeResult::Type UBTTask_MonsterSetPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon)
	{
		return EBTNodeResult::Failed;
	}

	ABaseMonster* Monster = Cast<ABaseMonster>(AICon->GetPawn());
	if (!Monster || !Monster->ShouldUsePatrol())
	{
		return EBTNodeResult::Failed;
	}

	AActor* PatrolPoint = Monster->GetCurrentPatrolPoint();
	if (!PatrolPoint)
	{
		return EBTNodeResult::Failed;
	}

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		BB->SetValueAsVector(TEXT("PatrolLocation"), PatrolPoint->GetActorLocation());
	}

	return EBTNodeResult::Succeeded;
}