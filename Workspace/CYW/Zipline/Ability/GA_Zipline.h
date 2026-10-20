#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Common/CharacterMovementStateGuard.h"
#include "Components/SkinnedMeshComponent.h"
#include "Zipline/ZiplineTypes.h"
#include "GA_Zipline.generated.h"

class AZiplineActor;
class ACharacter;
class APlayerCharacter;
class UAbilityTask_ZiplineMove;

UCLASS()
class TEAMPROJECT_DL_API UGA_Zipline : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Zipline();

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

private:
	void OnRequestedZipline(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ApplicationTag);
	void FinishInteraction(bool bWasCancelled);
	void BeginInteraction(const FZiplineInteractionResult& Result);
	FDelegateHandle TargetDataDelegateHandle;
	UFUNCTION()
	void OnMountCompleted(FGameplayEventData Payload);
	UFUNCTION()
	void OnMountMontageCompleted();
	UFUNCTION()
	void OnMountMontageInterrupted();
	UFUNCTION()
	void OnZiplineMoveFinished();
	UFUNCTION()
	void OnZiplineMoveFailed();
	void PrepareRiderForZipline();
	bool StartZiplineMove();
	bool WaitMountEvent();
	bool StartMountMontage();
	void RestoreRiderState();

	UPROPERTY() TObjectPtr<AZiplineActor> ActiveZipline;
	UPROPERTY() TObjectPtr<APlayerCharacter> Rider;
	UPROPERTY() TObjectPtr<UAbilityTask_ZiplineMove> ZiplineMoveTask;
	FZiplineInteractionResult ActiveInteractionResult;
	FCharacterMovementStateGuard MovementStateGuard;
	bool bRiding = false;
	bool bEnding = false;
	bool bServerAnimTickOptionCached = false;
	EVisibilityBasedAnimTickOption CachedServerAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
};
