// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GA_Reload.generated.h"

/**
 * 
 */
UCLASS()
class TEAMPROJECT_DL_API UGA_Reload : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Reload();
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
	void OnReloadMontageCompleted();

	UFUNCTION()
	void OnReloadMontageInterrupted();

	UFUNCTION()
	void OnReloadNotifyReceived(FGameplayEventData Payload);

private:
	bool bIsEnding = false;
	bool bReloadApplied = false;
};
