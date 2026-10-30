#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialPopupWidget.h"
#include "ChestTutorialWidget.generated.h"

class AMimicChestMonster;

UCLASS()
class COO_API UChestTutorialWidget : public UTutorialPopupWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "ChestTutorial")
	AMimicChestMonster* OwningChest = nullptr;

	UFUNCTION(BlueprintCallable, Category = "ChestTutorial")
	void CloseAndContinue();
};