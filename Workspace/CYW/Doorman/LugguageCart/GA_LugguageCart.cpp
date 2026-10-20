// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/LugguageCart/GA_LugguageCart.h"
#include "Common/AbilityTags.h"

#include "AbilitySystemComponent.h"
#include "Common/AbilitySupport.h"
#include "Common/ActorSpawnSupport.h"
#include "Doorman/LugguageCart/LugguageCartActor.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectTypes.h"

UGA_LugguageCart::UGA_LugguageCart()
{
	FGameplayTagContainer AssetTags;
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.Doorman.LugguageCart")), false);
	if (AbilityTag.IsValid())
	{
		AssetTags.AddTag(AbilityTag);
	}
	SetAssetTags(AssetTags);

	TargetingMode = ESkillTargetingMode::None;
	bRequiresSkillEvent = true;
	ExecuteEventTag = DLAbilityTags::SkillExecute();
	SkillEventParentTag = DLAbilityTags::SkillEvent();
	SpawnGameplayCueTag = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayCue.Skill.LugguageCart.Spawn")), false);
	LevelData.AddDefaulted();
}

void UGA_LugguageCart::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bHasExecuted = false;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_LugguageCart::OnSkillEvent_Implementation(FGameplayTag EventTag, FGameplayEventData Payload)
{
	Super::OnSkillEvent_Implementation(EventTag, Payload);
	if (!EventTag.MatchesTagExact(ExecuteEventTag) || bHasExecuted)
	{
		return;
	}

	SpawnCart();
}

void UGA_LugguageCart::SpawnCart()
{
	if (bHasExecuted)
	{
		return;
	}

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!CartClass)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = AvatarActor ? AvatarActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const FVector TravelDirection = BuildTravelDirection(AvatarActor);
	const FVector SpawnLocation = GetSpawnLocation(AvatarActor, TravelDirection);

	ALugguageCartActor* Cart = DLActorSpawn::AcquireOrSpawn<ALugguageCartActor>(World, CartClass,
		FTransform(TravelDirection.Rotation(), SpawnLocation), AvatarActor, Cast<APawn>(AvatarActor),
		SpawnCollisionHandlingMethod, [&]( ALugguageCartActor* Cart)
		{
			const FLugguageCartLevelData& CurrentLevelData = GetCurrentLevelData();
			FLugguageCartInitData Data;
			Data.SourceActor = AvatarActor;
			Data.DamageCoefficient = CurrentLevelData.SpiritPowerScale;
			Data.BaseDamage = CurrentLevelData.Damage;
			Data.WallImpactDamage = CurrentLevelData.WallImpactDamage;
			Data.WallImpactDamageCoefficient = CurrentLevelData.WallImpactSpiritPowerScale;
			Data.MaxTravelDistance = CurrentLevelData.MaxTravelDistance;
			Data.StunEffectSpec = MakeStunSpec(CurrentLevelData);
			Cart->InitializeCart(Data);
		});
	if (!Cart) return;
	bHasExecuted = true;

	Cart->Launch(TravelDirection);

	ExecuteSpawnGameplayCue(Cart, TravelDirection);
}

FVector UGA_LugguageCart::GetSpawnLocation(AActor* AvatarActor, const FVector& TravelDirection) const
{
	if (!AvatarActor)
	{
		return FVector::ZeroVector;
	}

	return AvatarActor->GetActorLocation()
		+ TravelDirection * SpawnForwardOffset
		+ FVector::UpVector * SpawnUpOffset;
}

FVector UGA_LugguageCart::BuildTravelDirection(AActor* AvatarActor) const
{
	if (!AvatarActor)
	{
		return FVector::ForwardVector;
	}

	FVector Direction = AvatarActor->GetActorForwardVector();
	Direction.Z = 0.f;
	return Direction.GetSafeNormal();
}

float UGA_LugguageCart::GetEffectiveCooldownDuration() const
{
	return GetCurrentLevelData().CooldownDuration;
}

const FLugguageCartLevelData& UGA_LugguageCart::GetCurrentLevelData() const
{
	return DLAbilitySupport::ResolveLevelData(LevelData, GetAbilityLevel());
}


void UGA_LugguageCart::ExecuteSpawnGameplayCue(AActor* SpawnedActor, const FVector& CueNormal) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SpawnGameplayCueTag.IsValid() || !AvatarActor || !SourceASC || !SpawnedActor)
	{
		return;
	}

	FGameplayCueParameters CueParams = DLAbilitySupport::MakeCueParameters(
		SpawnedActor->GetActorLocation(), CueNormal, AvatarActor, SpawnedActor, this);
	SourceASC->ExecuteGameplayCue(SpawnGameplayCueTag, CueParams);
}

FGameplayEffectSpecHandle UGA_LugguageCart::MakeStunSpec(const FLugguageCartLevelData& Data) const
{
	if (!StunEffectClass || Data.StunDuration <= 0.f)
	{
		return FGameplayEffectSpecHandle();
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!SourceASC || !AvatarActor)
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = DLAbilitySupport::MakeEffectSpec(
		SourceASC, StunEffectClass, GetAbilityLevel(), AvatarActor, this);
	const FGameplayTag DurationTag = DLAbilityTags::Duration();
	if (SpecHandle.IsValid() && DurationTag.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(DurationTag, Data.StunDuration);
	}

	return SpecHandle;
}


bool UGA_LugguageCart::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return ActorInfo && ActorInfo->AvatarActor.IsValid() && (CartClass && ExecuteEventTag.IsValid())
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}
