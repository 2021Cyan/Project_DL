// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/HotelGuest/GA_HotelGuest.h"
#include "Common/AbilityTags.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Common/AbilitySupport.h"
#include "Common/SkillDamageLibrary.h"
#include "Doorman/HotelGuest/HotelGuestLocationActor.h"
#include "GameFramework/Character.h"
#include "GameCore/Character/ADLPlayerState.h"
#include "GameCore/Character/BasePlayerController.h"
#include "GameCore/Character/PlayerCharacter.h"
#include "GameCore/Struct/ACharacterAttributeSet.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"

UGA_HotelGuest::UGA_HotelGuest()
{
	FGameplayTagContainer AssetTags;
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.Doorman.HotelGuest")), false);
	if (AbilityTag.IsValid())
	{
		AssetTags.AddTag(AbilityTag);
	}
	SetAssetTags(AssetTags);

	LocationActorClass = AHotelGuestLocationActor::StaticClass();
	TargetingMode = ESkillTargetingMode::Scope;
	bEndAbilityWhenCastMontageEnds = false;
	bRequiresSkillEvent = true;
	SkillEventParentTag = DLAbilityTags::SkillEvent();
	ExecuteEventTag = DLAbilityTags::SkillExecute();
	LevelData.AddDefaulted();
}

void UGA_HotelGuest::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	bUseFailureCooldown = false;
	HotelTarget = nullptr;
	HotelLocationActor = nullptr;
	OriginalTransform = FTransform::Identity;
	OriginalControlRotation = FRotator::ZeroRotator;
	HotelState = EHotelGuestState::Targeting;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_HotelGuest::OnCastStarted_Implementation()
{
	Super::OnCastStarted_Implementation();
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	TArray<AActor*> Targets = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(CachedTargetData, 0);
	APlayerCharacter* Target = Targets.Num() > 0 ? Cast<APlayerCharacter>(Targets[0]) : nullptr;
	if (!SourceActor || !Target || !ADLPlayerState::IsHostileActor(SourceActor, Target))
	{
		FinishHotelStay(EHotelGuestResult::Cancelled);
		return;
	}

	HotelTarget = Target;
	HotelState = EHotelGuestState::WaitingForEvent;
	StartTargetCameraDarken();
}

void UGA_HotelGuest::OnSkillEvent_Implementation(FGameplayTag EventTag, FGameplayEventData Payload)
{
	Super::OnSkillEvent_Implementation(EventTag, Payload);

	if (!EventTag.MatchesTagExact(ExecuteEventTag) || HotelState != EHotelGuestState::WaitingForEvent)
	{
		return;
	}

	EnterHotel();
}

void UGA_HotelGuest::EnterHotel()
{
	if (!HotelTarget || !HasAuthority(&CurrentActivationInfo))
	{
		FinishHotelStay(EHotelGuestResult::Cancelled);
		return;
	}

	HotelLocationActor = FindLocationActor();
	if (!HotelLocationActor)
	{
		FinishHotelStay(EHotelGuestResult::Cancelled);
		return;
	}

	OriginalTransform = HotelTarget->GetActorTransform();
	OriginalControlRotation = HotelTarget->GetControlRotation();

	ActiveStayLevelData = GetCurrentLevelData();
	const FHotelGuestLevelData& CurrentLevelData = ActiveStayLevelData;

	const FVector HotelStartLocation = HotelLocationActor->GetHotelStartLocation();
	const FRotator HotelStartRotation(0.f, HotelLocationActor->GetHotelStartRotation().Yaw, 0.f);
	if (!HotelTarget->TeleportInstantly(HotelStartLocation, HotelStartRotation, HotelStartRotation))
	{
		FinishHotelStay(EHotelGuestResult::Cancelled);
		return;
	}

	HotelState = EHotelGuestState::InHotel;
	ApplyStaminaDamage(CurrentLevelData);

	GetWorld()->GetTimerManager().SetTimer(StayTickHandle, this, &ThisClass::TickHotelStay, 0.1f, true);
	GetWorld()->GetTimerManager().SetTimer(
		FailureHandle,
		this,
		&ThisClass::OnHotelStayExpired,
		CurrentLevelData.EscapeDuration,
		false);
}

void UGA_HotelGuest::TickHotelStay()
{
	if (HotelState != EHotelGuestState::InHotel)
	{
		return;
	}
	if (!IsValid(HotelTarget) || !IsValid(HotelLocationActor))
	{
		FinishHotelStay(EHotelGuestResult::Cancelled);
		return;
	}

	if (IsTargetInsideEscapeRadius())
	{
		FinishHotelStay(EHotelGuestResult::Escaped);
		return;
	}

	const FVector Location = HotelTarget->GetActorLocation();
	const FVector HotelStartLocation = HotelLocationActor->GetHotelStartLocation();
	if (Location.Z < HotelStartLocation.Z - FallResetDistance)
	{
		const FRotator HotelStartRotation(0.f, HotelLocationActor->GetHotelStartRotation().Yaw, 0.f);
		HotelTarget->TeleportInstantly(HotelStartLocation, HotelStartRotation, HotelStartRotation);
	}
}

void UGA_HotelGuest::OnHotelStayExpired()
{
	if (HotelState != EHotelGuestState::InHotel)
	{
		return;
	}

	FinishHotelStay(IsTargetInsideEscapeRadius()
		? EHotelGuestResult::Escaped
		: EHotelGuestResult::Failed);
}

bool UGA_HotelGuest::IsTargetInsideEscapeRadius() const
{
	return HotelTarget
		&& HotelLocationActor
		&& FVector::DistSquared(HotelTarget->GetActorLocation(), HotelLocationActor->GetEscapeLocation())
			<= FMath::Square(EscapeRadius);
}

void UGA_HotelGuest::FinishHotelStay(EHotelGuestResult Result)
{
	if (HotelState == EHotelGuestState::Resolved || bEnding)
	{
		return;
	}

	const bool bWasInHotel = HotelState == EHotelGuestState::InHotel;
	HotelState = EHotelGuestState::Resolved;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StayTickHandle);
		World->GetTimerManager().ClearTimer(FailureHandle);
	}

	APlayerCharacter* Target = HotelTarget;
	const bool bHasResult = Result != EHotelGuestResult::Cancelled;
	if (Target && bWasInHotel)
	{
		const FHotelGuestLevelData& CurrentLevelData = ActiveStayLevelData;
		if (bHasResult)
		{
			USkillDamageLibrary::ApplyDamage(
				GetAvatarActorFromActorInfo(),
				Target,
				CurrentLevelData.SuccessSpiritPowerScale,
				CurrentLevelData.SuccessDamage,
				TAG_Data_Damage_Spirit);

			if (Result == EHotelGuestResult::Failed)
			{
				USkillDamageLibrary::ApplyDamage(
					GetAvatarActorFromActorInfo(),
					Target,
					CurrentLevelData.FailureSpiritPowerScale,
					CurrentLevelData.FailureDamage,
					TAG_Data_Damage_Spirit);
				if (CurrentLevelData.bOverrideCooldownAfterFailure)
				{
					OverrideCooldownAfterFailure();
				}
			}

		}

	}
	CleanupHotelStay(bWasInHotel);
	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		Result == EHotelGuestResult::Cancelled);
}

void UGA_HotelGuest::ApplyStaminaDamage(const FHotelGuestLevelData& Data)
{
	if (!HotelTarget || Data.StaminaDamage <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HotelTarget);
	if (!TargetASC)
	{
		return;
	}
	const float CurrentStamina = TargetASC->GetNumericAttribute(UACharacterAttributeSet::GetStaminaAttribute());
	const float AppliedStaminaDamage = FMath::Min(Data.StaminaDamage, FMath::Max(0.f, CurrentStamina));
	if (AppliedStaminaDamage <= 0.f)
	{
		return;
	}

	UGameplayEffect* StaminaEffect = NewObject<UGameplayEffect>(GetTransientPackage());
	StaminaEffect->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Modifier = StaminaEffect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UACharacterAttributeSet::GetStaminaAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-AppliedStaminaDamage));

	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	if (AActor* SourceActor = GetAvatarActorFromActorInfo())
	{
		Context.AddInstigator(SourceActor, SourceActor);
	}
	Context.AddSourceObject(this);
	TargetASC->ApplyGameplayEffectToSelf(StaminaEffect, GetAbilityLevel(), Context);
}

void UGA_HotelGuest::StartTargetCameraDarken() const
{
	ABasePlayerController* TargetController = GetHotelTargetController();
	if (!TargetController || CameraFadeAlpha <= 0.f)
	{
		return;
	}

	TargetController->ClientPulseCameraFade(
		FLinearColor::Black,
		CameraFadeAlpha,
		CameraFadeInDuration,
		CameraFadeOutDuration);
}

void UGA_HotelGuest::StopTargetCameraDarken() const
{
	ABasePlayerController* TargetController = GetHotelTargetController();
	if (!TargetController || CameraFadeAlpha <= 0.f)
	{
		return;
	}

	TargetController->ClientStopCameraFade(CameraFadeOutDuration);
}

ABasePlayerController* UGA_HotelGuest::GetHotelTargetController() const
{
	return HotelTarget
		? Cast<ABasePlayerController>(HotelTarget->GetController())
		: nullptr;
}

AHotelGuestLocationActor* UGA_HotelGuest::FindLocationActor() const
{
	UWorld* World = GetWorld();
	UClass* ActorClass = LocationActorClass
		? LocationActorClass.Get()
		: AHotelGuestLocationActor::StaticClass();
	if (!World)
	{
		return nullptr;
	}

	return Cast<AHotelGuestLocationActor>(UGameplayStatics::GetActorOfClass(World, ActorClass));
}

void UGA_HotelGuest::OverrideCooldownAfterFailure()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}
	FGameplayTagContainer Tags;
	if (CooldownTag.IsValid())
	{
		Tags.AddTag(CooldownTag);
	}
	ASC->RemoveActiveEffectsWithGrantedTags(Tags);
	bUseFailureCooldown = true;
	ApplyCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	bUseFailureCooldown = false;
}

float UGA_HotelGuest::GetEffectiveCooldownDuration() const
{
	const FHotelGuestLevelData& CurrentLevelData = GetCurrentLevelData();
	return bUseFailureCooldown
		? CurrentLevelData.FailureCooldownDuration
		: CurrentLevelData.CooldownDuration;
}

const FHotelGuestLevelData& UGA_HotelGuest::GetCurrentLevelData() const
{
	return DLAbilitySupport::ResolveLevelData(LevelData, GetAbilityLevel());
}


void UGA_HotelGuest::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEnding) return;
	bEnding = true;
	CleanupHotelStay(HotelState == EHotelGuestState::InHotel);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UGA_HotelGuest::CleanupHotelStay(bool bRestorePosition)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StayTickHandle);
		World->GetTimerManager().ClearTimer(FailureHandle);
	}
	if (IsValid(HotelTarget))
	{
		StopTargetCameraDarken();
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HotelTarget);
		const FGameplayTag DeadTag = DLAbilityTags::Find(TEXT("Character.State.Dead"));
		const bool bDead = TargetASC && DeadTag.IsValid() && TargetASC->HasMatchingGameplayTag(DeadTag);
		if (bRestorePosition && !bDead && HotelTarget->GetWorld() == GetWorld())
			HotelTarget->TeleportInstantly(OriginalTransform.GetLocation(), OriginalTransform.Rotator(), OriginalControlRotation);
	}
	HotelState = EHotelGuestState::Resolved;
	HotelTarget = nullptr;
	HotelLocationActor = nullptr;
}

bool UGA_HotelGuest::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return ActorInfo && ActorInfo->AvatarActor.IsValid() && (ExecuteEventTag.IsValid())
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}
