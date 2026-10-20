// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GA_SkillBase.h"
#include "GA_HotelGuest.generated.h"

class AHotelGuestLocationActor;
class ABasePlayerController;
class APlayerCharacter;

USTRUCT(BlueprintType)
struct FHotelGuestLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", DisplayName = "Base Damage (Always)"))
	float SuccessDamage = 75.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", DisplayName = "Base Spirit Power Scale (Always)"))
	float SuccessSpiritPowerScale = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", DisplayName = "Failure Bonus Damage"))
	float FailureDamage = 125.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", DisplayName = "Failure Bonus Spirit Power Scale"))
	float FailureSpiritPowerScale = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Penalty", meta = (ClampMin = "0.0"))
	float StaminaDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Timing", meta = (ClampMin = "0.1"))
	float EscapeDuration = 6.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (ClampMin = "0.0"))
	float CooldownDuration = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	bool bOverrideCooldownAfterFailure = false;

	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (ClampMin = "0.0", EditCondition = "bOverrideCooldownAfterFailure"))
	float FailureCooldownDuration = 15.f;
};

/**
 *
 */
UCLASS()
class TEAMPROJECT_DL_API UGA_HotelGuest : public UGA_SkillBase
{
	GENERATED_BODY()

public:
	UGA_HotelGuest();
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
	virtual void OnSkillEvent_Implementation(FGameplayTag EventTag, FGameplayEventData Payload) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual float GetEffectiveCooldownDuration() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|Event")
	FGameplayTag ExecuteEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Hotel Guest|Location")
	TSubclassOf<AHotelGuestLocationActor> LocationActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Hotel Guest|Location", meta = (ClampMin = "0.0"))
	float EscapeRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hotel Guest|Location", meta = (ClampMin = "0.0"))
	float FallResetDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hotel Guest|Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CameraFadeAlpha = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Hotel Guest|Camera", meta = (ClampMin = "0.0"))
	float CameraFadeInDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Hotel Guest|Camera", meta = (ClampMin = "0.0"))
	float CameraFadeOutDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Doorman|Level")
	TArray<FHotelGuestLevelData> LevelData;

private:
	enum class EHotelGuestState : uint8
	{
		Targeting,
		WaitingForEvent,
		InHotel,
		Resolved
	};

	enum class EHotelGuestResult : uint8
	{
		Escaped,
		Failed,
		Cancelled
	};

	AHotelGuestLocationActor* FindLocationActor() const;
	void EnterHotel();
	void TickHotelStay();
	void OnHotelStayExpired();
	bool IsTargetInsideEscapeRadius() const;
	void FinishHotelStay(EHotelGuestResult Result);
	void CleanupHotelStay(bool bRestorePosition);
	bool bEnding = false;
	FHotelGuestLevelData ActiveStayLevelData;
	void ApplyStaminaDamage(const FHotelGuestLevelData& Data);
	void StartTargetCameraDarken() const;
	void StopTargetCameraDarken() const;
	ABasePlayerController* GetHotelTargetController() const;
	void OverrideCooldownAfterFailure();
	const FHotelGuestLevelData& GetCurrentLevelData() const;

	UPROPERTY() TObjectPtr<APlayerCharacter> HotelTarget;
	UPROPERTY() TObjectPtr<AHotelGuestLocationActor> HotelLocationActor;
	FTransform OriginalTransform = FTransform::Identity;
	FRotator OriginalControlRotation = FRotator::ZeroRotator;
	FTimerHandle StayTickHandle;
	FTimerHandle FailureHandle;
	EHotelGuestState HotelState = EHotelGuestState::Resolved;
	bool bUseFailureCooldown = false;
};
