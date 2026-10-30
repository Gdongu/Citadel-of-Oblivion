#include "AnimNotify_BossRangedFire.h"
#include "Components/SkeletalMeshComponent.h"
#include "TutorialBossMonster.h"

void UAnimNotify_BossRangedFire::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
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

	if (ATutorialBossMonster* Boss = Cast<ATutorialBossMonster>(OwnerActor))
	{
		Boss->FireRangedNiagaraProjectile();
	}
}
