#include "AnimNotify_BossAreaNiagara.h"
#include "Components/SkeletalMeshComponent.h"
#include "TutorialBossMonster.h"

void UAnimNotify_BossAreaNiagara::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
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
		Boss->ActivateAreaNiagaraAttack();
	}
}
