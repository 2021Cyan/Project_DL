// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Parry.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Components/SceneComponent.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameCore/Debug/DLDebug.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

UGA_Parry::UGA_Parry()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	const FGameplayTag ParryTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.Parry")), false);
	ParryCueTag = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayCue.Skill.Parry.Spawn")), false);

	if (ParryTag.IsValid())
	{
		AssetTags.AddTag(ParryTag);
	}

	SetAssetTags(AssetTags);
}

void UGA_Parry::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	bIsEnding = false;
	bParryStateActive = false;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		DLDebug::Screen(TEXT("[Parry] GA_Parry failed: CommitAbility false"), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	if (!Character || !Character->ParryMontage)
	{
		DLDebug::Screen(FString::Printf(TEXT("[Parry] GA_Parry failed Character=%s Montage=%s"),
			*GetNameSafe(Character),
			Character ? *GetNameSafe(Character->ParryMontage.Get()) : TEXT("null")), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FGameplayTag ParryEndEventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.Parry.End")), false);
	if (!ParryEndEventTag.IsValid())
	{
		DLDebug::Screen(TEXT("[Parry] GA_Parry failed: Event.Parry.End tag invalid"), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* ParryEndTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			ParryEndEventTag,
			nullptr,
			true,
			true
		);

	if (!ParryEndTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ParryEndTask->EventReceived.AddDynamic(this, &ThisClass::OnParryEndEvent);
	ParryEndTask->ReadyForActivation();

	SetParryState(true);

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		Character->ParryMontage.Get()
	);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_Parry::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bIsEnding)
	{
		return;
	}

	bIsEnding = true;
	SetParryState(false);
	SetParryMovementLocked(false);

	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (Character && Character->ParryMontage && bWasCancelled)
	{
		Character->StopAnimMontage(Character->ParryMontage.Get());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Parry::OnMontageCompleted()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Parry::OnMontageBlendOut()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Parry::OnMontageCancelled()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Parry::OnParryEndEvent(FGameplayEventData Payload)
{
	(void)Payload;

	SetParryState(false);
}

void UGA_Parry::AddParryCue()
{
	if (bParryCueActive)
	{
		return;
	}

	if (!ParryCueTag.IsValid())
	{
		DLDebug::Screen(TEXT("[Parry] GameplayCue.Skill.Parry.Spawn tag invalid"), FColor::Red);
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* AvatarActor = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	if (!ASC || !AvatarActor)
	{
		return;
	}

	USceneComponent* AttachComponent = AvatarActor->GetRootComponent();

	FGameplayCueParameters CueParameters;
	CueParameters.Instigator = AvatarActor;
	CueParameters.EffectCauser = AvatarActor;
	CueParameters.SourceObject = this;
	CueParameters.Location = AttachComponent ? AttachComponent->GetComponentLocation() : AvatarActor->GetActorLocation();
	CueParameters.Normal = AvatarActor->GetActorForwardVector();
	CueParameters.TargetAttachComponent = AttachComponent;

	ASC->AddGameplayCue(ParryCueTag, CueParameters);
	bParryCueActive = true;
}

void UGA_Parry::RemoveParryCue()
{
	if (!bParryCueActive || !ParryCueTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return;
	}

	ASC->RemoveGameplayCue(ParryCueTag);
	bParryCueActive = false;
}

void UGA_Parry::SetParryState(bool bShouldParry)
{
	if (bParryStateActive == bShouldParry)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return;
	}

	const FGameplayTag ParryingTag = FGameplayTag::RequestGameplayTag(FName(TEXT("State.Combat.Parrying")), false);
	if (!ParryingTag.IsValid())
	{
		DLDebug::Screen(TEXT("[Parry] State.Combat.Parrying tag invalid"), FColor::Red);
		return;
	}

	ASC->SetLooseGameplayTagCount(ParryingTag, bShouldParry ? 1 : 0);
	SetParryMovementLocked(bShouldParry);
	if (bShouldParry)
	{
		AddParryCue();
	}
	else
	{
		RemoveParryCue();
	}
	bParryStateActive = bShouldParry;

	DLDebug::Screen(
		bShouldParry
			? TEXT("[Parry] State.Combat.Parrying ON: can stun melee attacker")
			: TEXT("[Parry] State.Combat.Parrying OFF"),
		bShouldParry ? FColor::Green : FColor::Yellow
	);
}

void UGA_Parry::SetParryMovementLocked(bool bShouldLock)
{
	ACharacter* Character = CurrentActorInfo ? Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	if (bShouldLock)
	{
		if (bMovementLockedByParry)
		{
			return;
		}

		ParryMovementState.Capture(Character);
		const bool bWasFalling = Movement->IsFalling();
		const float SavedVerticalVelocity = Movement->Velocity.Z;

		if (bWasFalling)
		{
			Movement->Velocity = FVector(0.f, 0.f, SavedVerticalVelocity);
			Movement->SetMovementMode(MOVE_Falling);
			bMovementDisabledByParry = false;
		}
		else
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
			bMovementDisabledByParry = true;
		}

		bMovementLockedByParry = true;
		return;
	}

	if (!bMovementLockedByParry)
	{
		return;
	}

	if (bMovementDisabledByParry)
	{
		ParryMovementState.Restore(Character);
	}
	else
	{
		ParryMovementState.Reset();
	}

	bMovementDisabledByParry = false;
	bMovementLockedByParry = false;
}
