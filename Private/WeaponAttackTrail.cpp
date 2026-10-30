#include "WeaponAttackTrail.h"
#include "Knight.h"
#include "Components/SkeletalMeshComponent.h"

void UWeaponAttackTrail::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference
)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (AKnight* Knight = Cast<AKnight>(MeshComp->GetOwner()))
	{
		Knight->StartWeaponAttackTrail();
	}
}

void UWeaponAttackTrail::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference
)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (AKnight* Knight = Cast<AKnight>(MeshComp->GetOwner()))
	{
		Knight->StopWeaponAttackTrail();
	}
}