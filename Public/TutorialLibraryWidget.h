#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FortressGameInstance.h"
#include "TutorialLibraryWidget.generated.h"

class UScrollBox;
class UUserWidget;
class AKnight;

UCLASS()
class COO_API UTutorialLibraryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TSubclassOf<class UTutorialLibraryEntryWidget> EntryWidgetClass;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UScrollBox* TutorialListScrollBox;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	AKnight* OwningKnight = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Tutorial")
	UUserWidget* PauseWidget = nullptr;

	UPROPERTY()
	UUserWidget* CurrentTutorialPopup = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeLibrary(AKnight* InKnight);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void RefreshTutorialList();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void OpenTutorialByID(FName TutorialID);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CloseCurrentTutorialPopup();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ReturnToPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CloseLibraryAndResumeGame();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeLibraryWithPause(AKnight* InKnight, UUserWidget* InPauseWidget);
};