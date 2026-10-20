#include "Weapon/ProjectileAimLibrary.h"

#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameCore/Struct/ACharacterAttributeSet.h"
#include "GameCore/Struct/DLCollisionTypes.h"
#include "GameFramework/Controller.h"
#include "TeamUtils.h"
#include "Doorman/Doorway/DoorwayActor.h"
#include "Common/PlayerViewSupport.h"

namespace
{
	FCollisionObjectQueryParams MakeAimObjectParams(const ABaseCharacter& Character)
	{
		FCollisionObjectQueryParams Params;
		Params.AddObjectTypesToQuery(ECC_WorldStatic);
		Params.AddObjectTypesToQuery(ECC_AimTarget);
		const int32 TeamIndex = SkillTeamUtils::GetTeamIdx(&Character);
		if (TeamIndex != INDEX_NONE)
		{
			Params.AddObjectTypesToQuery((TeamIndex % 2) == 0 ? ECC_Team2 : ECC_Team1);
		}
		return Params;
	}

	FVector TraceAimTarget(UWorld& World, const ABaseCharacter& Character,
		const FVector& Start, const FVector& End,
		const FCollisionObjectQueryParams& ObjectParams,
		const FCollisionQueryParams& QueryParams)
	{
		TArray<FHitResult> Hits;
		World.LineTraceMultiByObjectType(Hits, Start, End, ObjectParams, QueryParams);
		Hits.Sort([](const FHitResult& Left, const FHitResult& Right)
		{
			return Left.Distance < Right.Distance;
		});
		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			// Doorway query volumes can be WorldStatic in inherited BPs. They
			// detect traversal, but are not geometry visible through the portal.
			if (!HitActor || HitActor == &Character || HitActor->IsA<ADoorwayActor>())
			{
				continue;
			}
			const ECollisionChannel HitType = Hit.Component.IsValid()
				? Hit.Component->GetCollisionObjectType() : ECC_WorldDynamic;
			if (HitType == ECC_WorldStatic || HitType == ECC_AimTarget
				|| SkillTeamUtils::IsHostile(&Character, HitActor))
			{
				return Hit.ImpactPoint;
			}
		}
		return End;
	}

	bool TryResolvePortalTarget(UWorld& World, const ABaseCharacter& Character,
		const DLPlayerView::FAimView& View, float TraceDistance,
		const FCollisionObjectQueryParams& ObjectParams,
		const FCollisionQueryParams& QueryParams, DLProjectileAim::FShotAim& Aim)
	{
		FHitResult PortalHit;
		const FVector TraceEnd = View.Location + View.Rotation.Vector() * TraceDistance;
		if (!World.LineTraceSingleByChannel(PortalHit, View.Location, TraceEnd,
			DoorwayPortalMath::TraversalTraceChannel, QueryParams))
		{
			return false;
		}
		ADoorwayActor* Entrance = Cast<ADoorwayActor>(PortalHit.GetActor());
		ADoorwayActor* Exit = IsValid(Entrance) ? Entrance->GetLinkedDoorway() : nullptr;
		if (!IsValid(Exit) || Exit->GetLinkedDoorway() != Entrance
			|| PortalHit.Distance > FVector::Distance(View.Location, Aim.WorldTarget))
		{
			return false;
		}

		// Match the capture camera's position and rotation, including scaled portal locations.
		const FVector ExitCameraLocation = Entrance->ConvertLocationToLinkedPortal(View.Location);
		const FVector ExitDirection = Entrance->ConvertRotationToLinkedPortal(View.Rotation).Vector();
		const auto ClipFrame = Exit->GetSceneCaptureClipFrame();
		const float ForwardDot = FVector::DotProduct(ExitDirection, ClipFrame.PlaneNormal);
		if (ForwardDot <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		// Trace only the space the projectile can reach after exit clearance.
		// Advance along the capture ray, retaining its target rather than shifting sideways.
		const auto TraversalFrame = Exit->GetTraversableTraceFrame();
		constexpr float ExitPlaneTraceInset = 1.f;
		const float ClipPlaneDistance = FVector::DotProduct(
			ClipFrame.PlaneLocation - ExitCameraLocation, ClipFrame.PlaneNormal);
		const float TraversalPlaneDistance = FVector::DotProduct(
			TraversalFrame.PlaneLocation - ExitCameraLocation, ClipFrame.PlaneNormal)
			+ FMath::Max(0.f, Exit->GetExitSafeDistance());
		const float ExitAdvance = FMath::Max(0.f,
			FMath::Max(ClipPlaneDistance, TraversalPlaneDistance) + ExitPlaneTraceInset) / ForwardDot;
		const float RemainingDistance = TraceDistance - PortalHit.Distance;
		if (RemainingDistance <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		const FVector ExitStart = ExitCameraLocation + ExitDirection * ExitAdvance;
		FCollisionQueryParams ExitQueryParams = QueryParams;
		ExitQueryParams.AddIgnoredActor(Entrance);
		ExitQueryParams.AddIgnoredActor(Exit);
		Aim.WorldTarget = TraceAimTarget(World, Character, ExitStart,
			ExitStart + ExitDirection * RemainingDistance, ObjectParams, ExitQueryParams);
		Aim.MuzzleAimPoint = Exit->ConvertLocationToLinkedPortal(Aim.WorldTarget);
		Aim.Portal.Entrance = Entrance;
		Aim.Portal.Exit = Exit;
		Aim.Portal.Target = Aim.WorldTarget;
		return true;
	}
}

FVector DLProjectileAim::GetSpawnLocation(
	ABaseCharacter& Character,
	const FName& SpawnSocketName,
	float SpawnForwardOffset)
{
	const USkeletalMeshComponent* CharacterMesh = Character.GetMesh();
	const bool bUseMuzzleSocket = CharacterMesh
		&& SpawnSocketName != NAME_None
		&& CharacterMesh->DoesSocketExist(SpawnSocketName);
	if (bUseMuzzleSocket)
	{
		return CharacterMesh->GetSocketLocation(SpawnSocketName);
	}

	const FRotator AimRotation = Character.GetBaseAimRotation();
	return Character.GetActorLocation() + AimRotation.Vector() * SpawnForwardOffset + FVector(0.f, 0.f, 60.f);
}

DLProjectileAim::FShotAim DLProjectileAim::ResolveShotAim(
	ABaseCharacter& Character,
	const FVector& SpawnLocation,
	const UAbilitySystemComponent* ASC,
	float TraceDistance,
	float MaxAccuracyPenaltySpreadDegrees)
{
	const DLPlayerView::FAimView View = DLPlayerView::GetAimView(Character);
	FShotAim Aim;
	Aim.WorldTarget = View.Location + View.Rotation.Vector() * TraceDistance;
	Aim.MuzzleAimPoint = Aim.WorldTarget;
	if (UWorld* World = Character.GetWorld())
	{
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(&Character);
		const FCollisionObjectQueryParams ObjectParams = MakeAimObjectParams(Character);
		Aim.WorldTarget = TraceAimTarget(*World, Character, View.Location,
			Aim.WorldTarget, ObjectParams, QueryParams);
		Aim.MuzzleAimPoint = Aim.WorldTarget;
		TryResolvePortalTarget(*World, Character, View, TraceDistance,
			ObjectParams, QueryParams, Aim);
	}
	const FVector UnspreadDirection = (Aim.MuzzleAimPoint - SpawnLocation).GetSafeNormal();
	Aim.Portal.UnspreadDirection = UnspreadDirection;
	Aim.LaunchDirection = ApplyAccuracySpread(UnspreadDirection, ASC, MaxAccuracyPenaltySpreadDegrees);
	return Aim;
}

FVector DLProjectileAim::ApplyAccuracySpread(
	const FVector& FireDirection,
	const UAbilitySystemComponent* ASC,
	float MaxAccuracyPenaltySpreadDegrees)
{
	if (!ASC || FireDirection.IsNearlyZero())
	{
		return FireDirection;
	}

	const float AccuracyMultiplier = ASC->GetNumericAttribute(
		UACharacterAttributeSet::GetWeaponAccuracyMultiplierAttribute());
	const float AccuracyPenalty = FMath::Clamp(-AccuracyMultiplier, 0.f, 1.f);
	const float SpreadDegrees = AccuracyPenalty * MaxAccuracyPenaltySpreadDegrees;
	if (SpreadDegrees <= KINDA_SMALL_NUMBER)
	{
		return FireDirection;
	}

	return FMath::VRandCone(FireDirection, FMath::DegreesToRadians(SpreadDegrees)).GetSafeNormal();
}
