#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FortressGameInstance.h"
#include "FortressSaveGame.generated.h"

UCLASS()
class COO_API UFortressSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	FName SavedLevelName = NAME_None;

	UPROPERTY(SaveGame)
	bool bHasCheckpoint = false;

	UPROPERTY(SaveGame)
	FName LastCheckpointLevelName = NAME_None;

	UPROPERTY(SaveGame)
	FTransform LastCheckpointTransform = FTransform::Identity;

	UPROPERTY(SaveGame)
	TArray<FBonfireData> ActivatedBonfires;

	UPROPERTY(SaveGame)
	int32 CurrentMemoryShards = 0;

	UPROPERTY(SaveGame)
	FMemoryShardDropData PendingMemoryShardDrop;

	UPROPERTY(SaveGame)
	TArray<FInventoryItem> SavedInventoryItems;

	UPROPERTY(SaveGame)
	FInventoryItem SavedEquippedWeapon;

	UPROPERTY(SaveGame)
	FInventoryItem SavedEquippedHelmet;

	UPROPERTY(SaveGame)
	FInventoryItem SavedEquippedArmor;

	UPROPERTY(SaveGame)
	FInventoryItem SavedEquippedGloves;

	UPROPERTY(SaveGame)
	FInventoryItem SavedEquippedBoots;

	UPROPERTY(SaveGame)
	FPlayerEnhancementData SavedEnhancementData;

	UPROPERTY(SaveGame)
	TArray<FName> OpenedDoorIDs;

	UPROPERTY(SaveGame)
	TArray<FName> ConsumedChestIDs;

	UPROPERTY(SaveGame)
	TArray<FName> DefeatedBossIDs;

	UPROPERTY(SaveGame)
	TArray<FName> PlayedBossIntroIDs;

	UPROPERTY(SaveGame)
	bool bHasPlayedTownIntro = false;

	UPROPERTY(SaveGame)
	TArray<FName> SeenTutorialIDs;

	UPROPERTY(SaveGame)
	TArray<FName> ConsumedWorldDropIDs;
};