#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ItemTypes.h"
#include "PlayerEnhancementTypes.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "FortressGameInstance.generated.h"

class UFortressSaveGame;

USTRUCT(BlueprintType)
struct FBonfireData // 기억의 불씨
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName BonfireID = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FName LevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FTransform BonfireTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly)
	FText BonfireDisplayName = FText::GetEmpty();

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> BonfirePreviewImage;

	UPROPERTY(BlueprintReadOnly)
	int32 BonfireNumber = 0;
};

USTRUCT(BlueprintType)
struct FMemoryShardDropData // 기억의 파편
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bHasDrop = false;

	UPROPERTY(BlueprintReadOnly)
	FName LevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FTransform WorldTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly)
	FName TargetPointTag = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	int32 Amount = 0;
};

USTRUCT(BlueprintType)
struct FTutorialLibraryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName TutorialID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FText DisplayName = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	TSubclassOf<UUserWidget> TutorialWidgetClass;
};

UCLASS()
class COO_API UFortressGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void SetCheckpointTransform(const FTransform& NewTransform);

	UFUNCTION(BlueprintPure, Category = "Checkpoint")
	FTransform GetCheckpointTransform() const;

	UFUNCTION(BlueprintPure, Category = "Checkpoint")
	bool HasCheckpoint() const;

	UFUNCTION(BlueprintPure, Category = "Bonfire")
	bool IsBonfireActivated(FName BonfireID) const;

	UFUNCTION(BlueprintPure, Category = "Bonfire")
	const TArray<FBonfireData>& GetActivatedBonfires() const { return ActivatedBonfires; }

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void SetCheckpointData(const FName LevelName, const FTransform& NewTransform);

	UFUNCTION(BlueprintPure, Category = "Checkpoint")
	FName GetCheckpointLevelName() const;

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void RegisterBonfire(
		FName BonfireID,
		FName LevelName,
		const FTransform& Transform,
		const FText& DisplayName,
		int32 BonfireNumber
	);

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void SetPendingBonfireTravel(FName LevelName, const FTransform& Transform);

	UFUNCTION(BlueprintPure, Category = "Bonfire")
	bool HasPendingBonfireTravel() const;

	UFUNCTION(BlueprintPure, Category = "Bonfire")
	FName GetPendingTravelLevelName() const;

	UFUNCTION(BlueprintPure, Category = "Bonfire")
	FTransform GetPendingTravelTransform() const;

	UFUNCTION(BlueprintCallable, Category = "Bonfire")
	void ClearPendingBonfireTravel();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Checkpoint")
	FTransform LastCheckpointTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Checkpoint")
	bool bHasCheckpoint = false;

	UPROPERTY(BlueprintReadOnly, Category = "Bonfire")
	TArray<FBonfireData> ActivatedBonfires;

	UPROPERTY(BlueprintReadOnly, Category = "Checkpoint")
	FName LastCheckpointLevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Bonfire")
	bool bHasPendingBonfireTravel = false;

	UPROPERTY(BlueprintReadOnly, Category = "Bonfire")
	FName PendingTravelLevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Bonfire")
	FTransform PendingTravelTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TArray<FTutorialLibraryEntry> TutorialLibraryEntries;

public:
	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	int32 GetCurrentMemoryShards() const { return CurrentMemoryShards; }

	UFUNCTION(BlueprintCallable, Category = "MemoryShard")
	void SetCurrentMemoryShards(int32 NewValue);

	UFUNCTION(BlueprintCallable, Category = "MemoryShard")
	void AddMemoryShards(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "MemoryShard")
	void SetPendingMemoryShardDrop(FName LevelName, const FTransform& Transform, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "MemoryShard")
	void SetPendingMemoryShardDropAtTargetPoint(FName LevelName, FName TargetPointTag, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	bool HasPendingMemoryShardDrop() const { return PendingMemoryShardDrop.bHasDrop; }

	UFUNCTION(BlueprintPure, Category = "MemoryShard")
	const FMemoryShardDropData& GetPendingMemoryShardDrop() const { return PendingMemoryShardDrop; }

	UFUNCTION(BlueprintCallable, Category = "MemoryShard")
	void ClearPendingMemoryShardDrop();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	TArray<FTutorialLibraryEntry> GetSeenTutorialEntries() const;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	bool GetTutorialEntryByID(FName TutorialID, FTutorialLibraryEntry& OutEntry) const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "MemoryShard")
	int32 CurrentMemoryShards = 0;

	UPROPERTY(BlueprintReadOnly, Category = "MemoryShard")
	FMemoryShardDropData PendingMemoryShardDrop;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> SavedInventoryItems;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem SavedEquippedWeapon;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem SavedEquippedHelmet;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem SavedEquippedArmor;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem SavedEquippedGloves;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem SavedEquippedBoots;

	UPROPERTY(BlueprintReadOnly, Category = "Door")
	TArray<FName> OpenedDoorIDs;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SaveInventoryData(
		const TArray<FInventoryItem>& InItems,
		const FInventoryItem& InWeapon,
		const FInventoryItem& InHelmet,
		const FInventoryItem& InArmor,
		const FInventoryItem& InGloves,
		const FInventoryItem& InBoots
	);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void LoadInventoryData(
		TArray<FInventoryItem>& OutItems,
		FInventoryItem& OutWeapon,
		FInventoryItem& OutHelmet,
		FInventoryItem& OutArmor,
		FInventoryItem& OutGloves,
		FInventoryItem& OutBoots
	) const;

	UFUNCTION(BlueprintCallable, Category = "Door")
	void MarkDoorOpened(FName DoorID);

	UFUNCTION(BlueprintPure, Category = "Door")
	bool IsDoorOpened(FName DoorID) const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Enhance")
	FPlayerEnhancementData SavedEnhancementData;

	UPROPERTY(BlueprintReadOnly, Category = "Chest")
	TArray<FName> ConsumedChestIDs;

	UPROPERTY(BlueprintReadOnly, Category = "Boss")
	TArray<FName> DefeatedBossIDs;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Intro")
	TArray<FName> PlayedBossIntroIDs;

public:
	UFUNCTION(BlueprintCallable, Category = "Enhance")
	void SaveEnhancementData(const FPlayerEnhancementData& InData);

	UFUNCTION(BlueprintPure, Category = "Enhance")
	FPlayerEnhancementData LoadEnhancementData() const;

	UFUNCTION(BlueprintCallable, Category = "Chest")
	void MarkChestConsumed(FName ChestID);

	UFUNCTION(BlueprintPure, Category = "Chest")
	bool IsChestConsumed(FName ChestID) const;

	UFUNCTION(BlueprintCallable, Category = "Boss")
	void MarkBossDefeated(FName BossID);

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsBossDefeated(FName BossID) const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Intro")
	void MarkBossIntroPlayed(FName BossIntroID);

	UFUNCTION(BlueprintPure, Category = "Boss|Intro")
	bool HasPlayedBossIntro(FName BossIntroID) const;

public:
	UFUNCTION(BlueprintPure, Category = "Intro")
	bool HasPlayedTownIntro() const { return bHasPlayedTownIntro; }

	UFUNCTION(BlueprintCallable, Category = "Intro")
	void SetHasPlayedTownIntro(bool bPlayed);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void MarkTutorialSeen(FName TutorialID);

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool HasSeenTutorial(FName TutorialID) const;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetPendingDeathPenaltyTutorial(bool bPending) { bPendingDeathPenaltyTutorial = bPending; }

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool HasPendingDeathPenaltyTutorial() const { return bPendingDeathPenaltyTutorial; }

	UFUNCTION(BlueprintCallable, Category = "Loading")
	bool ConsumePendingLoadingScreen(float& OutDuration);

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void SetPendingLoadingScreen(float Duration);

	// Portal Travel
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void SetPendingPortalTravel(FName LevelName, FName ArrivalTag);

	UFUNCTION(BlueprintPure, Category = "Portal")
	bool HasPendingPortalTravel() const;

	UFUNCTION(BlueprintPure, Category = "Portal")
	FName GetPendingPortalLevelName() const;

	UFUNCTION(BlueprintPure, Category = "Portal")
	FName GetPendingPortalArrivalTag() const;

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void ClearPendingPortalTravel();

	UFUNCTION(BlueprintCallable, Category = "WorldDrop")
	void MarkWorldDropConsumed(FName DropID);

	UFUNCTION(BlueprintPure, Category = "WorldDrop")
	bool IsWorldDropConsumed(FName DropID) const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Intro")
	bool bHasPlayedTownIntro = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	TArray<FName> SeenTutorialIDs;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
	bool bPendingDeathPenaltyTutorial = false;

	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	bool bShowLoadingScreenOnNextMapOpen = false;

	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	float PendingLoadingScreenDuration = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Portal")
	bool bHasPendingPortalTravel = false;

	UPROPERTY(BlueprintReadOnly, Category = "Portal")
	FName PendingPortalLevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Portal")
	FName PendingPortalArrivalTag = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "WorldDrop")
	TArray<FName> ConsumedWorldDropIDs;

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveProgressToDisk();

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadProgressFromDisk();

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool DoesProgressSaveExist() const;

	UFUNCTION(BlueprintCallable, Category = "Save")
	void DeleteProgressSave();

	UFUNCTION(BlueprintPure, Category = "Save")
	FName GetSavedLevelName() const { return SavedLevelName; }

	UFUNCTION(BlueprintCallable, Category = "Save")
	void ContinueFromSave();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save")
	FString SaveSlotName = TEXT("FortressProgress");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save")
	int32 SaveUserIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FName SavedLevelName = NAME_None;

	void CopyToSaveGame(UFortressSaveGame* SaveGameObject) const;
	void CopyFromSaveGame(const UFortressSaveGame* SaveGameObject);
	FName GetCurrentWorldLevelName() const;
};