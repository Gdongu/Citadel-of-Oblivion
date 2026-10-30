#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopMenuWidgetBase.generated.h"

class AShopNPC;
class AKnight;

UCLASS()
class COO_API UShopMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	AShopNPC* OwnerShopNPC = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Shop")
	AKnight* OwnerKnight = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void InitializeShopMenu(AShopNPC* InNPC, AKnight* InKnight);
};