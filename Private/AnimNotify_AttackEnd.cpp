#include "AnimNotify_AttackEnd.h"
#include "Knight.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_AttackEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
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

	if (AKnight* Knight = Cast<AKnight>(OwnerActor))
	{
		Knight->OnAttackEnded();
	}
}