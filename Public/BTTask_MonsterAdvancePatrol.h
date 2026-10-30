#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MonsterAdvancePatrol.generated.h"

UCLASS()
class COO_API UBTTask_MonsterAdvancePatrol : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MonsterAdvancePatrol();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};