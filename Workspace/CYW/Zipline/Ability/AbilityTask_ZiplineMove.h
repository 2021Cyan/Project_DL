#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Zipline/ZiplineTypes.h"
#include "AbilityTask_ZiplineMove.generated.h"

class ABaseCharacter;
class AZiplineActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZiplineMoveDelegate);

enum class EZiplineMoveState : uint8
{
	MountApproach,
	Riding,
	Finished
};

UCLASS()
class TEAMPROJECT_DL_API UAbilityTask_ZiplineMove : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FZiplineMoveDelegate OnFinished;

	UPROPERTY(BlueprintAssignable)
	FZiplineMoveDelegate OnFailed;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (
		DisplayName = "Move Along Zipline",
		HidePin = "OwningAbility",
		DefaultToSelf = "OwningAbility",
		BlueprintInternalUseOnly = "true"
		))
	static UAbilityTask_ZiplineMove* MoveAlongZipline(
		UGameplayAbility* OwningAbility,
		ABaseCharacter* Character,
		AZiplineActor* Zipline,
		const FZiplineInteractionResult& InteractionResult
	);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;

	void StartRiding();
	bool IsRiding() const { return MoveState == EZiplineMoveState::Riding; }

private:
	bool ShouldUpdateCharacter() const;
	FVector GetRideLocation(float InDistanceAlongSpline) const;
	FRotator GetRideRotation(float InDistanceAlongSpline) const;
	void UpdateMountApproach(float DeltaTime);
	void UpdateRide(float DeltaTime);
	void FinishTask(bool bSucceeded);

	TWeakObjectPtr<ABaseCharacter> Character;
	TWeakObjectPtr<AZiplineActor> Zipline;
	FZiplineRideParams RideParams;
	float DistanceAlongSpline = 0.0f;
	float RideDirection = 1.0f;
	FVector ApproachStartLocation = FVector::ZeroVector;
	FQuat ApproachStartRotation = FQuat::Identity;
	float ApproachElapsedTime = 0.0f;
	EZiplineMoveState MoveState = EZiplineMoveState::MountApproach;
};
