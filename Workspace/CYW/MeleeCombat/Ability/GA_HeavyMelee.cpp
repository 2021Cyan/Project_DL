// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_HeavyMelee.h"
#include "GameCore/Combat/MeleeHitUtils.h"
#include "Common/SkillDamageLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameCore/Debug/DLDebug.h"
#include "GameCore/Struct/ACharacterAttributeSet.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_HeavyMelee::UGA_HeavyMelee()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	const FGameplayTag HeavyMeleeTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.MeleeAttack.Heavy")), false);

	if (HeavyMeleeTag.IsValid())
	{
		AssetTags.AddTag(HeavyMeleeTag);
	}

	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("State.Movement.AirDash"))));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("State.Movement.Rolling"))));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("State.Movement.Slide"))));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("State.Movement.Vaulting"))));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("State.Movement.WallCling"))));
}

void UGA_HeavyMelee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		DLDebug::Screen(TEXT("[Melee] GA_HeavyMelee failed: CommitAbility false"), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	if (!Character || !Character->HeavyMeleeMontage)
	{
		DLDebug::Screen(FString::Printf(TEXT("[Melee] GA_HeavyMelee failed Character=%s Montage=%s"),
			*GetNameSafe(Character),
			Character ? *GetNameSafe(Character->HeavyMeleeMontage.Get()) : TEXT("null")), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SetLungeState(true);
	ApplyLungeMovementState(Character);

	const FGameplayTag LungeEventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.Melee.StartLunge")), false);
	if (LungeEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* LungeEventTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, LungeEventTag, nullptr, true, true);
		if (LungeEventTask)
		{
			LungeEventTask->EventReceived.AddDynamic(this, &ThisClass::OnLungeEvent);
			LungeEventTask->ReadyForActivation();
		}
	}
	else
	{
		DLDebug::Screen(TEXT("[Melee] Event.Melee.StartLunge tag invalid"), FColor::Red);
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
		Character->HeavyMeleeMontage.Get()
	);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_HeavyMelee::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	StopLunge();
	RestoreLungeMovementState();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_HeavyMelee::OnMontageCompleted()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_HeavyMelee::OnMontageBlendOut()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_HeavyMelee::OnMontageCancelled()
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_HeavyMelee::OnHitEvent(FGameplayEventData Payload)
{
	(void)Payload;
	PerformMeleeHit();
}

void UGA_HeavyMelee::OnLungeEvent(FGameplayEventData Payload)
{
	(void)Payload;

	ABaseCharacter* Character = CurrentActorInfo ? Cast<ABaseCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	UCharacterMovementComponent* MovementComponent = LungeMovementState.MovementComponent.Get();
	if (MovementComponent && bLungeMovementModeDisabled)
	{
		MovementComponent->SetMovementMode(LungeMovementState.MovementMode, LungeMovementState.CustomMovementMode);
		bLungeMovementModeDisabled = false;
	}

	StartLunge(Character);
}

void UGA_HeavyMelee::ApplyLungeMovementState(ABaseCharacter* Character)
{
	UCharacterMovementComponent* MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !MovementComponent || bLungeMovementStateApplied)
	{
		return;
	}

	LungeMovementState.Capture(Character);
	bLungeMovementStateApplied = true;

	Character->ConsumeMovementInputVector();
	MovementComponent->StopMovementImmediately();
	Character->bUseControllerRotationYaw = true;
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = false;
	if (MovementComponent->IsFalling())
	{
		MovementComponent->GravityScale = 0.f;
		bLungeGravityScaleApplied = true;
	}

	MovementComponent->DisableMovement();
	bLungeMovementModeDisabled = true;
}

void UGA_HeavyMelee::RestoreLungeMovementState()
{
	UCharacterMovementComponent* MovementComponent = LungeMovementState.MovementComponent.Get();
	if (!MovementComponent || !bLungeMovementStateApplied)
	{
		return;
	}

	if (bLungeMovementModeDisabled)
	{
		MovementComponent->SetMovementMode(LungeMovementState.MovementMode, LungeMovementState.CustomMovementMode);
		bLungeMovementModeDisabled = false;
	}

	ABaseCharacter* Character = CurrentActorInfo ? Cast<ABaseCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	LungeMovementState.Restore(Character);
	bLungeGravityScaleApplied = false;
	bLungeMovementStateApplied = false;
}

void UGA_HeavyMelee::StartLunge(ABaseCharacter* Character)
{
	if (!Character || LungeDuration <= 0.f || LungeSpeed <= 0.f)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	const FRotator ControlRotation = Character->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	FVector LungeDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X).GetSafeNormal();
	if (LungeDirection.IsNearlyZero())
	{
		LungeDirection = Character->GetActorForwardVector().GetSafeNormal();
	}

	Character->SetActorRotation(FRotator(0.f, LungeDirection.Rotation().Yaw, 0.f));
	Character->bUseControllerRotationYaw = false;

	const FVector FinishVelocity = bStopHorizontalVelocityOnLungeEnd
		? FVector::ZeroVector
		: FVector(LungeDirection.X * MovementComponent->MaxWalkSpeed, LungeDirection.Y * MovementComponent->MaxWalkSpeed, 0.f);

	UAbilityTask_ApplyRootMotionConstantForce* LungeTask =
		UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this,
			TEXT("HeavyMeleeLungeRootMotionForce"),
			LungeDirection,
			LungeSpeed,
			LungeDuration,
			false,
			LungeSpeedCurve.Get(),
			ERootMotionFinishVelocityMode::SetVelocity,
			FinishVelocity,
			0.f,
			false
		);

	if (LungeTask)
	{
		LungeTask->OnFinish.AddDynamic(this, &ThisClass::OnLungeFinished);
		LungeTask->ReadyForActivation();
	}
}

void UGA_HeavyMelee::StopLunge()
{
	SetLungeState(false);
}

void UGA_HeavyMelee::SetLungeState(bool bShouldLunge)
{
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return;
	}

	const FGameplayTag LungeTag = FGameplayTag::RequestGameplayTag(FName(TEXT("State.Combat.HeavyMeleeLunging")), false);
	if (!LungeTag.IsValid())
	{
		DLDebug::Screen(TEXT("[Melee] State.Combat.HeavyMeleeLunging tag invalid"), FColor::Red);
		return;
	}

	ASC->SetLooseGameplayTagCount(LungeTag, bShouldLunge ? 1 : 0);
}

void UGA_HeavyMelee::OnLungeFinished()
{
	StopLunge();
	RestoreLungeMovementState();
}

void UGA_HeavyMelee::PerformMeleeHit()
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
	HitParams.DebugColor = FColor::Orange;
	HitParams.InitialTraceStatName = TEXT("HeavyMeleeInitialCheck");
	HitParams.ConeTraceStatName = TEXT("HeavyMeleeConeCheck");
	HitParams.ParryTraceStatName = TEXT("HeavyMeleeParryCheck");

	if (DLMeleeHitUtils::HasParryingTargetInInitialCheck(*SourceCharacter, HitParams))
	{
		DLDebug::Screen(TEXT("[Parry] Heavy melee parried: attacker stun trigger point"), FColor::Orange);
		return;
	}

	TArray<AActor*> TargetActors;
	DLMeleeHitUtils::FindTargetsInCone(*SourceCharacter, HitParams, TargetActors);
	for (AActor* TargetActor : TargetActors)
	{
		ApplyMeleeDamage(SourceCharacter, TargetActor);
	}
}

void UGA_HeavyMelee::ApplyMeleeDamage(ABaseCharacter* SourceCharacter, AActor* TargetActor) const
{
	if (!SourceCharacter || !TargetActor || (DamageCoefficient <= 0.f && BaseDamage <= 0.f))
	{
		return;
	}

	USkillDamageLibrary::ApplyDamage(
		SourceCharacter, TargetActor, DamageCoefficient, BaseDamage, TAG_Data_Damage_HeavyMelee);

	// ============================================================================
	// [아이템 시스템 추가 - 근접 적중 패시브]
	// develop의 공용 피해 처리는 유지하고, 피해 적용이 끝난 뒤 아이템 패시브에
	// Heavy 근접 적중 사실과 계산된 기본 피해량을 전달한다.
	// ============================================================================
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceCharacter);
	const float SourceMeleeDamage = SourceASC
		? SourceASC->GetNumericAttribute(UACharacterAttributeSet::GetHeavyMeleeDamageAttribute())
		: 0.f;
	const float EventDamage = SourceMeleeDamage * DamageCoefficient + BaseDamage;
	const FGameplayTag MeleeHitTag =
		FGameplayTag::RequestGameplayTag(TEXT("Event.Item.MeleeHit.Heavy"));
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

	DLDebug::Screen(FString::Printf(TEXT("[Melee] Heavy hit %s Coefficient=%.2f BaseDamage=%.1f"), *GetNameSafe(TargetActor), DamageCoefficient, BaseDamage), FColor::Orange);
}
