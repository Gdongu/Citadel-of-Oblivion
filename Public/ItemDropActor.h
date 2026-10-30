#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "ItemTypes.h"
#include "ItemDropActor.generated.h"

UCLASS()
class COO_API AItemDropActor : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	AItemDropActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USphereComponent* InteractSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
	FInventoryItem DroppedItem;

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop|Save")
	FName DropID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop|Save")
	bool bUsePersistentDropSave = true;

	FName MakeDropSaveKey() const;

public:
	UFUNCTION(BlueprintCallable, Category = "Drop")
	void InitializeDrop(const FInventoryItem& InItem);

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;
};