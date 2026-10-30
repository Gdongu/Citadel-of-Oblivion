#pragma once

#include "CoreMinimal.h"
#include "PlayerEnhancementTypes.generated.h"

USTRUCT(BlueprintType)
struct FPlayerEnhancementData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	int32 HPLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	int32 StaminaLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	int32 AttackLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	int32 NextSharedCost = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	int32 TotalSpentMemoryShards = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	int32 PotionUpgradeCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enhance")
	float HPPenaltyPercent = 0.f;
};