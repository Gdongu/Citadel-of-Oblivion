#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MonsterHPBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class COO_API UMonsterHPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MonsterHP")
	void UpdateHP(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category = "MonsterHP")
	void UpdateMonsterName(const FText& NewName);

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_HP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_MonsterName;
};