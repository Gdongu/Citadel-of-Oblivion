#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTypes.h"
#include "InventoryComponent.generated.h"

class AKnight;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COO_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> Items;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FInventoryItem EquippedWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FInventoryItem EquippedHelmet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FInventoryItem EquippedArmor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FInventoryItem EquippedGloves;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	FInventoryItem EquippedBoots;

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FItemData& ItemData, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAtIndex(int32 Index, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(AKnight* OwnerKnight, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipItem(AKnight* OwnerKnight, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UnequipItem(AKnight* OwnerKnight, EEquipmentSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryItem>& GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const FInventoryItem& GetEquippedWeapon() const { return EquippedWeapon; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const FInventoryItem& GetEquippedHelmet() const { return EquippedHelmet; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const FInventoryItem& GetEquippedArmor() const { return EquippedArmor; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const FInventoryItem& GetEquippedGloves() const { return EquippedGloves; }

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const FInventoryItem& GetEquippedBoots() const { return EquippedBoots; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemCountByID(FName ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ConsumeItemByID(FName ItemID, int32 Amount);

protected:
	FInventoryItem* GetEquippedSlotRef(EEquipmentSlot Slot);
	const FInventoryItem* GetEquippedSlotRef(EEquipmentSlot Slot) const;

	void BroadcastInventoryUpdated();
	void RemoveEmptySlots();

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void GetInventorySnapshot(
		TArray<FInventoryItem>& OutItems,
		FInventoryItem& OutWeapon,
		FInventoryItem& OutHelmet,
		FInventoryItem& OutArmor,
		FInventoryItem& OutGloves,
		FInventoryItem& OutBoots
	) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RestoreInventorySnapshot(
		const TArray<FInventoryItem>& InItems,
		const FInventoryItem& InWeapon,
		const FInventoryItem& InHelmet,
		const FInventoryItem& InArmor,
		const FInventoryItem& InGloves,
		const FInventoryItem& InBoots
	);
};