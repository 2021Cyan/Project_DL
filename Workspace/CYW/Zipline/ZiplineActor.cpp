#include "ZiplineActor.h"
#include "Zipline/ZiplineRideMath.h"

#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Common/PlayerViewSupport.h"
#include "GameCore/Character/ADLPlayerState.h"
#include "Zipline/ZiplineInteractionLibrary.h"
#include "Zipline/ZiplineMountIndicatorComponent.h"
#include "Zipline/ZiplineSplineVisualComponent.h"

AZiplineActor::AZiplineActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("ZiplineSpline"));
	SetRootComponent(Spline);

	SplineVisualComponent = CreateDefaultSubobject<UZiplineSplineVisualComponent>(TEXT("SplineVisual"));
	MountIndicatorComponent = CreateDefaultSubobject<UZiplineMountIndicatorComponent>(TEXT("MountIndicator"));
}

void AZiplineActor::BeginPlay()
{
	Super::BeginPlay();

	// Components created during construction are not guaranteed to be present
	// when a cooked map is loaded. Rebuild the cable visuals locally for every
	// rendered game instance; dedicated servers do not need visual components.
	if (GetNetMode() != NM_DedicatedServer && SplineVisualComponent)
	{
		SplineVisualComponent->Rebuild(Spline);
	}

	if (GetNetMode() == NM_DedicatedServer || !bShowDebug)
	{
		SetActorTickEnabled(false);
	}
}

void AZiplineActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SplineVisualComponent)
	{
		SplineVisualComponent->Rebuild(Spline);
	}
}

void AZiplineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ShowDebug();
}

void AZiplineActor::ShowDebug()
{
	if (!bShowDebug || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = DLPlayerView::GetLocalController(World);
	ACharacter* Character = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
	if (!Character || !PlayerController->IsLocalController())
	{
		return;
	}

	FVector EyeLocation;
	FRotator ViewRotation;
	Character->GetActorEyesViewPoint(EyeLocation, ViewRotation);

	float InteractionDistanceAlong = 0.0f;
	if (!CanReachInteractionPoint(Character, InteractionDistanceAlong))
	{
		return;
	}

	float MountDistanceAlong = 0.0f;
	if (!FindMountDistanceAlongSpline(Character, InteractionDistanceAlong, MountDistanceAlong, EyeLocation, ViewRotation.Vector().GetSafeNormal()))
	{
		return;
	}

	const FVector InteractionPoint = Spline->GetLocationAtDistanceAlongSpline(
		InteractionDistanceAlong,
		ESplineCoordinateSpace::World
	);
	const FVector MountPoint = Spline->GetLocationAtDistanceAlongSpline(
		MountDistanceAlong,
		ESplineCoordinateSpace::World
	);
	const FVector ToZipline = MountPoint - EyeLocation;
	const float ViewDot = FVector::DotProduct(ViewRotation.Vector(), ToZipline.GetSafeNormal());
	const bool bCanInteract = CanInteractWith(Character) && ViewDot >= MinimumViewDot;

	DrawDebugBox(
		World,
		InteractionPoint,
		InteractionBoxExtent,
		bCanInteract ? FColor::Green : FColor::Red,
		false,
		0.0f
	);
	DrawDebugSphere(World, InteractionPoint, 18.0f, 12, FColor::Cyan, false, 0.0f);
	DrawDebugSphere(World, MountPoint, 18.0f, 12, bCanInteract ? FColor::Green : FColor::Red, false, 0.0f);

	if (bCanInteract)
	{
		DrawDebugLine(World, EyeLocation, MountPoint, FColor::Green, false, 0.0f, 0, 3.0f);
	}
}

float AZiplineActor::FindClosestDistanceAlongSpline(const FVector& WorldLocation) const
{
	if (!Spline)
	{
		return 0.0f;
	}

	const float InputKey = Spline->FindInputKeyClosestToWorldLocation(WorldLocation);
	return Spline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
}

bool AZiplineActor::FindMountDistanceAlongSpline(
	const ACharacter* Character,
	float InteractionDistanceAlongSpline,
	float& OutDistanceAlongSpline
) const
{
	if (!IsValid(Character)) return false;
	FVector EyeLocation; FRotator ViewRotation;
	Character->GetActorEyesViewPoint(EyeLocation, ViewRotation);
	return FindMountDistanceAlongSpline(Character, InteractionDistanceAlongSpline,
		OutDistanceAlongSpline, EyeLocation, ViewRotation.Vector().GetSafeNormal());
}

bool AZiplineActor::FindMountDistanceAlongSpline(
	const ACharacter* Character,
	float InteractionDistanceAlongSpline,
	float& OutDistanceAlongSpline,
	const FVector& EyeLocation,
	const FVector& ViewDirection
) const
{
	if (!IsValid(Character) || !Spline || Spline->IsClosedLoop())
	{
		return false;
	}

	const float SplineLength = Spline->GetSplineLength();
	if (!FMath::IsFinite(SplineLength) || SplineLength <= KINDA_SMALL_NUMBER
		|| !FMath::IsFinite(InteractionDistance) || InteractionDistance <= 0.f
		|| !FMath::IsFinite(MountSearchDistanceAlongSpline) || MountSearchDistanceAlongSpline <= 0.f)
	{
		return false;
	}

	if (ViewDirection.ContainsNaN() || EyeLocation.ContainsNaN() || ViewDirection.IsNearlyZero())
	{
		return false;
	}

	const float SearchStartDistance = FMath::Clamp(
		InteractionDistanceAlongSpline - MountSearchDistanceAlongSpline,
		0.0f,
		SplineLength
	);
	const float SearchEndDistance = FMath::Clamp(
		InteractionDistanceAlongSpline + MountSearchDistanceAlongSpline,
		0.0f,
		SplineLength
	);
	const float SearchLength = SearchEndDistance - SearchStartDistance;
	if (SearchLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const int32 SampleCount = FMath::Clamp(FMath::CeilToInt(SearchLength / 50.0f), 16, 256);
	float BestDistanceAlongSpline = 0.0f;
	float BestViewRayDistanceSq = TNumericLimits<float>::Max();

	for (int32 SampleIndex = 0; SampleIndex <= SampleCount; ++SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) / SampleCount;
		const float DistanceAlongSpline = FMath::Lerp(SearchStartDistance, SearchEndDistance, Alpha);
		const FVector SplineLocation = Spline->GetLocationAtDistanceAlongSpline(
			DistanceAlongSpline,
			ESplineCoordinateSpace::World
		);
		const FVector ToSpline = SplineLocation - EyeLocation;
		const float ForwardDistance = FVector::DotProduct(ToSpline, ViewDirection);

		if (ForwardDistance < 0.0f || ForwardDistance > InteractionDistance)
		{
			continue;
		}

		const FVector ClosestPointOnViewRay = EyeLocation + ViewDirection * ForwardDistance;
		const float ViewRayDistanceSq = FVector::DistSquared(SplineLocation, ClosestPointOnViewRay);
		if (ViewRayDistanceSq < BestViewRayDistanceSq)
		{
			BestViewRayDistanceSq = ViewRayDistanceSq;
			BestDistanceAlongSpline = DistanceAlongSpline;
		}
	}

	if (BestViewRayDistanceSq == TNumericLimits<float>::Max())
	{
		return false;
	}

	OutDistanceAlongSpline = BestDistanceAlongSpline;
	return true;
}

bool AZiplineActor::CanReachInteractionPoint(
	const ACharacter* Character,
	float& OutDistanceAlongSpline
) const
{
	if (!IsValid(Character) || !Spline || Spline->IsClosedLoop())
	{
		return false;
	}

	OutDistanceAlongSpline = FindClosestDistanceAlongSpline(Character->GetActorLocation());
	const FVector InteractionPoint = Spline->GetLocationAtDistanceAlongSpline(
		OutDistanceAlongSpline,
		ESplineCoordinateSpace::World
	);
	const FVector OffsetFromInteractionPoint = Character->GetActorLocation() - InteractionPoint;

	return FMath::Abs(OffsetFromInteractionPoint.X) <= InteractionBoxExtent.X
		&& FMath::Abs(OffsetFromInteractionPoint.Y) <= InteractionBoxExtent.Y
		&& FMath::Abs(OffsetFromInteractionPoint.Z) <= InteractionBoxExtent.Z;
}

bool AZiplineActor::CanInteractWith(const ACharacter* Character) const
{
	if (!bRestrictInteractionToTeam)
	{
		return true;
	}

	const ADLPlayerState* PlayerState = Character ? Character->GetPlayerState<ADLPlayerState>() : nullptr;
	return PlayerState && PlayerState->IsSameTeam(this);
}

FVector AZiplineActor::GetRideLocation(float DistanceAlongSpline) const
{
	return DLZiplineRide::GetLocation(Spline, DistanceAlongSpline, RiderOffset);
}

FRotator AZiplineActor::GetRideRotation(float DistanceAlongSpline, float Direction) const
{
	return DLZiplineRide::GetRotation(Spline, DistanceAlongSpline, Direction);
}

float AZiplineActor::GetSplineLength() const
{
	return Spline ? Spline->GetSplineLength() : 0.0f;
}

FZiplineRideParams AZiplineActor::MakeRideParams() const
{
	FZiplineRideParams Params;
	Params.RideSpeed = FMath::IsFinite(RideSpeed) ? FMath::Max(0.f, RideSpeed) : 0.f;
	Params.MountApproachDuration = FMath::IsFinite(MountApproachDuration) ? FMath::Max(0.f, MountApproachDuration) : 0.f;
	Params.RiderOffset = RiderOffset;
	Params.SplineLength = GetSplineLength();
	return Params;
}

AZiplineActor* AZiplineActor::FindViewedZipline(const ACharacter* Character)
{
	return FZiplineInteractionLibrary::FindBestZipline(Character);
}
