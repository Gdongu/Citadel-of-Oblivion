#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "MonsterAIController.generated.h"

UCLASS()
class COO_API AMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMonsterAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAISenseConfig_Sight* SightConfig;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

public:
	void SetTargetActor(AActor* NewTarget);
	void SetIsDead(bool bNewIsDead);

	static const FName TargetActorKey;
	static const FName IsDeadKey;

	static const FName IsAttackingKey;
	void SetIsAttacking(bool bNewIsAttacking);

	static const FName IsKnockedDownKey;
	void SetIsKnockedDown(bool bNewIsKnockedDown);

	static const FName IsReturningHomeKey;
	void SetIsReturningHome(bool bNewIsReturningHome);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ResetSightToNormal();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void EnableFullAlertSight(AActor* FocusActor);
};