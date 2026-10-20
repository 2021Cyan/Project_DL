// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/CallBell/GA_CallBell.h"
#include "Common/AbilityTags.h"

#include "Doorman/CallBell/CallBellProjectile.h"
#include "AbilitySystemComponent.h"
#include "Common/AbilitySupport.h"
#include "Common/ActorSpawnSupport.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectTypes.h"

UGA_CallBell::UGA_CallBell()
{
	FGameplayTagContainer AssetTags;
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.Doorman.CallBell")), false);
	if (AbilityTag.IsValid())
	{
		AssetTags.AddTag(AbilityTag);
	}
	SetAssetTags(AssetTags);
	LevelData.AddDefaulted();
}

void UGA_CallBell::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bHasExecuted = false;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_CallBell::OnCastStarted_Implementation()
{
	Super::OnCastStarted_Implementation();
	SpawnCallBell();
}

void UGA_CallBell::SpawnCallBell()
{
	if (bHasExecuted)
	{
		return;
	}

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!CallBellClass)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = AvatarActor ? AvatarActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const DLPlayerView::FAimView View = DLPlayerView::GetAimView(*AvatarActor);
	const FVector SpawnLocation = GetSpawnLocation(View);
	const FVector LaunchDirection = BuildLaunchDirection(SpawnLocation, View);

	ACallBellProjectile* CallBell = DLActorSpawn::AcquireOrSpawn<ACallBellProjectile>(World, CallBellClass,
		FTransform(LaunchDirection.Rotation(), SpawnLocation), AvatarActor, Cast<APawn>(AvatarActor),
		SpawnCollisionHandlingMethod, [&]( ACallBellProjectile* CallBell)
		{
			const FCallBellLevelData& CurrentLevelData = GetCurrentLevelData();

			FCallBellProjectileInitData InitData;
			InitData.SourceActor = AvatarActor;
			InitData.ImpactDamage = CurrentLevelData.ImpactDamage;
			InitData.ImpactDamageCoefficient = CurrentLevelData.ImpactSpiritPowerScale;
			InitData.ExplosionDamage = CurrentLevelData.ExplosionDamage;
			InitData.ExplosionDamageCoefficient = CurrentLevelData.ExplosionSpiritPowerScale;
			InitData.StatusEffectSpecs = MakeStatusEffectSpecs();
			InitData.ExplosionRadius = CurrentLevelData.ExplosionRadius;
			InitData.BellLifetime = CurrentLevelData.BellLifetime;

			CallBell->InitializeCallBell(InitData);
		});
	if (!CallBell) return;
	bHasExecuted = true;

	CallBell->Launch(LaunchDirection);
}

FVector UGA_CallBell::GetSpawnLocation(const DLPlayerView::FAimView& View) const
{
	return View.Location + View.Rotation.Vector().GetSafeNormal() * CameraRaySpawnDistance;
}

FVector UGA_CallBell::BuildLaunchDirection(const FVector& SpawnLocation, const DLPlayerView::FAimView& View) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!Pawn)
	{
		return AvatarActor ? AvatarActor->GetActorForwardVector().GetSafeNormal() : FVector::ZeroVector;
	}

	const FVector ViewLocation = View.Location;
	const FRotator ViewRotation = View.Rotation;

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;
	FHitResult AimHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CallBellAimTrace), false, Pawn);
	const bool bHit = Pawn->GetWorld() && Pawn->GetWorld()->LineTraceSingleByChannel(
		AimHit,
		ViewLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams);

	const FVector TargetPoint = bHit ? AimHit.ImpactPoint : TraceEnd;
	FVector Direction = (TargetPoint - SpawnLocation).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = ViewRotation.Vector().GetSafeNormal();
	}

	return Direction;
}


const FCallBellLevelData& UGA_CallBell::GetCurrentLevelData() const
{
	return DLAbilitySupport::ResolveLevelData(LevelData, GetAbilityLevel());
}

FGameplayEffectSpecHandle UGA_CallBell::MakeEffectSpec(TSubclassOf<UGameplayEffect> EffectClass) const
{
	return DLAbilitySupport::MakeEffectSpec(GetAbilitySystemComponentFromActorInfo(),
		EffectClass, GetAbilityLevel(), GetAvatarActorFromActorInfo(), this);
}

FGameplayEffectSpecHandle UGA_CallBell::MakeDebuffSpec(
	TSubclassOf<UGameplayEffect> EffectClass, FName MagnitudeTagName,
	float Magnitude, float Duration) const
{
	const FGameplayTag DurationTag = DLAbilityTags::Duration();
	const FGameplayTag MagnitudeTag = DLAbilityTags::Find(MagnitudeTagName);
	if (!DurationTag.IsValid() || !MagnitudeTag.IsValid())
	{
		return {};
	}
	FGameplayEffectSpecHandle Spec = MakeEffectSpec(EffectClass);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(DurationTag, Duration);
		Spec.Data->SetSetByCallerMagnitude(MagnitudeTag, Magnitude);
	}
	return Spec;
}

TArray<FGameplayEffectSpecHandle> UGA_CallBell::MakeStatusEffectSpecs() const
{
	TArray<FGameplayEffectSpecHandle> Specs;
	const FCallBellLevelData& CurrentLevelData = GetCurrentLevelData();

	FGameplayEffectSpecHandle SlowSpec = MakeDebuffSpec(SlowEffectClass, TEXT("Debuff.SlowPercent"),
		CurrentLevelData.SlowPercent, CurrentLevelData.DebuffDuration);
	if (SlowSpec.IsValid())
	{
		Specs.Add(SlowSpec);
	}

	FGameplayEffectSpecHandle DazedSpec = MakeDebuffSpec(DazedEffectClass, TEXT("Debuff.WeaponAccuracyMultiplier"),
		CurrentLevelData.WeaponAccuracyMultiplier, CurrentLevelData.DebuffDuration);
	if (DazedSpec.IsValid())
	{
		Specs.Add(DazedSpec);
	}

	return Specs;
}

bool UGA_CallBell::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return ActorInfo && ActorInfo->AvatarActor.IsValid() && (CallBellClass)
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}
