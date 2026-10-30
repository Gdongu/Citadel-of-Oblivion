#include "ChestTutorialWidget.h"
#include "MimicChestMonster.h"

void UChestTutorialWidget::CloseAndContinue()
{
	if (OwningChest)
	{
		OwningChest->CloseChestTutorialAndContinue();
	}
}