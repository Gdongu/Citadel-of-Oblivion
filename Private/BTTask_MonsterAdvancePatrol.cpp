#include "BTTask_MonsterAdvancePatrol.h"
#include "AIController.h"
#include "BaseMonster.h"

UBTTask_MonsterAdvancePatrol::UBTTask_MonsterAdvancePatrol()
{
	NodeName = TEXT("Advance Patrol Point");
}

EBTNodeResult::Type UBTTask_MonsterAdvancePatrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon)
	{
		return EBTNodeResult::Failed;
	}

	ABaseMonster* Monster = Cast<ABaseMonster>(AICon->GetPawn());
	if (!Monster)
	{
		return EBTNodeResult::Failed;
	}

	Monster->AdvancePatrolPoint();
	return EBTNodeResult::Succeeded;
}