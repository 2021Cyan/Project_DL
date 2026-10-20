#include "GA_Zipline.h"
#include "Common/AbilityTags.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Zipline/ZiplineActor.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Zipline/Ability/AbilityTask_ZiplineMove.h"
#include "Zipline/ZiplineInteractionLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameCore/Character/PlayerCharacter.h"

UGA_Zipline::UGA_Zipline()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Movement.Zipline"), false);
	const FGameplayTag StateTag = FGameplayTag::RequestGameplayTag(TEXT("State.Movement.Zipline"), false);
	const FGameplayTag SkillActiveTag = FGameplayTag::RequestGameplayTag(TEXT("State.Skill.Active"), false);

	if (AbilityTag.IsValid())
	{
		AssetTags.AddTag(AbilityTag);
	}

	SetAssetTags(AssetTags);

	if (StateTag.IsValid())
	{
		ActivationOwnedTags.AddTag(StateTag);
	}

	if (SkillActiveTag.IsValid())
	{
		ActivationOwnedTags.AddTag(SkillActiveTag);
	}

	const FGameplayTag MovementAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Movement"), false);
	const FGameplayTag AbilityRootTag = FGameplayTag::RequestGameplayTag(TEXT("Ability"), false);

	if (AbilityRootTag.IsValid())
	{
		BlockAbilitiesWithTag.AddTag(AbilityRootTag);
	}

	if (MovementAbilityTag.IsValid())
	{
		CancelAbilitiesWithTag.AddTag(MovementAbilityTag);
	}
}

void UGA_Zipline::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	bEnding = false;
	bRiding = false;
	bServerAnimTickOptionCached = false;
	ZiplineMoveTask = nullptr;
	Rider = ActorInfo ? Cast<APlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	ActiveInteractionResult = FZiplineInteractionResult();
	ActiveZipline = nullptr;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayTag MountedTag = DLAbilityTags::ZiplineMounted();
	if (!Rider || !ASC || !Rider->ZiplineMontage || !MountedTag.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// Remote authority validates the actor requested by the predicting client.
	if (ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled())
	{
		TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle,
			ActivationInfo.GetActivationPredictionKey()).AddUObject(this, &UGA_Zipline::OnRequestedZipline);
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
		return;
	}
	FZiplineInteractionResult Result;
	if (!FZiplineInteractionLibrary::FindBestZipline(Rider, Result))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!ActorInfo->IsNetAuthority())
	{
		FGameplayAbilityTargetDataHandle Data;
		FGameplayAbilityTargetData_ActorArray* Actors = new FGameplayAbilityTargetData_ActorArray();
		Actors->TargetActorArray.Add(Result.Zipline);
		Data.Add(Actors);
		ASC->CallServerSetReplicatedTargetData(Handle, ActivationInfo.GetActivationPredictionKey(),
			Data, FGameplayTag(), ASC->ScopedPredictionKey);
	}
	BeginInteraction(Result);
}

void UGA_Zipline::OnRequestedZipline(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ApplicationTag)
{
	if (bEnding || !CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid()) return;
	const TArray<TWeakObjectPtr<AActor>> Actors = Data.Num() == 1 && Data.Get(0)
		? Data.Get(0)->GetActors() : TArray<TWeakObjectPtr<AActor>>();
	AZiplineActor* Requested = Actors.Num() == 1 ? Cast<AZiplineActor>(Actors[0].Get()) : nullptr;
	CurrentActorInfo->AbilitySystemComponent->ConsumeClientReplicatedTargetData(
		CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	FZiplineInteractionResult Result;
	if (!FZiplineInteractionLibrary::BuildInteractionResult(Rider, Requested, Result))
	{
		FinishInteraction(true);
		return;
	}
	BeginInteraction(Result);
}

void UGA_Zipline::BeginInteraction(const FZiplineInteractionResult& Result)
{
	if (bEnding || ActiveZipline) return;
	ActiveInteractionResult = Result;
	ActiveZipline = Result.Zipline;
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		FinishInteraction(true);
		return;
	}
	PrepareRiderForZipline();
	if (!WaitMountEvent() || bEnding || !StartZiplineMove() || bEnding || !StartMountMontage())
	{
		if (!bEnding) FinishInteraction(true);
	}
}

void UGA_Zipline::PrepareRiderForZipline()
{
	if (!Rider)
	{
		return;
	}

	if (Rider->HasAuthority() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		if (USkeletalMeshComponent* Mesh = Rider->GetMesh())
		{
			CachedServerAnimTickOption = Mesh->VisibilityBasedAnimTickOption;
			bServerAnimTickOptionCached = true;
			Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		}
	}

	MovementStateGuard.Capture(Rider);
	if (UCharacterMovementComponent* Movement = Rider->GetCharacterMovement())
	{
		Movement->DisableMovement();
	}
}

bool UGA_Zipline::StartZiplineMove()
{
	ZiplineMoveTask = UAbilityTask_ZiplineMove::MoveAlongZipline(
		this,
		Rider,
		ActiveZipline,
		ActiveInteractionResult
	);

	if (!ZiplineMoveTask)
	{
		return false;
	}

	ZiplineMoveTask->OnFinished.AddDynamic(this, &UGA_Zipline::OnZiplineMoveFinished);
	ZiplineMoveTask->OnFailed.AddDynamic(this, &UGA_Zipline::OnZiplineMoveFailed);
	ZiplineMoveTask->ReadyForActivation();
	return !bEnding && IsActive();
}

bool UGA_Zipline::WaitMountEvent()
{
	const FGameplayTag MountedEventTag = DLAbilityTags::ZiplineMounted();
	if (!MountedEventTag.IsValid())
	{
		return false;
	}

	UAbilityTask_WaitGameplayEvent* EventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			MountedEventTag,
			nullptr,
			true,
			true
		);

	if (!EventTask)
	{
		return false;
	}

	EventTask->EventReceived.AddDynamic(this, &UGA_Zipline::OnMountCompleted);
	EventTask->ReadyForActivation();
	return !bEnding && IsActive();
}

bool UGA_Zipline::StartMountMontage()
{
	if (!Rider || !Rider->ZiplineMontage)
	{
		return false;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			TEXT("Zipline"),
			Rider->ZiplineMontage,
			1.0f
		);

	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UGA_Zipline::OnMountMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Zipline::OnMountMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Zipline::OnMountMontageInterrupted);
	MontageTask->ReadyForActivation();
	return !bEnding && IsActive();
}

void UGA_Zipline::OnMountMontageCompleted()
{
	if (bRiding)
	{
		return;
	}

	if (!bEnding)
	{
		FinishInteraction(true);
	}
}

void UGA_Zipline::OnMountCompleted(FGameplayEventData Payload)
{
	if (bEnding || bRiding || !Rider || !ActiveZipline || !ZiplineMoveTask)
	{
		return;
	}

	ZiplineMoveTask->StartRiding();
	if (!bEnding && ZiplineMoveTask) bRiding = ZiplineMoveTask->IsRiding();
}

void UGA_Zipline::OnZiplineMoveFinished()
{
	FinishInteraction(false);
}

void UGA_Zipline::OnZiplineMoveFailed()
{
	FinishInteraction(true);
}

void UGA_Zipline::OnMountMontageInterrupted()
{
	if (!bRiding && !bEnding)
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

void UGA_Zipline::FinishInteraction(bool bWasCancelled)
{
	if (!bEnding)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
	}
}

void UGA_Zipline::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (bEnding)
	{
		return;
	}

	bEnding = true;
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && TargetDataDelegateHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->AbilityTargetDataSetDelegate(Handle,
			ActivationInfo.GetActivationPredictionKey()).Remove(TargetDataDelegateHandle);
		TargetDataDelegateHandle.Reset();
	}

	RestoreRiderState();

	ActiveZipline = nullptr;
	ActiveInteractionResult = FZiplineInteractionResult();
	Rider = nullptr;
	ZiplineMoveTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Zipline::RestoreRiderState()
{
	if (!Rider)
	{
		return;
	}

	if (bServerAnimTickOptionCached)
	{
		if (USkeletalMeshComponent* Mesh = Rider->GetMesh())
		{
			Mesh->VisibilityBasedAnimTickOption = CachedServerAnimTickOption;
		}

		bServerAnimTickOptionCached = false;
	}

	if (Rider->ZiplineMontage)
	{
		Rider->StopAnimMontage(Rider->ZiplineMontage);
	}

	MovementStateGuard.Restore(Rider);
	if (UCharacterMovementComponent* Movement = Rider->GetCharacterMovement(); Movement && bRiding)
	{
		Movement->SetMovementMode(MOVE_Falling);
	}
}
