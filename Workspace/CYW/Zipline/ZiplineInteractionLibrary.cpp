#include "Zipline/ZiplineInteractionLibrary.h"

#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Zipline/ZiplineActor.h"

FZiplineQueryView FZiplineQueryView::FromCharacter(const ACharacter& Character)
{
	FZiplineQueryView View;
	FRotator Rotation;
	Character.GetActorEyesViewPoint(View.EyeLocation, Rotation);
	View.ViewDirection = Rotation.Vector().GetSafeNormal();
	return View;
}

bool FZiplineInteractionLibrary::BuildInteractionResult(
	const ACharacter* Character,
	AZiplineActor* Zipline,
	FZiplineInteractionResult& OutResult
)
{
	OutResult = FZiplineInteractionResult();
	if (!IsValid(Character)) return false;
	const FZiplineQueryView View = FZiplineQueryView::FromCharacter(*Character);
	return BuildInteractionResult(Character, Zipline, View, OutResult);
}

bool FZiplineInteractionLibrary::BuildInteractionResult(
	const ACharacter* Character,
	AZiplineActor* Zipline,
	const FZiplineQueryView& View,
	FZiplineInteractionResult& OutResult
)
{
	OutResult = FZiplineInteractionResult();

	if (!IsValid(Character) || !IsValid(Zipline) || !Zipline->CanInteractWith(Character))
	{
		return false;
	}

	float InteractionDistanceAlong = 0.0f;
	if (!Zipline->CanReachInteractionPoint(Character, InteractionDistanceAlong))
	{
		return false;
	}

	float MountDistanceAlong = 0.0f;
	if (!Zipline->FindMountDistanceAlongSpline(Character, InteractionDistanceAlong, MountDistanceAlong, View.EyeLocation, View.ViewDirection))
	{
		return false;
	}

	const FVector InteractionPoint = Zipline->GetSpline()->GetLocationAtDistanceAlongSpline(
		InteractionDistanceAlong,
		ESplineCoordinateSpace::World
	);
	const FVector MountPoint = Zipline->GetSpline()->GetLocationAtDistanceAlongSpline(
		MountDistanceAlong,
		ESplineCoordinateSpace::World
	);
	const FVector ToMountPoint = MountPoint - View.EyeLocation;
	const float ViewDot = FVector::DotProduct(View.ViewDirection, ToMountPoint.GetSafeNormal());
	if (!FMath::IsFinite(ViewDot) || !FMath::IsFinite(Zipline->MinimumViewDot) || ViewDot < Zipline->MinimumViewDot)
	{
		return false;
	}

	OutResult.Zipline = Zipline;
	OutResult.InteractionDistanceAlongSpline = InteractionDistanceAlong;
	OutResult.MountDistanceAlongSpline = MountDistanceAlong;
	OutResult.ViewDot = ViewDot;
	OutResult.InteractionPoint = InteractionPoint;
	OutResult.MountPoint = MountPoint;
	return true;
}

bool FZiplineInteractionLibrary::FindBestZipline(
	const ACharacter* Character,
	FZiplineInteractionResult& OutResult
)
{
	OutResult = FZiplineInteractionResult();

	if (!Character || !Character->GetWorld())
	{
		return false;
	}

	const FZiplineQueryView View = FZiplineQueryView::FromCharacter(*Character);
	FZiplineInteractionResult CandidateResult;
	for (TActorIterator<AZiplineActor> It(Character->GetWorld()); It; ++It)
	{
		if (BuildInteractionResult(Character, *It, View, CandidateResult)
			&& CandidateResult.ViewDot > OutResult.ViewDot)
		{
			OutResult = CandidateResult;
		}
	}

	return OutResult.IsValid();
}

AZiplineActor* FZiplineInteractionLibrary::FindBestZipline(const ACharacter* Character)
{
	FZiplineInteractionResult Result;
	return FindBestZipline(Character, Result) ? Result.Zipline : nullptr;
}
