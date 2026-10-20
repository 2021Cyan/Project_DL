#pragma once

#include "CoreMinimal.h"

class ABaseCharacter;
class UWorld;

struct FMeleeHitQueryParams
{
	FName CollisionProfileName = TEXT("EnemyHit");
	float InitialCheckRadius = 0.f;
	float InitialCheckForwardOffset = 0.f;
	float ConeRange = 0.f;
	float ConeHalfAngleDegrees = 0.f;
	bool bShowDebug = false;
	float DebugDrawDuration = 0.f;
	float DebugDrawThickness = 0.f;
	FColor DebugColor = FColor::White;
	FName InitialTraceStatName = TEXT("MeleeInitialCheck");
	FName ConeTraceStatName = TEXT("MeleeConeCheck");
	FName ParryTraceStatName = TEXT("MeleeParryCheck");
};

namespace DLMeleeHitUtils
{
	bool HasInitialOverlap(const ABaseCharacter& SourceCharacter, const FMeleeHitQueryParams& Params);
	bool HasParryingTargetInInitialCheck(const ABaseCharacter& SourceCharacter, const FMeleeHitQueryParams& Params);
	void FindTargetsInCone(const ABaseCharacter& SourceCharacter, const FMeleeHitQueryParams& Params, TArray<AActor*>& OutTargets);
}
