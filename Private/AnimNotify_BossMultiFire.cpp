#include "AnimNotify_BossMultiFire.h"
#include "Components/SkeletalMeshComponent.h"
#include "FinalBossPhase2.h"

void UAnimNotify_BossMultiFire::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (AFinalBossPhase2* Boss = Cast<AFinalBossPhase2>(OwnerActor))
	{
		Boss->FireMultiRangedProjectiles();
	}
} 

