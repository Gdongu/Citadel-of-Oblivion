#include "EQC_TargetActor.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "BaseMonster.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "AIController.h"

void UEQC_TargetActor::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* QuerierObject = QueryInstance.Owner.Get();
	if (!QuerierObject)
	{
		return;
	}

	ABaseMonster* Monster = Cast<ABaseMonster>(QuerierObject);

	// EQS Querier가 AIController인 경우 처리
	if (!Monster)
	{
		if (AAIController* AICon = Cast<AAIController>(QuerierObject))
		{
			Monster = Cast<ABaseMonster>(AICon->GetPawn());
		}
	}

	// 혹시 다른 Actor로 들어온 경우
	if (!Monster)
	{
		if (AActor* QuerierActor = Cast<AActor>(QuerierObject))
		{
			Monster = Cast<ABaseMonster>(QuerierActor);
		}
	}

	if (!Monster)
	{
		return;
	}

	AActor* TargetActor = Monster->GetTargetActor();
	if (!TargetActor)
	{
		return;
	}

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, TargetActor);
}