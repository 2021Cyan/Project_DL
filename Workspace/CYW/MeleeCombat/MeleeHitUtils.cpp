#include "GameCore/Combat/MeleeHitUtils.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameplayTagContainer.h"

namespace
{
	FVector GetInitialCenter(const ABaseCharacter& SourceCharacter, const FMeleeHitQueryParams& Params)
	{
		return SourceCharacter.GetActorLocation()
			+ SourceCharacter.GetActorForwardVector().GetSafeNormal() * Params.InitialCheckForwardOffset;
	}

	bool QueryOverlaps(
		UWorld& World,
		const ABaseCharacter& SourceCharacter,
		const FVector& Center,
		float Radius,
		const FName& CollisionProfileName,
		const FName& TraceStatName,
		TArray<FOverlapResult>& OutOverlaps)
	{
		FCollisionQueryParams QueryParams(TraceStatName, false, &SourceCharacter);
		return World.OverlapMultiByProfile(
			OutOverlaps,
			Center,
			FQuat::Identity,
			CollisionProfileName,
			FCollisionShape::MakeSphere(Radius),
			QueryParams);
	}
}

bool DLMeleeHitUtils::HasInitialOverlap(
	const ABaseCharacter& SourceCharacter,
	const FMeleeHitQueryParams& Params)
{
	UWorld* World = SourceCharacter.GetWorld();
	if (!World)
	{
		return false;
	}

	TArray<FOverlapResult> InitialOverlaps;
	return QueryOverlaps(
		*World,
		SourceCharacter,
		GetInitialCenter(SourceCharacter, Params),
		Params.InitialCheckRadius,
		Params.CollisionProfileName,
		Params.InitialTraceStatName,
		InitialOverlaps);
}

bool DLMeleeHitUtils::HasParryingTargetInInitialCheck(
	const ABaseCharacter& SourceCharacter,
	const FMeleeHitQueryParams& Params)
{
	UWorld* World = SourceCharacter.GetWorld();
	if (!World)
	{
		return false;
	}

	const FGameplayTag ParryingTag =
		FGameplayTag::RequestGameplayTag(FName(TEXT("State.Combat.Parrying")), false);
	if (!ParryingTag.IsValid())
	{
		return false;
	}

	TArray<FOverlapResult> Overlaps;
	QueryOverlaps(
		*World,
		SourceCharacter,
		GetInitialCenter(SourceCharacter, Params),
		Params.InitialCheckRadius,
		Params.CollisionProfileName,
		Params.ParryTraceStatName,
		Overlaps);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		UAbilitySystemComponent* TargetASC = TargetActor
			? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor)
			: nullptr;
		if (TargetASC && TargetASC->HasMatchingGameplayTag(ParryingTag))
		{
			return true;
		}
	}

	return false;
}

void DLMeleeHitUtils::FindTargetsInCone(
	const ABaseCharacter& SourceCharacter,
	const FMeleeHitQueryParams& Params,
	TArray<AActor*>& OutTargets)
{
	OutTargets.Reset();

	UWorld* World = SourceCharacter.GetWorld();
	if (!World)
	{
		return;
	}

	const FVector SourceLocation = SourceCharacter.GetActorLocation();
	const FVector Forward = SourceCharacter.GetActorForwardVector().GetSafeNormal();
	const FVector InitialCenter = GetInitialCenter(SourceCharacter, Params);
	const bool bHasInitialOverlap = HasInitialOverlap(SourceCharacter, Params);
	const float ConeAngleRadians = FMath::DegreesToRadians(Params.ConeHalfAngleDegrees);

	if (Params.bShowDebug)
	{
		DrawDebugSphere(
			World,
			InitialCenter,
			Params.InitialCheckRadius,
			16,
			bHasInitialOverlap ? Params.DebugColor : FColor::Silver,
			false,
			Params.DebugDrawDuration,
			0,
			Params.DebugDrawThickness);
		DrawDebugCone(
			World,
			SourceLocation,
			Forward,
			Params.ConeRange,
			ConeAngleRadians,
			ConeAngleRadians,
			24,
			bHasInitialOverlap ? Params.DebugColor : FColor::Silver,
			false,
			Params.DebugDrawDuration,
			0,
			Params.DebugDrawThickness);
	}

	if (!bHasInitialOverlap)
	{
		return;
	}

	const float CosHalfAngle = FMath::Cos(ConeAngleRadians);
	const FVector ConeCenter = SourceLocation + Forward * (Params.ConeRange * 0.5f);

	TArray<FOverlapResult> ConeOverlaps;
	QueryOverlaps(
		*World,
		SourceCharacter,
		ConeCenter,
		Params.ConeRange,
		Params.CollisionProfileName,
		Params.ConeTraceStatName,
		ConeOverlaps);

	TSet<AActor*> AddedActors;
	for (const FOverlapResult& Overlap : ConeOverlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || TargetActor == &SourceCharacter || AddedActors.Contains(TargetActor))
		{
			continue;
		}

		const FVector ToTarget = TargetActor->GetActorLocation() - SourceLocation;
		const float Distance = ToTarget.Size();
		if (Distance > Params.ConeRange || Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (FVector::DotProduct(Forward, ToTarget / Distance) < CosHalfAngle)
		{
			continue;
		}

		OutTargets.Add(TargetActor);
		AddedActors.Add(TargetActor);
	}
}
