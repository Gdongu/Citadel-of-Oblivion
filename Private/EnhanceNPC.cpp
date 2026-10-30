#include "EnhanceNPC.h"
#include "Blueprint/UserWidget.h"
#include "Knight.h"
#include "GameFramework/PlayerController.h"
#include "EnhanceMenuWidgetBase.h"

AEnhanceNPC::AEnhanceNPC()
{
	NPCType = ENPCType::Enhance;
}

void AEnhanceNPC::OnEnhanceButtonClicked()
{
	if (!CurrentInteractingKnight || !EnhanceMenuWidgetClass)
	{
		return;
	}

	if (!EnhanceMenuWidget)
	{
		EnhanceMenuWidget = CreateWidget<UUserWidget>(GetWorld(), EnhanceMenuWidgetClass);
	}

	if (UEnhanceMenuWidgetBase* EnhanceWidget = Cast<UEnhanceMenuWidgetBase>(EnhanceMenuWidget))
	{
		EnhanceWidget->InitializeEnhanceMenu(this, CurrentInteractingKnight);
	}

	if (EnhanceMenuWidget)
	{
		EnhanceMenuWidget->AddToViewport(600);
	}

	if (DialogueWidget)
	{
		DialogueWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (APlayerController* PC = Cast<APlayerController>(CurrentInteractingKnight->GetController()))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(EnhanceMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void AEnhanceNPC::ReturnToDialogue()
{
	if (EnhanceMenuWidget)
	{
		EnhanceMenuWidget->RemoveFromParent();
		EnhanceMenuWidget = nullptr;
	}

	if (DialogueWidget)
	{
		DialogueWidget->SetVisibility(ESlateVisibility::Visible);
	}
}