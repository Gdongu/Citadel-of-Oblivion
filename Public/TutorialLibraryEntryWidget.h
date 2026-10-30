#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FortressGameInstance.h"
#include "TutorialLibraryEntryWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialEntryClicked, FName, TutorialID);

UCLASS()
class COO_API UTutorialLibraryEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	FTutorialLibraryEntry EntryData;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialEntryClicked OnTutorialEntryClicked;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void InitializeEntry(const FTutorialLibraryEntry& InEntry);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void HandleClicked();
};