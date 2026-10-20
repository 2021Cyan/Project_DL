// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Common/PlayerViewSupport.h"
#include "GA_SkillBase.h"
#include "GameplayEffectTypes.h"
#include "GA_CallBell.generated.h"

class ACallBellProjectile;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FCallBellLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float ImpactDamage = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float ImpactSpiritPowerScale = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float ExplosionDamage = 55.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float ExplosionSpiritPowerScale = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Area", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing", meta = (ClampMin = "0.0"))
	float BellLifetime = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Debuff", meta = (ClampMin = "0.0"))
	float DebuffDuration = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Debuff")
	float WeaponAccuracyMultiplier = -0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Debuff")
	float SlowPercent = -0.35f;
};

UCLASS()
class TEAMPROJECT_DL_API UGA_CallBell : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_CallBell();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void OnCastStarted_Implementation() override;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell")
	TSubclassOf<ACallBellProjectile> CallBellClass;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell")
	TSubclassOf<UGameplayEffect> DazedEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Call Bell|Targeting", meta = (ClampMin = "0.0"))
	float CameraRaySpawnDistance = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Call Bell|Targeting", meta = (ClampMin = "1.0"))
	float TraceDistance = 100000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|Level")
	TArray<FCallBellLevelData> LevelData;

private:
	void SpawnCallBell();
	FVector GetSpawnLocation(const DLPlayerView::FAimView& View) const;
	FVector BuildLaunchDirection(const FVector& SpawnLocation, const DLPlayerView::FAimView& View) const;
	const FCallBellLevelData& GetCurrentLevelData() const;
	FGameplayEffectSpecHandle MakeEffectSpec(TSubclassOf<UGameplayEffect> EffectClass) const;
	FGameplayEffectSpecHandle MakeDebuffSpec(TSubclassOf<UGameplayEffect> EffectClass,
		FName MagnitudeTagName, float Magnitude, float Duration) const;
	TArray<FGameplayEffectSpecHandle> MakeStatusEffectSpecs() const;

	bool bHasExecuted = false;
};
