#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialPopupWidget.generated.h"

class ATutorialTriggerZone;
class AKnight;
class UTutorialLibraryWidget;

UCLASS()
class COO_API UTutorialPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Tutorial")
	ATutorialTriggerZone* OwningTutorialTrigger = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Tutorial")
	AKnight* OwningKnight = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Tutorial")
	UTutorialLibraryWidget* OwningLibraryWidget = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CloseTutorial();
};