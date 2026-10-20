#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_HeavyMeleeLunge.generated.h"

class ABaseCharacter;
class UCurveFloat;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHeavyMeleeLungeFinishedDelegate);

UCLASS()
class TEAMPROJECT_DL_API UAbilityTask_HeavyMeleeLunge : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FHeavyMeleeLungeFinishedDelegate OnFinish;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (
		DisplayName = "Apply Heavy Melee Lunge",
		HidePin = "OwningAbility",
		DefaultToSelf = "OwningAbility",
		BlueprintInternalUseOnly = "true"
		))
	static UAbilityTask_HeavyMeleeLunge* ApplyHeavyMeleeLunge(
		UGameplayAbility* OwningAbility,
		float Duration,
		float MaxSpeed,
		UCurveFloat* SpeedCurve,
		float DirectionSteeringRate,
		bool bStopHorizontalVelocityOnEnd
	);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	void CacheMovementState();
	void ApplyLungeMovementState();
	void RestoreMovementState();
	FVector GetCameraForwardDirection() const;
	float GetSpeedAlpha() const;
	void UpdateLungeForce(float DeltaTime);
	void UpdateCharacterRotation(float DeltaTime) const;
	void FinishLunge();
	void RemoveRootMotionSource();

private:
	UPROPERTY()
	TObjectPtr<UCurveFloat> SpeedCurve;

	TWeakObjectPtr<ABaseCharacter> Character;
	TWeakObjectPtr<UCharacterMovementComponent> MovementComponent;

	TSharedPtr<struct FRootMotionSource_ConstantForce> RootMotionSource;
	uint16 RootMotionSourceID = 0;
	FVector CurrentDirection = FVector::ForwardVector;
	FRotator CachedRotationRate = FRotator::ZeroRotator;
	float Duration = 0.f;
	float MaxSpeed = 0.f;
	float DirectionSteeringRate = 12.f;
	float ElapsedTime = 0.f;
	bool bCachedOrientRotationToMovement = false;
	bool bCachedUseControllerDesiredRotation = false;
	bool bStopHorizontalVelocityOnEnd = true;
	bool bMovementStateCached = false;
	bool bFinished = false;
};
