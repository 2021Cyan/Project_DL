// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GameplayEffectTypes.h"
#include "GA_LugguageCart.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct FLugguageCartLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float Damage = 80.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float SpiritPowerScale = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float WallImpactDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float WallImpactSpiritPowerScale = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stun", meta = (ClampMin = "0.0"))
	float StunDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "0.0"))
	float MaxTravelDistance = 4500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (ClampMin = "0.0"))
	float CooldownDuration = 3.f;
};

/**
 *
 */
UCLASS()
class TEAMPROJECT_DL_API UGA_LugguageCart : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_LugguageCart();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void OnSkillEvent_Implementation(FGameplayTag EventTag, FGameplayEventData Payload) override;
	virtual float GetEffectiveCooldownDuration() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|Event")
	FGameplayTag ExecuteEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|FX")
	FGameplayTag SpawnGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Lugguage Cart")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lugguage Cart")
	TSubclassOf<class ALugguageCartActor> CartClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lugguage Cart")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Lugguage Cart", meta = (ClampMin = "0.0"))
	float SpawnForwardOffset = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lugguage Cart", meta = (ClampMin = "0.0"))
	float SpawnUpOffset = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|Level")
	TArray<FLugguageCartLevelData> LevelData;

private:
	void SpawnCart();
	FVector GetSpawnLocation(AActor* AvatarActor, const FVector& TravelDirection) const;
	FVector BuildTravelDirection(AActor* AvatarActor) const;
	void ExecuteSpawnGameplayCue(AActor* SpawnedActor, const FVector& CueNormal) const;
	const FLugguageCartLevelData& GetCurrentLevelData() const;
	FGameplayEffectSpecHandle MakeStunSpec(const FLugguageCartLevelData& Data) const;

	bool bHasExecuted = false;
};
