// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Common/CharacterMovementStateGuard.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTagContainer.h"
#include "GA_Parry.generated.h"

/**
 *
 */
UCLASS()
class TEAMPROJECT_DL_API UGA_Parry : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Parry();
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnParryEndEvent(FGameplayEventData Payload);

	void AddParryCue();
	void RemoveParryCue();
	void SetParryMovementLocked(bool bShouldLock);
	void SetParryState(bool bShouldParry);

	UPROPERTY(EditDefaultsOnly, Category = "Parry|Cue")
	FGameplayTag ParryCueTag;

	FCharacterMovementStateGuard ParryMovementState;
	bool bIsEnding = false;
	bool bMovementDisabledByParry = false;
	bool bMovementLockedByParry = false;
	bool bParryCueActive = false;
	bool bParryStateActive = false;
};
