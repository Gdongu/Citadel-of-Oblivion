#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "PortalActor.generated.h"

UCLASS()
class COO_API APortalActor : public AActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	APortalActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	class USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	class UStaticMeshComponent* PortalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	class USphereComponent* InteractSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FName TargetLevelName = TEXT("Stage1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FName TargetArrivalTag = TEXT("Portal_FromStage0");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FText InteractText = FText::FromString(TEXT("[E] 스테이지 이동"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bUseLoadingScreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	float LoadingScreenDuration = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Portal")
	bool bTriggered = false;

	UFUNCTION()
	void OnInteractSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnInteractSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

public:
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractText_Implementation() const override;
};