// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GA_LightMelee.generated.h"

/**
 *
 */
UCLASS()
class TEAMPROJECT_DL_API UGA_LightMelee : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_LightMelee();
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
	void OnHitEvent(FGameplayEventData Payload);

	void PerformMeleeHit();
	void ApplyMeleeDamage(class ABaseCharacter* SourceCharacter, class AActor* TargetActor) const;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	FName EnemyHitCollisionProfileName = TEXT("EnemyHit");

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	float InitialCheckRadius = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	float InitialCheckForwardOffset = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	float ConeRange = 180.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	float ConeHalfAngleDegrees = 35.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Damage", meta = (ClampMin = "0.0"))
	float DamageCoefficient = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Debug")
	bool bShowDebug = false;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Debug", meta = (EditCondition = "bShowDebug"))
	float DebugDrawDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Debug", meta = (EditCondition = "bShowDebug"))
	float DebugDrawThickness = 2.0f;
};
