#include "TutorialPopupWidget.h"
#include "TutorialTriggerZone.h"
#include "TutorialLibraryWidget.h"
#include "Knight.h"

void UTutorialPopupWidget::CloseTutorial()
{
	if (OwningLibraryWidget)
	{
		OwningLibraryWidget->CloseCurrentTutorialPopup();
		return;
	}

	if (OwningTutorialTrigger)
	{
		OwningTutorialTrigger->HideTutorialWidget();

		if (OwningKnight)
		{
			OwningKnight->OnTutorialPopupClosedFromWidget();
		}

		return;
	}

	if (OwningKnight)
	{
		OwningKnight->CloseDeathPenaltyTutorial();
		return;
	}

	RemoveFromParent();
}