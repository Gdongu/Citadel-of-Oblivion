#include "FortressGameInstance.h"
#include "FortressSaveGame.h"
#include "Kismet/GameplayStatics.h"

void UFortressGameInstance::SetCheckpointTransform(const FTransform& NewTransform)
{
	LastCheckpointTransform = NewTransform;
	bHasCheckpoint = true;

	SaveProgressToDisk();
}

FTransform UFortressGameInstance::GetCheckpointTransform() const
{
	return LastCheckpointTransform;
}

bool UFortressGameInstance::HasCheckpoint() const
{
	return bHasCheckpoint;
}

bool UFortressGameInstance::IsBonfireActivated(FName BonfireID) const
{
	if (BonfireID.IsNone())
	{
		return false;
	}

	for (const FBonfireData& Data : ActivatedBonfires)
	{
		if (Data.BonfireID == BonfireID)
		{
			return true;
		}
	}

	return false;
}

void UFortressGameInstance::SetCheckpointData(const FName LevelName, const FTransform& NewTransform)
{
	LastCheckpointLevelName = LevelName;
	LastCheckpointTransform = NewTransform;
	bHasCheckpoint = true;

	SaveProgressToDisk();
}

FName UFortressGameInstance::GetCheckpointLevelName() const
{
	return LastCheckpointLevelName;
}

void UFortressGameInstance::RegisterBonfire(
	FName BonfireID,
	FName LevelName,
	const FTransform& Transform,
	const FText& DisplayName,
	int32 BonfireNumber)
{
	if (BonfireID.IsNone() || LevelName.IsNone())
	{
		return;
	}

	for (FBonfireData& Data : ActivatedBonfires)
	{
		if (Data.BonfireID == BonfireID)
		{
			Data.LevelName = LevelName;
			Data.BonfireTransform = Transform;
			Data.BonfireDisplayName = DisplayName;
			Data.BonfireNumber = BonfireNumber;

			ActivatedBonfires.Sort(
				[](const FBonfireData& A, const FBonfireData& B)
				{
					if (A.BonfireNumber == B.BonfireNumber)
					{
						return A.BonfireID.LexicalLess(B.BonfireID);
					}

					return A.BonfireNumber < B.BonfireNumber;
				}
			);

			SaveProgressToDisk();
			return;
		}
	}

	FBonfireData NewData;
	NewData.BonfireID = BonfireID;
	NewData.LevelName = LevelName;
	NewData.BonfireTransform = Transform;
	NewData.BonfireDisplayName = DisplayName;
	NewData.BonfireNumber = BonfireNumber;

	ActivatedBonfires.Add(NewData);

	ActivatedBonfires.Sort(
		[](const FBonfireData& A, const FBonfireData& B)
		{
			if (A.BonfireNumber == B.BonfireNumber)
			{
				return A.BonfireID.LexicalLess(B.BonfireID);
			}

			return A.BonfireNumber < B.BonfireNumber;
		}
	);

	SaveProgressToDisk();
}

void UFortressGameInstance::SetPendingBonfireTravel(FName LevelName, const FTransform& Transform)
{
	bHasPendingBonfireTravel = true;
	PendingTravelLevelName = LevelName;
	PendingTravelTransform = Transform;
}

bool UFortressGameInstance::HasPendingBonfireTravel() const
{
	return bHasPendingBonfireTravel;
}

FName UFortressGameInstance::GetPendingTravelLevelName() const
{
	return PendingTravelLevelName;
}

FTransform UFortressGameInstance::GetPendingTravelTransform() const
{
	return PendingTravelTransform;
}

void UFortressGameInstance::ClearPendingBonfireTravel()
{
	bHasPendingBonfireTravel = false;
	PendingTravelLevelName = NAME_None;
	PendingTravelTransform = FTransform::Identity;
}

void UFortressGameInstance::SetCurrentMemoryShards(int32 NewValue)
{
	CurrentMemoryShards = FMath::Max(0, NewValue);

	SaveProgressToDisk();
}

void UFortressGameInstance::AddMemoryShards(int32 Amount)
{
	CurrentMemoryShards = FMath::Max(0, CurrentMemoryShards + Amount);

	SaveProgressToDisk();
}

void UFortressGameInstance::SetPendingMemoryShardDrop(FName LevelName, const FTransform& Transform, int32 Amount)
{
	PendingMemoryShardDrop.bHasDrop = true;
	PendingMemoryShardDrop.LevelName = LevelName;
	PendingMemoryShardDrop.WorldTransform = Transform;
	PendingMemoryShardDrop.TargetPointTag = NAME_None;
	PendingMemoryShardDrop.Amount = FMath::Max(0, Amount);

	SaveProgressToDisk();
}

void UFortressGameInstance::SetPendingMemoryShardDropAtTargetPoint(
	FName LevelName,
	FName TargetPointTag,
	int32 Amount)
{
	PendingMemoryShardDrop.bHasDrop = true;
	PendingMemoryShardDrop.LevelName = LevelName;
	PendingMemoryShardDrop.WorldTransform = FTransform::Identity;
	PendingMemoryShardDrop.TargetPointTag = TargetPointTag;
	PendingMemoryShardDrop.Amount = FMath::Max(0, Amount);

	SaveProgressToDisk();
}

void UFortressGameInstance::ClearPendingMemoryShardDrop()
{
	PendingMemoryShardDrop = FMemoryShardDropData{};

	SaveProgressToDisk();
}

void UFortressGameInstance::SaveInventoryData(
	const TArray<FInventoryItem>& InItems,
	const FInventoryItem& InWeapon,
	const FInventoryItem& InHelmet,
	const FInventoryItem& InArmor,
	const FInventoryItem& InGloves,
	const FInventoryItem& InBoots)
{
	SavedInventoryItems = InItems;
	SavedEquippedWeapon = InWeapon;
	SavedEquippedHelmet = InHelmet;
	SavedEquippedArmor = InArmor;
	SavedEquippedGloves = InGloves;
	SavedEquippedBoots = InBoots;

	SaveProgressToDisk();
}

void UFortressGameInstance::LoadInventoryData(
	TArray<FInventoryItem>& OutItems,
	FInventoryItem& OutWeapon,
	FInventoryItem& OutHelmet,
	FInventoryItem& OutArmor,
	FInventoryItem& OutGloves,
	FInventoryItem& OutBoots) const
{
	OutItems = SavedInventoryItems;
	OutWeapon = SavedEquippedWeapon;
	OutHelmet = SavedEquippedHelmet;
	OutArmor = SavedEquippedArmor;
	OutGloves = SavedEquippedGloves;
	OutBoots = SavedEquippedBoots;
}

void UFortressGameInstance::SaveEnhancementData(const FPlayerEnhancementData& InData)
{
	SavedEnhancementData = InData;

	SaveProgressToDisk();
}

FPlayerEnhancementData UFortressGameInstance::LoadEnhancementData() const
{
	return SavedEnhancementData;
}

void UFortressGameInstance::MarkChestConsumed(FName ChestID)
{
	if (ChestID.IsNone())
	{
		return;
	}

	if (!ConsumedChestIDs.Contains(ChestID))
	{
		ConsumedChestIDs.Add(ChestID);
	}

	SaveProgressToDisk();
}

bool UFortressGameInstance::IsChestConsumed(FName ChestID) const
{
	if (ChestID.IsNone())
	{
		return false;
	}

	return ConsumedChestIDs.Contains(ChestID);
}

void UFortressGameInstance::MarkBossDefeated(FName BossID)
{
	if (BossID.IsNone())
	{
		return;
	}

	if (!DefeatedBossIDs.Contains(BossID))
	{
		DefeatedBossIDs.Add(BossID);
	}

	SaveProgressToDisk();
}

bool UFortressGameInstance::IsBossDefeated(FName BossID) const
{
	if (BossID.IsNone())
	{
		return false;
	}

	return DefeatedBossIDs.Contains(BossID);
}

void UFortressGameInstance::MarkDoorOpened(FName DoorID)
{
	if (DoorID.IsNone())
	{
		return;
	}

	if (!OpenedDoorIDs.Contains(DoorID))
	{
		OpenedDoorIDs.Add(DoorID);
	}

	SaveProgressToDisk();
}

bool UFortressGameInstance::IsDoorOpened(FName DoorID) const
{
	if (DoorID.IsNone())
	{
		return false;
	}

	return OpenedDoorIDs.Contains(DoorID);
}

void UFortressGameInstance::MarkTutorialSeen(FName TutorialID)
{
	if (TutorialID.IsNone())
	{
		return;
	}

	if (!SeenTutorialIDs.Contains(TutorialID))
	{
		SeenTutorialIDs.Add(TutorialID);
	}

	SaveProgressToDisk();
}

bool UFortressGameInstance::HasSeenTutorial(FName TutorialID) const
{
	if (TutorialID.IsNone())
	{
		return false;
	}

	return SeenTutorialIDs.Contains(TutorialID);
}

void UFortressGameInstance::SetPendingLoadingScreen(float Duration)
{
	bShowLoadingScreenOnNextMapOpen = true;
	PendingLoadingScreenDuration = FMath::Max(0.1f, Duration);
}

bool UFortressGameInstance::ConsumePendingLoadingScreen(float& OutDuration)
{
	if(!bShowLoadingScreenOnNextMapOpen)
	{
		return false;
	}

	OutDuration = PendingLoadingScreenDuration;
	bShowLoadingScreenOnNextMapOpen = false;
	PendingLoadingScreenDuration = 0.f;
	return true;
}

TArray<FTutorialLibraryEntry> UFortressGameInstance::GetSeenTutorialEntries() const
{
	TArray<FTutorialLibraryEntry> Result;

	for (const FTutorialLibraryEntry& Entry : TutorialLibraryEntries)
	{
		if (Entry.TutorialID.IsNone())
		{
			continue;
		}

		if (SeenTutorialIDs.Contains(Entry.TutorialID))
		{
			Result.Add(Entry);
		}
	}

	return Result;
}

bool UFortressGameInstance::GetTutorialEntryByID(FName TutorialID, FTutorialLibraryEntry& OutEntry) const
{
	if (TutorialID.IsNone())
	{
		return false;
	}

	for (const FTutorialLibraryEntry& Entry : TutorialLibraryEntries)
	{
		if (Entry.TutorialID == TutorialID)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

void UFortressGameInstance::SetPendingPortalTravel(FName LevelName, FName ArrivalTag)
{
	if (LevelName.IsNone() || ArrivalTag.IsNone())
	{
		return;
	}

	bHasPendingPortalTravel = true;
	PendingPortalLevelName = LevelName;
	PendingPortalArrivalTag = ArrivalTag;
}

bool UFortressGameInstance::HasPendingPortalTravel() const
{
	return bHasPendingPortalTravel;
}

FName UFortressGameInstance::GetPendingPortalLevelName() const
{
	return PendingPortalLevelName;
}

FName UFortressGameInstance::GetPendingPortalArrivalTag() const
{
	return PendingPortalArrivalTag;
}

void UFortressGameInstance::ClearPendingPortalTravel()
{
	bHasPendingPortalTravel = false;
	PendingPortalLevelName = NAME_None;
	PendingPortalArrivalTag = NAME_None;
}

void UFortressGameInstance::MarkBossIntroPlayed(FName BossIntroID)
{
	if (BossIntroID.IsNone())
	{
		return;
	}

	if (!PlayedBossIntroIDs.Contains(BossIntroID))
	{
		PlayedBossIntroIDs.Add(BossIntroID);
	}

	SaveProgressToDisk();
}

bool UFortressGameInstance::HasPlayedBossIntro(FName BossIntroID) const
{
	if (BossIntroID.IsNone())
	{
		return false;
	}

	return PlayedBossIntroIDs.Contains(BossIntroID);
}

void UFortressGameInstance::MarkWorldDropConsumed(FName DropID)
{
	if (DropID.IsNone())
	{
		return;
	}

	if (!ConsumedWorldDropIDs.Contains(DropID))
	{
		ConsumedWorldDropIDs.Add(DropID);
	}

	SaveProgressToDisk();
}

bool UFortressGameInstance::IsWorldDropConsumed(FName DropID) const
{
	if (DropID.IsNone())
	{
		return false;
	}

	return ConsumedWorldDropIDs.Contains(DropID);
}

void UFortressGameInstance::Init()
{
	Super::Init();

	LoadProgressFromDisk();
}

void UFortressGameInstance::Shutdown()
{
	SaveProgressToDisk();

	Super::Shutdown();
}

FName UFortressGameInstance::GetCurrentWorldLevelName() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return SavedLevelName;
	}

	return FName(*UGameplayStatics::GetCurrentLevelName(this, true));
}

bool UFortressGameInstance::DoesProgressSaveExist() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

bool UFortressGameInstance::SaveProgressToDisk()
{
	UFortressSaveGame* SaveGameObject = Cast<UFortressSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UFortressSaveGame::StaticClass())
	);

	if (!SaveGameObject)
	{
		return false;
	}

	CopyToSaveGame(SaveGameObject);

	return UGameplayStatics::SaveGameToSlot(
		SaveGameObject,
		SaveSlotName,
		SaveUserIndex
	);
}

bool UFortressGameInstance::LoadProgressFromDisk()
{
	if (!DoesProgressSaveExist())
	{
		return false;
	}

	UFortressSaveGame* SaveGameObject = Cast<UFortressSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex)
	);

	if (!SaveGameObject)
	{
		return false;
	}

	CopyFromSaveGame(SaveGameObject);
	return true;
}

void UFortressGameInstance::DeleteProgressSave()
{
	if (DoesProgressSaveExist())
	{
		UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	}

	SavedLevelName = NAME_None;
	LastCheckpointTransform = FTransform::Identity;
	bHasCheckpoint = false;
	LastCheckpointLevelName = NAME_None;

	ActivatedBonfires.Empty();
	CurrentMemoryShards = 0;
	PendingMemoryShardDrop = FMemoryShardDropData{};

	SavedInventoryItems.Empty();
	SavedEquippedWeapon = FInventoryItem{};
	SavedEquippedHelmet = FInventoryItem{};
	SavedEquippedArmor = FInventoryItem{};
	SavedEquippedGloves = FInventoryItem{};
	SavedEquippedBoots = FInventoryItem{};

	SavedEnhancementData = FPlayerEnhancementData{};

	OpenedDoorIDs.Empty();
	ConsumedChestIDs.Empty();
	DefeatedBossIDs.Empty();
	PlayedBossIntroIDs.Empty();
	SeenTutorialIDs.Empty();
	ConsumedWorldDropIDs.Empty();

	bHasPlayedTownIntro = false;
}

void UFortressGameInstance::SetHasPlayedTownIntro(bool bPlayed)
{
	bHasPlayedTownIntro = bPlayed;

	SaveProgressToDisk();
}

void UFortressGameInstance::ContinueFromSave()
{
	if (!LoadProgressFromDisk())
	{
		return;
	}

	FName LevelToOpen = SavedLevelName;

	if (LevelToOpen.IsNone())
	{
		LevelToOpen = LastCheckpointLevelName;
	}

	if (LevelToOpen.IsNone())
	{
		return;
	}

	UGameplayStatics::OpenLevel(this, LevelToOpen);
}

void UFortressGameInstance::CopyToSaveGame(UFortressSaveGame* SaveGameObject) const
{
	if (!SaveGameObject)
	{
		return;
	}

	SaveGameObject->SavedLevelName = GetCurrentWorldLevelName();

	SaveGameObject->bHasCheckpoint = bHasCheckpoint;
	SaveGameObject->LastCheckpointLevelName = LastCheckpointLevelName;
	SaveGameObject->LastCheckpointTransform = LastCheckpointTransform;

	SaveGameObject->ActivatedBonfires = ActivatedBonfires;

	SaveGameObject->CurrentMemoryShards = CurrentMemoryShards;
	SaveGameObject->PendingMemoryShardDrop = PendingMemoryShardDrop;

	SaveGameObject->SavedInventoryItems = SavedInventoryItems;
	SaveGameObject->SavedEquippedWeapon = SavedEquippedWeapon;
	SaveGameObject->SavedEquippedHelmet = SavedEquippedHelmet;
	SaveGameObject->SavedEquippedArmor = SavedEquippedArmor;
	SaveGameObject->SavedEquippedGloves = SavedEquippedGloves;
	SaveGameObject->SavedEquippedBoots = SavedEquippedBoots;

	SaveGameObject->SavedEnhancementData = SavedEnhancementData;

	SaveGameObject->OpenedDoorIDs = OpenedDoorIDs;
	SaveGameObject->ConsumedChestIDs = ConsumedChestIDs;
	SaveGameObject->DefeatedBossIDs = DefeatedBossIDs;
	SaveGameObject->PlayedBossIntroIDs = PlayedBossIntroIDs;

	SaveGameObject->bHasPlayedTownIntro = bHasPlayedTownIntro;
	SaveGameObject->SeenTutorialIDs = SeenTutorialIDs;

	SaveGameObject->ConsumedWorldDropIDs = ConsumedWorldDropIDs;
}

void UFortressGameInstance::CopyFromSaveGame(const UFortressSaveGame* SaveGameObject)
{
	if (!SaveGameObject)
	{
		return;
	}

	SavedLevelName = SaveGameObject->SavedLevelName;

	bHasCheckpoint = SaveGameObject->bHasCheckpoint;
	LastCheckpointLevelName = SaveGameObject->LastCheckpointLevelName;
	LastCheckpointTransform = SaveGameObject->LastCheckpointTransform;

	ActivatedBonfires = SaveGameObject->ActivatedBonfires;

	CurrentMemoryShards = SaveGameObject->CurrentMemoryShards;
	PendingMemoryShardDrop = SaveGameObject->PendingMemoryShardDrop;

	SavedInventoryItems = SaveGameObject->SavedInventoryItems;
	SavedEquippedWeapon = SaveGameObject->SavedEquippedWeapon;
	SavedEquippedHelmet = SaveGameObject->SavedEquippedHelmet;
	SavedEquippedArmor = SaveGameObject->SavedEquippedArmor;
	SavedEquippedGloves = SaveGameObject->SavedEquippedGloves;
	SavedEquippedBoots = SaveGameObject->SavedEquippedBoots;

	SavedEnhancementData = SaveGameObject->SavedEnhancementData;

	OpenedDoorIDs = SaveGameObject->OpenedDoorIDs;
	ConsumedChestIDs = SaveGameObject->ConsumedChestIDs;
	DefeatedBossIDs = SaveGameObject->DefeatedBossIDs;
	PlayedBossIntroIDs = SaveGameObject->PlayedBossIntroIDs;

	bHasPlayedTownIntro = SaveGameObject->bHasPlayedTownIntro;
	SeenTutorialIDs = SaveGameObject->SeenTutorialIDs;

	ConsumedWorldDropIDs = SaveGameObject->ConsumedWorldDropIDs;
}