#include "TutorialLibraryEntryWidget.h"

void UTutorialLibraryEntryWidget::InitializeEntry(const FTutorialLibraryEntry& InEntry)
{
	EntryData = InEntry;
}

void UTutorialLibraryEntryWidget::HandleClicked()
{
	OnTutorialEntryClicked.Broadcast(EntryData.TutorialID);
}