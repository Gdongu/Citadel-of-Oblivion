#include "BTTask_MonsterAttack.h"
#include "AIController.h"
#include "BaseMonster.h"

UBTTask_MonsterAttack::UBTTask_MonsterAttack()
{
	NodeName = TEXT("Monster Attack");
}

EBTNodeResult::Type UBTTask_MonsterAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	if (!Monster->CanAttack())
	{
		return EBTNodeResult::Failed;
	}

	Monster->PerformAttack();
	return EBTNodeResult::Succeeded;
}