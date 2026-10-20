// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_LightMelee.h"
#include "GameCore/Combat/MeleeHitUtils.h"
#include "Common/SkillDamageLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameCore/Debug/DLDebug.h"
#include "GameCore/Struct/ACharacterAttributeSet.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

UGA_LightMelee::UGA_LightMelee()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	const FGameplayTag LMeleeTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.MeleeAttack.Light")), false);

	if (LMeleeTag.IsValid())
	{
		AssetTags.AddTag(LMeleeTag);
	}

	SetAssetTags(AssetTags);
}

void UGA_LightMelee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		DLDebug::Screen(TEXT("[Melee] GA_LightMelee failed: CommitAbility false"), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	if (!Character || !Character->LightMeleeMontage)
	{
		DLDebug::Screen(FString::Printf(TEXT("[Melee] GA_LightMelee failed Character=%s Montage=%s"),
			*GetNameSafe(Character),
			Character ? *GetNameSafe(Character->LightMeleeMontage.Get()) : TEXT("null")), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FGameplayTag HitEventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.Melee.RegisterHit")), false);
	if (HitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* HitEventTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, HitEventTag, nullptr, true, true);
		if (HitEventTask)
		{
			HitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnHitEvent);
			HitEventTask->ReadyForActivation();
		}
	}
	else
	{
		DLDebug::Screen(TEXT("[Melee] Event.Melee.RegisterHit tag invalid"), FColor::Red);
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		Character->LightMeleeMontage.Get()
	);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_LightMelee::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_LightMelee::OnMontageCompleted()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_LightMelee::OnMontageBlendOut()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_LightMelee::OnMontageCancelled()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_LightMelee::OnHitEvent(FGameplayEventData Payload)
{
	(void)Payload;
	PerformMeleeHit();
}

void UGA_LightMelee::PerformMeleeHit()
{
	ABaseCharacter* SourceCharacter = CurrentActorInfo ? Cast<ABaseCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	UAbilitySystemComponent* SourceASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	UWorld* World = SourceCharacter ? SourceCharacter->GetWorld() : nullptr;
	if (!SourceCharacter || !SourceASC || !World || !SourceCharacter->HasAuthority())
	{
		return;
	}

	FMeleeHitQueryParams HitParams;
	HitParams.CollisionProfileName = EnemyHitCollisionProfileName;
	HitParams.InitialCheckRadius = InitialCheckRadius;
	HitParams.InitialCheckForwardOffset = InitialCheckForwardOffset;
	HitParams.ConeRange = ConeRange;
	HitParams.ConeHalfAngleDegrees = ConeHalfAngleDegrees;
	HitParams.bShowDebug = bShowDebug;
	HitParams.DebugDrawDuration = DebugDrawDuration;
	HitParams.DebugDrawThickness = DebugDrawThickness;
	HitParams.DebugColor = FColor::Yellow;
	HitParams.InitialTraceStatName = TEXT("LightMeleeInitialCheck");
	HitParams.ConeTraceStatName = TEXT("LightMeleeConeCheck");
	HitParams.ParryTraceStatName = TEXT("LightMeleeParryCheck");

	if (DLMeleeHitUtils::HasParryingTargetInInitialCheck(*SourceCharacter, HitParams))
	{
		DLDebug::Screen(TEXT("[Parry] Light melee parried: attacker stun trigger point"), FColor::Orange);
		return;
	}

	TArray<AActor*> TargetActors;
	DLMeleeHitUtils::FindTargetsInCone(*SourceCharacter, HitParams, TargetActors);
	for (AActor* TargetActor : TargetActors)
	{
		ApplyMeleeDamage(SourceCharacter, TargetActor);
	}
}

void UGA_LightMelee::ApplyMeleeDamage(ABaseCharacter* SourceCharacter, AActor* TargetActor) const
{
	if (!SourceCharacter || !TargetActor || (DamageCoefficient <= 0.f && BaseDamage <= 0.f))
	{
		return;
	}

	USkillDamageLibrary::ApplyDamage(
		SourceCharacter, TargetActor, DamageCoefficient, BaseDamage, TAG_Data_Damage_Melee);

	// ============================================================================
	// [아이템 시스템 추가 - 근접 적중 패시브]
	// develop의 공용 피해 처리는 유지하고, 피해 적용이 끝난 뒤 아이템 패시브에
	// Light 근접 적중 사실과 계산된 기본 피해량을 전달한다.
	// ============================================================================
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
	const float SourceMeleeDamage = SourceASC
		? SourceASC->GetNumericAttribute(UACharacterAttributeSet::GetMeleeDamageAttribute())
		: 0.f;
	const float EventDamage = SourceMeleeDamage * DamageCoefficient + BaseDamage;
	const FGameplayTag MeleeHitTag =
		FGameplayTag::RequestGameplayTag(TEXT("Event.Item.MeleeHit.Light"));
	FGameplayEventData MeleeHitPayload;
	MeleeHitPayload.EventTag = MeleeHitTag;
	MeleeHitPayload.Instigator = SourceCharacter;
	MeleeHitPayload.Target = TargetActor;
	MeleeHitPayload.EventMagnitude = EventDamage;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		SourceCharacter,
		MeleeHitTag,
		MeleeHitPayload);
	// [아이템 시스템 추가 - 근접 적중 패시브 끝]

	DLDebug::Screen(FString::Printf(TEXT("[Melee] Light hit %s Coefficient=%.2f BaseDamage=%.1f"), *GetNameSafe(TargetActor), DamageCoefficient, BaseDamage), FColor::Yellow);
}
