// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Common/CharacterMovementStateGuard.h"
#include "GA_HeavyMelee.generated.h"

/**
 *
 */
UCLASS()
class TEAMPROJECT_DL_API UGA_HeavyMelee : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_HeavyMelee();
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

	UFUNCTION()
	void OnLungeEvent(FGameplayEventData Payload);

	void ApplyLungeMovementState(class ABaseCharacter* Character);
	void RestoreLungeMovementState();
	void StartLunge(class ABaseCharacter* Character);
	void StopLunge();
	void SetLungeState(bool bShouldLunge);
	void PerformMeleeHit();
	void ApplyMeleeDamage(class ABaseCharacter* SourceCharacter, class AActor* TargetActor) const;

	UFUNCTION()
	void OnLungeFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	FName EnemyHitCollisionProfileName = TEXT("EnemyHit");

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	float InitialCheckRadius = 110.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	float InitialCheckForwardOffset = 110.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	float ConeRange = 260.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Hit")
	float ConeHalfAngleDegrees = 45.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Damage", meta = (ClampMin = "0.0"))
	float DamageCoefficient = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Lunge")
	float LungeDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Lunge")
	float LungeSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Lunge")
	TObjectPtr<class UCurveFloat> LungeSpeedCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Lunge")
	float LungeSteeringRate = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Lunge")
	bool bStopHorizontalVelocityOnLungeEnd = true;

	FCharacterMovementStateGuard LungeMovementState;
	bool bLungeMovementStateApplied = false;
	bool bLungeGravityScaleApplied = false;
	bool bLungeMovementModeDisabled = false;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Debug")
	bool bShowDebug = false;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Debug", meta = (EditCondition = "bShowDebug"))
	float DebugDrawDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Melee|Debug", meta = (EditCondition = "bShowDebug"))
	float DebugDrawThickness = 2.0f;
};
