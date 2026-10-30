#include "ShopNPC.h"
#include "Blueprint/UserWidget.h"
#include "Knight.h"
#include "GameFramework/PlayerController.h"
#include "ShopMenuWidgetBase.h"
#include "InventoryComponent.h"

AShopNPC::AShopNPC()
{
	NPCType = ENPCType::Shop;
}

void AShopNPC::OnShopButtonClicked()
{
	if (!CurrentInteractingKnight || !ShopMenuWidgetClass)
	{
		return;
	}

	if (!ShopMenuWidget)
	{
		ShopMenuWidget = CreateWidget<UUserWidget>(GetWorld(), ShopMenuWidgetClass);
	}

	if (UShopMenuWidgetBase* ShopWidget = Cast<UShopMenuWidgetBase>(ShopMenuWidget))
	{
		ShopWidget->InitializeShopMenu(this, CurrentInteractingKnight);
	}

	if (ShopMenuWidget)
	{
		ShopMenuWidget->AddToViewport(600);
	}

	if (DialogueWidget)
	{
		DialogueWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (APlayerController* PC = Cast<APlayerController>(CurrentInteractingKnight->GetController()))
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ShopMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void AShopNPC::ReturnToDialogue()
{
	if (ShopMenuWidget)
	{
		ShopMenuWidget->RemoveFromParent();
		ShopMenuWidget = nullptr;
	}

	if (DialogueWidget)
	{
		DialogueWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

bool AShopNPC::BuyItem(int32 ItemIndex)
{
	if (!CurrentInteractingKnight || !ShopItems.IsValidIndex(ItemIndex))
	{
		return false;
	}

	FShopItemEntry& Entry = ShopItems[ItemIndex];

	if (Entry.ItemData.ItemID.IsNone())
	{
		return false;
	}

	if (Entry.RemainingStock == 0)
	{
		return false;
	}

	if (CurrentInteractingKnight->GetCurrentMemoryShardCount() < Entry.Price)
	{
		return false;
	}

	UInventoryComponent* InvComp = CurrentInteractingKnight->GetInventoryComp();
	if (!InvComp)
	{
		return false;
	}

	if (!InvComp->AddItem(Entry.ItemData, 1))
	{
		return false;
	}

	CurrentInteractingKnight->SetMemoryShards(
		CurrentInteractingKnight->GetCurrentMemoryShardCount() - Entry.Price
	);

	if (Entry.RemainingStock > 0)
	{
		Entry.RemainingStock -= 1;
	}

	return true;
}

int32 AShopNPC::GetItemPrice(int32 ItemIndex) const
{
	return ShopItems.IsValidIndex(ItemIndex) ? ShopItems[ItemIndex].Price : 0;
}

int32 AShopNPC::GetItemRemainingStock(int32 ItemIndex) const
{
	return ShopItems.IsValidIndex(ItemIndex) ? ShopItems[ItemIndex].RemainingStock : 0;
}

bool AShopNPC::IsItemSoldOut(int32 ItemIndex) const
{
	return ShopItems.IsValidIndex(ItemIndex) && ShopItems[ItemIndex].RemainingStock == 0;
}