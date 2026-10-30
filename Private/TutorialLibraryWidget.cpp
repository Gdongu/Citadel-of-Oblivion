#include "TutorialLibraryWidget.h"
#include "TutorialLibraryEntryWidget.h"
#include "TutorialPopupWidget.h"
#include "Components/ScrollBox.h"
#include "Blueprint/UserWidget.h"
#include "FortressGameInstance.h"
#include "Knight.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

void UTutorialLibraryWidget::InitializeLibrary(AKnight* InKnight)
{
	OwningKnight = InKnight;
	RefreshTutorialList();
}

void UTutorialLibraryWidget::RefreshTutorialList()
{
	if (!TutorialListScrollBox)
	{
		return;
	}

	TutorialListScrollBox->ClearChildren();

	UFortressGameInstance* GI = GetGameInstance<UFortressGameInstance>();
	if (!GI || !EntryWidgetClass)
	{
		return;
	}

	const TArray<FTutorialLibraryEntry> SeenEntries = GI->GetSeenTutorialEntries();

	for (const FTutorialLibraryEntry& Entry : SeenEntries)
	{
		UTutorialLibraryEntryWidget* EntryWidget =
			CreateWidget<UTutorialLibraryEntryWidget>(GetWorld(), EntryWidgetClass);

		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->InitializeEntry(Entry);
		EntryWidget->OnTutorialEntryClicked.AddDynamic(this, &UTutorialLibraryWidget::OpenTutorialByID);

		TutorialListScrollBox->AddChild(EntryWidget);
	}
}

void UTutorialLibraryWidget::OpenTutorialByID(FName TutorialID)
{
	UFortressGameInstance* GI = GetGameInstance<UFortressGameInstance>();
	if (!GI)
	{
		return;
	}

	FTutorialLibraryEntry FoundEntry;
	if (!GI->GetTutorialEntryByID(TutorialID, FoundEntry))
	{
		return;
	}

	if (!FoundEntry.TutorialWidgetClass)
	{
		return;
	}

	CloseCurrentTutorialPopup();

	UUserWidget* TutorialWidget = CreateWidget<UUserWidget>(GetWorld(), FoundEntry.TutorialWidgetClass);
	if (!TutorialWidget)
	{
		return;
	}

	if (UTutorialPopupWidget* TutorialPopup = Cast<UTutorialPopupWidget>(TutorialWidget))
	{
		TutorialPopup->OwningKnight = OwningKnight;
		TutorialPopup->OwningLibraryWidget = this;
	}

	CurrentTutorialPopup = TutorialWidget;
	TutorialWidget->AddToViewport(500);

	if (APlayerController* PC = OwningKnight ? Cast<APlayerController>(OwningKnight->GetController()) : nullptr)
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TutorialWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void UTutorialLibraryWidget::CloseCurrentTutorialPopup()
{
	if (CurrentTutorialPopup)
	{
		CurrentTutorialPopup->RemoveFromParent();
		CurrentTutorialPopup = nullptr;
	}

	if (APlayerController* PC = OwningKnight ? Cast<APlayerController>(OwningKnight->GetController()) : nullptr)
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void UTutorialLibraryWidget::ReturnToPauseMenu()
{
	CloseCurrentTutorialPopup();

	RemoveFromParent();

	if (!PauseWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("TutorialLibrary ReturnToPauseMenu: PauseWidget is null"));

		if (OwningKnight)
		{
			OwningKnight->RestoreGameplayInputFromUI(true);
		}
		return;
	}

	if (!PauseWidget->IsInViewport())
	{
		PauseWidget->AddToViewport(400);
	}

	PauseWidget->SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* PC = OwningKnight ? Cast<APlayerController>(OwningKnight->GetController()) : nullptr)
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PauseWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void UTutorialLibraryWidget::CloseLibraryAndResumeGame()
{
	CloseCurrentTutorialPopup();

	RemoveFromParent();

	if (PauseWidget)
	{
		PauseWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (OwningKnight)
	{
		OwningKnight->RestoreGameplayInputFromUI(true);
	}
}

void UTutorialLibraryWidget::InitializeLibraryWithPause(AKnight* InKnight, UUserWidget* InPauseWidget)
{
	OwningKnight = InKnight;
	PauseWidget = InPauseWidget;
	RefreshTutorialList();
}