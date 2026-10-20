// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_FireProjectile.h"
#include "Common/DLActorPoolSubsystem.h"
#include "Common/ActorSpawnSupport.h"
#include "Weapon/ProjectileAimLibrary.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameCore/Debug/DLDebug.h"
#include "GameCore/Struct/ACharacterAttributeSet.h"
#include "Weapon/WeaponComponent.h"
#include "Weapon/ProjectileBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Common/AbilitySupport.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"

UGA_FireProjectile::UGA_FireProjectile()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	FireGameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Fire"), false);

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.BasicAttack")));
	SetAssetTags(AssetTags);
}

void UGA_FireProjectile::PrewarmActorPool(AActor* PoolOwner, int32 PrewarmCount) const
{
	UWorld* World = PoolOwner ? PoolOwner->GetWorld() : nullptr;
	UDLActorPoolSubsystem* Pool = World ? World->GetSubsystem<UDLActorPoolSubsystem>() : nullptr;
	if (!Pool || !ProjectileClass || PrewarmCount <= 0)
	{
		return;
	}

	Pool->Prewarm<AProjectileBase>(
		ProjectileClass,
		PrewarmCount,
		PoolOwner,
		Cast<APawn>(PoolOwner),
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
}

void UGA_FireProjectile::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABaseCharacter* Character = Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWeaponComponent* Weapon = Character->GetWeaponComponent();
	if (!CanFire(Character, Weapon, ActorInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = Character->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float FireInterval = GetFireInterval(ActorInfo->AbilitySystemComponent.Get());
	const float RemainingTime = FireInterval - (CurrentTime - LastFireTime);

	if (RemainingTime > 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	const bool bFireExpressShotProjectile = NotifyAndConsumeShotItems(Character, SourceASC, CurrentTime);
	// An accepted attempt consumes ammo/interval even when actor spawning fails.
	LastFireTime = CurrentTime;

	Weapon->ConsumeAmmo();

	const FVector SpawnLocation = DLProjectileAim::GetSpawnLocation(
		*Character,
		ProjectileSpawnSocketName,
		SpawnForwardOffset);
	const DLProjectileAim::FShotAim Aim = DLProjectileAim::ResolveShotAim(
		*Character,
		SpawnLocation,
		ActorInfo->AbilitySystemComponent.Get(),
		TraceDistance,
		MaxAccuracyPenaltySpreadDegrees);

	FireShot(Character, Weapon, SpawnLocation, Aim, SourceASC, bFireExpressShotProjectile);

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

bool UGA_FireProjectile::CanFire(ABaseCharacter* Character, UWeaponComponent* Weapon, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!Character) return false;
	if (!Weapon) return false;
	if (!Weapon->CanFire()) return false;
	if (!ProjectileClass) return false;
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()) return false;

	return true;
}

float UGA_FireProjectile::GetFireInterval(const UAbilitySystemComponent* ASC) const
{
	const float FireRate = ASC
		? ASC->GetNumericAttribute(UACharacterAttributeSet::GetFireRateAttribute())
		: DefaultFireRate;

	const float SafeFireRate = FireRate > 0.0f ? FireRate : DefaultFireRate;

	return 1.0f / FMath::Max(0.01f, SafeFireRate);
}


AProjectileBase* UGA_FireProjectile::SpawnProjectile(
	ABaseCharacter* Character,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation) const
{
	return DLActorSpawn::AcquireOrSpawn<AProjectileBase>(Character ? Character->GetWorld() : nullptr,
		ProjectileClass, FTransform(SpawnRotation, SpawnLocation), Character, Character,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
		[&](AProjectileBase* Projectile) { InitializeProjectile(Projectile, Character, CurrentActorInfo); });
}

void UGA_FireProjectile::InitializeProjectile(
	AProjectileBase* Projectile,
	ABaseCharacter* Character,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!Projectile || !Character || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UNiagaraSystem* ProjectileEffect = Character->GetBasicAttackProjectileEffect();

	Projectile->SetDamageParameters(1.f, 0.f);
	Projectile->SetProjectileEffect(ProjectileEffect);
	Projectile->ConfigureCollisionForSource(Character);
}

void UGA_FireProjectile::ExecuteFireGameplayCue(
	ABaseCharacter* Character,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation,
	UAbilitySystemComponent* SourceASC) const
{
	if (!Character || !SourceASC || !FireGameplayCueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters CueParams = DLAbilitySupport::MakeCueParameters(
		SpawnLocation, SpawnRotation.Vector(), Character, Character, this);

	SourceASC->ExecuteGameplayCue(FireGameplayCueTag, CueParams);
}

bool UGA_FireProjectile::NotifyAndConsumeShotItems(ABaseCharacter* Character, UAbilitySystemComponent* SourceASC, float CurrentTime)
{
	const FGameplayTag BeforeWeaponFireTag =
		FGameplayTag::RequestGameplayTag(TEXT("Event.Item.BeforeWeaponFire"));
	const FGameplayTag ExpressShotTriggeredTag =
		FGameplayTag::RequestGameplayTag(TEXT("State.Item.ExpressShot.Triggered"));

	FGameplayEventData BeforeFirePayload;
	BeforeFirePayload.EventTag = BeforeWeaponFireTag;
	BeforeFirePayload.Instigator = Character;
	BeforeFirePayload.Target = Character;
	BeforeFirePayload.EventMagnitude = CurrentTime - LastFireTime;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Character,
		BeforeWeaponFireTag,
		BeforeFirePayload);

	const bool bFireExpressShotProjectile =
		SourceASC && SourceASC->HasMatchingGameplayTag(ExpressShotTriggeredTag);
	if (bFireExpressShotProjectile)
	{
		// 이번 공격에서만 사용하는 신호이므로 Projectile 생성 전에 즉시 소비한다.
		SourceASC->SetLooseGameplayTagCount(ExpressShotTriggeredTag, 0);
	}
	return bFireExpressShotProjectile;
}

void UGA_FireProjectile::FireShot(ABaseCharacter* Character, UWeaponComponent* Weapon, const FVector& Location,
	const DLProjectileAim::FShotAim& Aim, UAbilitySystemComponent* SourceASC, bool bExpressShot) const
{
	const FRotator Rotation = Aim.LaunchDirection.Rotation();
	AProjectileBase* Projectile = SpawnProjectile(Character, Location, Rotation);

	if (Projectile)
	{
		Projectile->SetPortalAim(Aim.Portal);
		Projectile->Launch(Aim.LaunchDirection);
		ExecuteFireGameplayCue(Character, Location, Rotation, SourceASC);
		UE_LOG(LogTemp, Verbose,
			TEXT("[Projectile] Spawned: Projectile=%s Owner=%s Location=%s"),
			*GetNameSafe(Projectile),
			*GetNameSafe(Character),
			*Location.ToCompactString());

		DLDebug::Screen(
			FString::Printf(
				TEXT("[Fire] Projectile fired. Ammo=%d/%d"),
				Weapon->GetCurrentAmmo(),
				Weapon->GetMagazineSize()
			),
			FColor::Green
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Projectile] Spawn failed: Class=%s Owner=%s Location=%s"),
			*GetNameSafe(ProjectileClass),
			*GetNameSafe(Character),
			*Location.ToCompactString());
	}

	if (bExpressShot)
	{
		if (AProjectileBase* AdditionalProjectile =
			SpawnProjectile(Character, Location, Rotation))
		{
			AdditionalProjectile->SetPortalAim(Aim.Portal);
			AdditionalProjectile->Launch(Aim.LaunchDirection);
			UE_LOG(LogTemp, Verbose,
				TEXT("[ExpressShot] Additional projectile spawned: Projectile=%s AmmoConsumed=false"),
				*GetNameSafe(AdditionalProjectile));
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[ExpressShot] Additional projectile spawn failed: Class=%s"),
				*GetNameSafe(ProjectileClass));
		}
	}

}
