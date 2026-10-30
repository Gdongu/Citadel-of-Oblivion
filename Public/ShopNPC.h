#pragma once

#include "CoreMinimal.h"
#include "NPCBase.h"
#include "ItemTypes.h"
#include "ShopNPC.generated.h"

USTRUCT(BlueprintType)
struct FShopItemEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	FItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	int32 Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	int32 RemainingStock = -1;
};

UCLASS()
class COO_API AShopNPC : public ANPCBase
{
	GENERATED_BODY()

public:
	AShopNPC();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Shop")
	TSubclassOf<class UUserWidget> ShopMenuWidgetClass;

	UPROPERTY()
	class UUserWidget* ShopMenuWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Shop")
	TArray<FShopItemEntry> ShopItems;

public:
	virtual void OnShopButtonClicked() override;

	UFUNCTION(BlueprintCallable, Category = "NPC|Shop")
	void ReturnToDialogue();

	UFUNCTION(BlueprintPure, Category = "NPC|Shop")
	const TArray<FShopItemEntry>& GetShopItems() const { return ShopItems; }

	UFUNCTION(BlueprintCallable, Category = "NPC|Shop")
	bool BuyItem(int32 ItemIndex);

	UFUNCTION(BlueprintPure, Category = "NPC|Shop")
	int32 GetItemPrice(int32 ItemIndex) const;

	UFUNCTION(BlueprintPure, Category = "NPC|Shop")
	int32 GetItemRemainingStock(int32 ItemIndex) const;

	UFUNCTION(BlueprintPure, Category = "NPC|Shop")
	bool IsItemSoldOut(int32 ItemIndex) const;
};