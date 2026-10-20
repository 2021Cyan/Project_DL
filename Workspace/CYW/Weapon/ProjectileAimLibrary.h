#pragma once

#include "CoreMinimal.h"

class ABaseCharacter;
class UAbilitySystemComponent;
class ADoorwayActor;

namespace DLProjectileAim
{
	struct FPortalAim
	{
		TWeakObjectPtr<ADoorwayActor> Entrance;
		TWeakObjectPtr<ADoorwayActor> Exit;
		FVector Target = FVector::ZeroVector;
		FVector UnspreadDirection = FVector::ZeroVector;
	};

	struct FShotAim
	{
		// Destination world position; differs from MuzzleAimPoint for portal shots.
		FVector WorldTarget = FVector::ZeroVector;
		// Virtual target in the muzzle's space, used to aim toward the entrance.
		FVector MuzzleAimPoint = FVector::ZeroVector;
		FVector LaunchDirection = FVector::ForwardVector;
		FPortalAim Portal;
	};
	FVector GetSpawnLocation(
		ABaseCharacter& Character,
		const FName& SpawnSocketName,
		float SpawnForwardOffset);

	FVector ApplyAccuracySpread(
		const FVector& FireDirection,
		const UAbilitySystemComponent* ASC,
		float MaxAccuracyPenaltySpreadDegrees);

	FShotAim ResolveShotAim(
		ABaseCharacter& Character,
		const FVector& SpawnLocation,
		const UAbilitySystemComponent* ASC,
		float TraceDistance,
		float MaxAccuracyPenaltySpreadDegrees);
}
