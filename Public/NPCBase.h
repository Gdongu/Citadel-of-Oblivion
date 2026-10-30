#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "NPCBase.generated.h"

UENUM(BlueprintType)
enum class ENPCType : uint8
{
	None,
	Enhance,
	Shop
};

UCLASS()
class COO_API ANPCBase : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	ANPCBase();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USkeletalMeshComponent* NPCMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USphereComponent* InteractSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USceneComponent* DialogueCameraPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	ENPCType NPCType = ENPCType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText NPCName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	TArray<FText> DialogueLines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	TSubclassOf<class UUserWidget> DialogueWidgetClass;

	UPROPERTY()
	class UUserWidget* DialogueWidget;

	UPROPERTY()
	class AKnight* CurrentInteractingKnight;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	int32 CurrentDialogueIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "NPC")
	bool bIsTalking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UCameraComponent* DialogueCamera;

public:
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void StartDialogue(class AKnight* Knight);

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void EndDialogue();

	UFUNCTION(BlueprintPure, Category = "NPC")
	FText GetCurrentDialogueText() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	FText GetNPCName() const { return NPCName; }

	UFUNCTION(BlueprintPure, Category = "NPC")
	bool IsLastDialogueLine() const;

	UFUNCTION(BlueprintPure, Category = "NPC")
	ENPCType GetNPCType() const { return NPCType; }

	UFUNCTION(BlueprintCallable, Category = "NPC")
	virtual void OnShopButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "NPC")
	virtual void OnEnhanceButtonClicked();

protected:
	void EnterDialogueCamera(class APlayerController* PC);
	void ExitDialogueCamera(class APlayerController* PC);
};