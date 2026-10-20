// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_Doorway.generated.h"

class ADoorwayActor;

USTRUCT(BlueprintType)
struct FDoorwayLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (ClampMin = "0.0"))
	float CooldownDuration = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Doorway", meta = (ClampMin = "0.0"))
	float DoorLifetime = 15.f;
};

UCLASS()
class TEAMPROJECT_DL_API UGA_Doorway : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_Doorway();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
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
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual float GetEffectiveCooldownDuration() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|Event")
	FGameplayTag ExecuteEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|FX")
	FGameplayTag SpawnGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Doorway|Spawn")
	TSubclassOf<ADoorwayActor> DoorwayActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Doorway|Spawn")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Doorway|Spawn", meta = (ClampMin = "0.0"))
	float WallClearanceOffset = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|Level")
	TArray<FDoorwayLevelData> LevelData;

private:
	enum class EDoorwayPlacementStep : uint8
	{
		Entrance,
		Exit
	};

	void ExecuteDoorway();
	void SpawnEntrance();
	void SpawnExit();
	void ClearDoorwayPair();
	void ClearDoorwayTimers();
	void ScheduleDoorwayExpiration();
	void OnDoorwayExpired();
	ADoorwayActor* SpawnDoorwayActor(const FTransform& SpawnTransform, bool bIsEntrance) const;
	FTransform BuildDoorwaySpawnTransform(const FTransform& PreviewTransform) const;
	void ExecuteSpawnGameplayCue(ADoorwayActor* SpawnedDoorway) const;
	const FDoorwayLevelData& GetCurrentLevelData() const;

	UPROPERTY(Replicated) TObjectPtr<ADoorwayActor> EntranceDoorway;
	UPROPERTY(Replicated) TObjectPtr<ADoorwayActor> ExitDoorway;
	FTimerHandle DoorwayLifetimeHandle;
	EDoorwayPlacementStep PlacementStep = EDoorwayPlacementStep::Entrance;
	bool bHasExecuted = false;
};
