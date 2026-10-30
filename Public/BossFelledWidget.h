#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossFelledWidget.generated.h"

class UTextBlock;

UCLASS()
class COO_API UBossFelledWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Boss Felled")
	void SetBossFelledText(const FText& InText);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_BossFelled = nullptr;
};