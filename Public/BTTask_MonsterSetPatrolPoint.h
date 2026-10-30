#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MonsterSetPatrolPoint.generated.h"

UCLASS()
class COO_API UBTTask_MonsterSetPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MonsterSetPatrolPoint();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};