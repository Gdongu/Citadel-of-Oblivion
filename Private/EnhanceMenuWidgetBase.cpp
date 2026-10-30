#include "EnhanceMenuWidgetBase.h"
#include "EnhanceNPC.h"
#include "Knight.h"

void UEnhanceMenuWidgetBase::InitializeEnhanceMenu(AEnhanceNPC* InNPC, AKnight* InKnight)
{
	OwnerEnhanceNPC = InNPC;
	OwnerKnight = InKnight;
}