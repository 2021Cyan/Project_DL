// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_FireProjectile.generated.h"

class ABaseCharacter;
class UWeaponComponent;
class UAbilitySystemComponent;
class AProjectileBase;
namespace DLProjectileAim
{
	struct FShotAim;
}

UCLASS()
class TEAMPROJECT_DL_API UGA_FireProjectile : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_FireProjectile();
	void PrewarmActorPool(AActor* PoolOwner, int32 PrewarmCount) const;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;


	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass;
private:
	float LastFireTime = -TNumericLimits<float>::Max();

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float SpawnForwardOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	float DefaultFireRate = 1.47f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float TraceDistance = 100000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Fire")
	FName ProjectileSpawnSocketName = TEXT("ProjectileSpawnPoint");

	UPROPERTY(EditDefaultsOnly, Category = "Fire|GameplayCue")
	FGameplayTag FireGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Fire|Accuracy", meta = (ClampMin = "0.0"))
	float MaxAccuracyPenaltySpreadDegrees = 18.f;

	bool CanFire(
		ABaseCharacter* Character,
		UWeaponComponent* Weapon,
		const FGameplayAbilityActorInfo* ActorInfo) const;

	bool NotifyAndConsumeShotItems(ABaseCharacter* Character, UAbilitySystemComponent* SourceASC, float CurrentTime);
	void FireShot(ABaseCharacter* Character, UWeaponComponent* Weapon, const FVector& Location,
		const DLProjectileAim::FShotAim& Aim, UAbilitySystemComponent* SourceASC, bool bExpressShot) const;
	float GetFireInterval(const UAbilitySystemComponent* ASC) const;

	AProjectileBase* SpawnProjectile(
		ABaseCharacter* Character,
		const FVector& SpawnLocation,
		const FRotator& SpawnRotation) const;

	void InitializeProjectile(
		AProjectileBase* Projectile,
		ABaseCharacter* Character,
		const FGameplayAbilityActorInfo* ActorInfo) const;

	void ExecuteFireGameplayCue(
		ABaseCharacter* Character,
		const FVector& SpawnLocation,
		const FRotator& SpawnRotation,
		UAbilitySystemComponent* SourceASC) const;
};
