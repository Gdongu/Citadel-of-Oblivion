#include "ShopMenuWidgetBase.h"
#include "ShopNPC.h"
#include "Knight.h"

void UShopMenuWidgetBase::InitializeShopMenu(AShopNPC* InNPC, AKnight* InKnight)
{
	OwnerShopNPC = InNPC;
	OwnerKnight = InKnight;
}