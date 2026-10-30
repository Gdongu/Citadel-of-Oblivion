#pragma once

#include "CoreMinimal.h"
#include "NPCBase.h"
#include "EnhanceNPC.generated.h"

UCLASS()
class COO_API AEnhanceNPC : public ANPCBase
{
	GENERATED_BODY()

public:
	AEnhanceNPC();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Enhance")
	TSubclassOf<class UUserWidget> EnhanceMenuWidgetClass;

	UPROPERTY()
	class UUserWidget* EnhanceMenuWidget;

public:
	virtual void OnEnhanceButtonClicked() override;

	UFUNCTION(BlueprintCallable, Category = "NPC|Enhance")
	void ReturnToDialogue();
};