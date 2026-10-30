#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Animation/AnimMontage.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	Consumable	UMETA(DisplayName = "Consumable"),
	Equipment	UMETA(DisplayName = "Equipment"),
	Material	UMETA(DisplayName = "Material"),
	Quest		UMETA(DisplayName = "Quest")
};

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None	UMETA(DisplayName = "None"),
	Weapon	UMETA(DisplayName = "Weapon"),
	Helmet	UMETA(DisplayName = "Helmet"),
	Armor	UMETA(DisplayName = "Armor"),
	Gloves	UMETA(DisplayName = "Gloves"),
	Boots	UMETA(DisplayName = "Boots")
};

UENUM(BlueprintType)
enum class EWeaponEffectType : uint8
{
	None		UMETA(DisplayName = "None"),
	Niagara		UMETA(DisplayName = "Niagara"),
	Particle	UMETA(DisplayName = "Particle")
};

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemType ItemType = EItemType::Consumable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EEquipmentSlot EquipSlot = EEquipmentSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bStackable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 MaxStackCount = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 MaxHPBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 AttackBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 MaxStaminaBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 HealAmount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UTexture2D> ItemIcon = nullptr;

	// ===== Weapon Visual =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Visual")
	TObjectPtr<UStaticMesh> WeaponStaticMesh = nullptr;

	// ===== Weapon Effect =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effect")
	EWeaponEffectType WeaponEffectType = EWeaponEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effect")
	TObjectPtr<UNiagaraSystem> WeaponNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effect")
	TObjectPtr<UParticleSystem> WeaponParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effect")
	FName WeaponEffectSocketName = NAME_None;

	// ===== Weapon Attack Trail =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AttackTrail")
	EWeaponEffectType WeaponAttackTrailEffectType = EWeaponEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AttackTrail")
	TObjectPtr<UNiagaraSystem> WeaponAttackTrailEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AttackTrail")
	TObjectPtr<UParticleSystem> WeaponAttackTrailParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AttackTrail")
	FName WeaponAttackTrailSocketName = TEXT("TraceStart");

	// ===== Weapon Skill Effect =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|SkillEffect")
	EWeaponEffectType WeaponSkillEffectType = EWeaponEffectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|SkillEffect")
	TObjectPtr<UNiagaraSystem> WeaponSkillNiagaraEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|SkillEffect")
	TObjectPtr<UParticleSystem> WeaponSkillParticleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|SkillEffect")
	FName WeaponSkillEffectSocketName = NAME_None;

	// ===== Weapon Skill Setting =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Skill")
	TObjectPtr<UAnimMontage> WeaponSkillMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Skill")
	float WeaponSkillRadius = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Skill")
	float WeaponSkillForwardDistance = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Skill")
	float WeaponSkillMoveDuration = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Skill")
	float WeaponSkillTriggerTimeRatio = 0.5f;

	// ===== Weapon Light =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Light")
	bool bUseWeaponLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Light")
	FLinearColor WeaponLightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Light")
	float WeaponLightIntensity = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Light")
	float WeaponLightRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Light")
	FName WeaponLightSocketName = TEXT("TraceStart");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Light")
	FVector WeaponLightRelativeLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Count = 1;

	bool IsValid() const
	{
		return ItemData.ItemID != NAME_None && Count > 0;
	}
};