#include "InventoryComponent.h"
#include "Knight.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::BroadcastInventoryUpdated()
{
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::RemoveEmptySlots()
{
	Items.RemoveAll([](const FInventoryItem& Item)
		{
			return !Item.IsValid();
		});
}

FInventoryItem* UInventoryComponent::GetEquippedSlotRef(EEquipmentSlot Slot)
{
	switch (Slot)
	{
	case EEquipmentSlot::Weapon: return &EquippedWeapon;
	case EEquipmentSlot::Helmet: return &EquippedHelmet;
	case EEquipmentSlot::Armor:  return &EquippedArmor;
	case EEquipmentSlot::Gloves: return &EquippedGloves;
	case EEquipmentSlot::Boots:  return &EquippedBoots;
	default: return nullptr;
	}
}

const FInventoryItem* UInventoryComponent::GetEquippedSlotRef(EEquipmentSlot Slot) const
{
	switch (Slot)
	{
	case EEquipmentSlot::Weapon: return &EquippedWeapon;
	case EEquipmentSlot::Helmet: return &EquippedHelmet;
	case EEquipmentSlot::Armor:  return &EquippedArmor;
	case EEquipmentSlot::Gloves: return &EquippedGloves;
	case EEquipmentSlot::Boots:  return &EquippedBoots;
	default: return nullptr;
	}
}

bool UInventoryComponent::AddItem(const FItemData& ItemData, int32 Amount)
{
	if (ItemData.ItemID.IsNone() || Amount <= 0)
	{
		return false;
	}

	if (ItemData.bStackable)
	{
		for (FInventoryItem& Slot : Items)
		{
			if (Slot.ItemData.ItemID == ItemData.ItemID)
			{
				Slot.Count = FMath::Clamp(Slot.Count + Amount, 0, ItemData.MaxStackCount);
				BroadcastInventoryUpdated();
				return true;
			}
		}
	}

	FInventoryItem NewItem;
	NewItem.ItemData = ItemData;
	NewItem.Count = Amount;
	Items.Add(NewItem);

	BroadcastInventoryUpdated();
	return true;
}

bool UInventoryComponent::RemoveItemAtIndex(int32 Index, int32 Amount)
{
	if (!Items.IsValidIndex(Index) || Amount <= 0)
	{
		return false;
	}

	Items[Index].Count -= Amount;
	if (Items[Index].Count <= 0)
	{
		Items.RemoveAt(Index);
	}

	BroadcastInventoryUpdated();
	return true;
}

bool UInventoryComponent::UseItem(AKnight* OwnerKnight, int32 Index)
{
	if (!OwnerKnight || !Items.IsValidIndex(Index))
	{
		return false;
	}

	FInventoryItem& Item = Items[Index];
	if (Item.ItemData.ItemType != EItemType::Consumable)
	{
		return false;
	}

	bool bUsed = false;

	// 영웅의 성배: 패널티 게이지 전부 제거
	if (Item.ItemData.ItemID == TEXT("HeroChalice"))
	{
		OwnerKnight->ClearAllHPPenalty();
		bUsed = true;
	}
	// 소형 포션: 현재 최대체력의 25% 회복
	else if (Item.ItemData.ItemID == TEXT("MiniPotion"))
	{
		OwnerKnight->HealByMaxHPPercent(0.25f);
		bUsed = true;
	}
	// 기존 일반 회복 아이템
	else if (Item.ItemData.HealAmount > 0)
	{
		OwnerKnight->Heal(Item.ItemData.HealAmount);
		bUsed = true;
	}

	if (!bUsed)
	{
		return false;
	}

	Item.Count--;
	RemoveEmptySlots();
	BroadcastInventoryUpdated();
	return true;
}

bool UInventoryComponent::EquipItem(AKnight* OwnerKnight, int32 Index)
{
	if (!OwnerKnight || !Items.IsValidIndex(Index))
	{
		return false;
	}

	FInventoryItem ItemToEquip = Items[Index];

	if (ItemToEquip.ItemData.ItemType != EItemType::Equipment)
	{
		return false;
	}

	if (ItemToEquip.ItemData.EquipSlot == EEquipmentSlot::None)
	{
		return false;
	}

	FInventoryItem* EquippedSlot = GetEquippedSlotRef(ItemToEquip.ItemData.EquipSlot);
	if (!EquippedSlot)
	{
		return false;
	}

	// 기존 장비 해제
	if (EquippedSlot->IsValid())
	{
		OwnerKnight->RemoveEquipmentBonus(
			EquippedSlot->ItemData.MaxHPBonus,
			EquippedSlot->ItemData.AttackBonus,
			EquippedSlot->ItemData.MaxStaminaBonus
		);

		AddItem(EquippedSlot->ItemData, 1);
	}

	// 인벤토리에서 제거
	RemoveItemAtIndex(Index, 1);

	// 새 장비 착용
	*EquippedSlot = ItemToEquip;
	EquippedSlot->Count = 1;

	OwnerKnight->ApplyEquipmentBonus(
		ItemToEquip.ItemData.MaxHPBonus,
		ItemToEquip.ItemData.AttackBonus,
		ItemToEquip.ItemData.MaxStaminaBonus
	);

	BroadcastInventoryUpdated();
	return true;
}

bool UInventoryComponent::UnequipItem(AKnight* OwnerKnight, EEquipmentSlot Slot)
{
	if (!OwnerKnight)
	{
		return false;
	}

	FInventoryItem* EquippedSlot = GetEquippedSlotRef(Slot);
	if (!EquippedSlot || !EquippedSlot->IsValid())
	{
		return false;
	}

	OwnerKnight->RemoveEquipmentBonus(
		EquippedSlot->ItemData.MaxHPBonus,
		EquippedSlot->ItemData.AttackBonus,
		EquippedSlot->ItemData.MaxStaminaBonus
	);

	AddItem(EquippedSlot->ItemData, 1);
	*EquippedSlot = FInventoryItem{};

	BroadcastInventoryUpdated();
	return true;
}

int32 UInventoryComponent::GetItemCountByID(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return 0;
	}

	int32 TotalCount = 0;

	for (const FInventoryItem& Item : Items)
	{
		if (Item.ItemData.ItemID == ItemID)
		{
			TotalCount += Item.Count;
		}
	}

	return TotalCount;
}

bool UInventoryComponent::ConsumeItemByID(FName ItemID, int32 Amount)
{
	if (ItemID.IsNone() || Amount <= 0)
	{
		return false;
	}

	int32 RemainingToConsume = Amount;

	for (int32 i = Items.Num() - 1; i >= 0; --i)
	{
		FInventoryItem& Item = Items[i];

		if (Item.ItemData.ItemID != ItemID)
		{
			continue;
		}

		const int32 ConsumeNow = FMath::Min(Item.Count, RemainingToConsume);
		Item.Count -= ConsumeNow;
		RemainingToConsume -= ConsumeNow;

		if (Item.Count <= 0)
		{
			Items.RemoveAt(i);
		}

		if (RemainingToConsume <= 0)
		{
			BroadcastInventoryUpdated();
			return true;
		}
	}

	BroadcastInventoryUpdated();
	return false;
}

void UInventoryComponent::GetInventorySnapshot(
	TArray<FInventoryItem>& OutItems,
	FInventoryItem& OutWeapon,
	FInventoryItem& OutHelmet,
	FInventoryItem& OutArmor,
	FInventoryItem& OutGloves,
	FInventoryItem& OutBoots) const
{
	OutItems = Items;
	OutWeapon = EquippedWeapon;
	OutHelmet = EquippedHelmet;
	OutArmor = EquippedArmor;
	OutGloves = EquippedGloves;
	OutBoots = EquippedBoots;
}

void UInventoryComponent::RestoreInventorySnapshot(
	const TArray<FInventoryItem>& InItems,
	const FInventoryItem& InWeapon,
	const FInventoryItem& InHelmet,
	const FInventoryItem& InArmor,
	const FInventoryItem& InGloves,
	const FInventoryItem& InBoots)
{
	Items = InItems;
	EquippedWeapon = InWeapon;
	EquippedHelmet = InHelmet;
	EquippedArmor = InArmor;
	EquippedGloves = InGloves;
	EquippedBoots = InBoots;

	BroadcastInventoryUpdated();
}