#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "MemoryShardDrop.generated.h"

UCLASS()
class COO_API AMemoryShardDrop : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	AMemoryShardDrop();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USphereComponent* InteractSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MemoryShard")
	int32 StoredAmount = 0;

public:
	void InitializeDrop(int32 InAmount);

	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	int32 GetStoredAmount() const { return StoredAmount; }

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;
};