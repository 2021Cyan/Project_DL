// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Reload.h"
#include "Common/AbilityTags.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameCore/Debug/DLDebug.h"
#include "Weapon/WeaponComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

UGA_Reload::UGA_Reload()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	const FGameplayTag ReloadTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.Reload")), false);

	if (ReloadTag.IsValid())
	{
		AssetTags.AddTag(ReloadTag);
		ActivationOwnedTags.AddTag(ReloadTag);
	}

	SetAssetTags(AssetTags);
}

void UGA_Reload::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bIsEnding = false;
	bReloadApplied = false;

	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWeaponComponent* Weapon = Character->GetWeaponComponent();
	if (!Weapon || !Weapon->CanReload())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!Character->ReloadMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	if (!AnimInstance)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* Montage = Character->ReloadMontage.Get();


	const FGameplayTag ReloadCompleteEventTag = DLAbilityTags::ReloadComplete();
	if (!ReloadCompleteEventTag.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}


	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	DLDebug::Screen(FString::Printf(TEXT("[Reload] Started. Ammo=%d/%d"), Weapon->GetCurrentAmmo(), Weapon->GetMagazineSize()), FColor::Cyan);


	UAbilityTask_WaitGameplayEvent* ReloadNotifyTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			ReloadCompleteEventTag,
			nullptr,
			true,
			true
		);

	if (!ReloadNotifyTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ReloadNotifyTask->EventReceived.AddDynamic(this, &UGA_Reload::OnReloadNotifyReceived);
	ReloadNotifyTask->ReadyForActivation();
	if (bIsEnding || !IsActive()) return;

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			Montage,
			1.0f
		);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UGA_Reload::OnReloadMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Reload::OnReloadMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Reload::OnReloadMontageInterrupted);

	MontageTask->ReadyForActivation();
}

void UGA_Reload::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (bIsEnding)
	{
		return;
	}

	bIsEnding = true;

	ABaseCharacter* Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	if (Character && Character->ReloadMontage && bWasCancelled)
	{
		Character->StopAnimMontage(Character->ReloadMontage.Get());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Reload::OnReloadMontageCompleted()
{
	if (!bReloadApplied && HasAuthority(&CurrentActivationInfo))
		UE_LOG(LogTemp, Warning, TEXT("%s: Reload montage ended without Event.Reload.Complete"), *GetName());
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Reload::OnReloadMontageInterrupted()
{
	DLDebug::Screen(TEXT("[Reload] Cancelled"), FColor::Yellow);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Reload::OnReloadNotifyReceived(FGameplayEventData Payload)
{
	if (bIsEnding || bReloadApplied)
	{
		return;
	}

	ABaseCharacter* Character =
		CurrentActorInfo ? Cast<ABaseCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;

	UWeaponComponent* Weapon = Character ? Character->GetWeaponComponent() : nullptr;
	if (!Character || !Weapon)
	{
		return;
	}

	if (!Character->HasAuthority())
	{
		return;
	}

	bReloadApplied = true;
	Weapon->ReloadAmmo();
}
