#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnhanceMenuWidgetBase.generated.h"

class AEnhanceNPC;
class AKnight;

UCLASS()
class COO_API UEnhanceMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Enhance")
	AEnhanceNPC* OwnerEnhanceNPC = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Enhance")
	AKnight* OwnerKnight = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Enhance")
	void InitializeEnhanceMenu(AEnhanceNPC* InNPC, AKnight* InKnight);
};