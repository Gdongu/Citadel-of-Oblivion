#include "AnimNotify_BossGrabCheck.h"
#include "Components/SkeletalMeshComponent.h"
#include "TutorialBossMonster.h"

void UAnimNotify_BossGrabCheck::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
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
		Boss->StartGrabCheck();
	}
}