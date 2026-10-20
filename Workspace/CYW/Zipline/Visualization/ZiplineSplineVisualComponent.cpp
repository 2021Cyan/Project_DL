#include "Zipline/ZiplineSplineVisualComponent.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Materials/MaterialInterface.h"

UZiplineSplineVisualComponent::UZiplineSplineVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZiplineSplineVisualComponent::Rebuild(USplineComponent* Spline)
{
	Clear();

	const AActor* Owner = GetOwner();
	if (!Owner || Owner->GetNetMode() == NM_DedicatedServer || !VisualSplineMesh || !Spline)
	{
		return;
	}

	const int32 PointCount = Spline->GetNumberOfSplinePoints();
	const int32 SegmentCount = Spline->IsClosedLoop() ? PointCount : PointCount - 1;
	if (SegmentCount <= 0)
	{
		return;
	}

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const int32 StartPointIndex = SegmentIndex;
		const int32 EndPointIndex = (SegmentIndex + 1) % PointCount;
		const float StartDistance = Spline->GetDistanceAlongSplineAtSplinePoint(StartPointIndex);
		const float EndDistance = EndPointIndex == 0
			? Spline->GetSplineLength()
			: Spline->GetDistanceAlongSplineAtSplinePoint(EndPointIndex);
		const int32 SubdivisionCount = FMath::Clamp(VisualSubdivisionsPerSplineSegment, 1, 256);

		for (int32 SubdivisionIndex = 0; SubdivisionIndex < SubdivisionCount; ++SubdivisionIndex)
		{
			const float SubdivisionStartAlpha = static_cast<float>(SubdivisionIndex) / SubdivisionCount;
			const float SubdivisionEndAlpha = static_cast<float>(SubdivisionIndex + 1) / SubdivisionCount;
			const float SubdivisionStartDistance = FMath::Lerp(StartDistance, EndDistance, SubdivisionStartAlpha);
			const float SubdivisionEndDistance = FMath::Lerp(StartDistance, EndDistance, SubdivisionEndAlpha);
			const float SubdivisionLength = SubdivisionEndDistance - SubdivisionStartDistance;

			const FVector StartLocation = Spline->GetLocationAtDistanceAlongSpline(
				SubdivisionStartDistance,
				ESplineCoordinateSpace::Local
			);
			const FVector EndLocation = Spline->GetLocationAtDistanceAlongSpline(
				SubdivisionEndDistance,
				ESplineCoordinateSpace::Local
			);
			const FVector StartTangent = Spline->GetDirectionAtDistanceAlongSpline(
				SubdivisionStartDistance,
				ESplineCoordinateSpace::Local
			) * SubdivisionLength;
			const FVector EndTangent = Spline->GetDirectionAtDistanceAlongSpline(
				SubdivisionEndDistance,
				ESplineCoordinateSpace::Local
			) * SubdivisionLength;

			CreateSegment(Spline, StartLocation, StartTangent, EndLocation, EndTangent);
		}
	}
}

void UZiplineSplineVisualComponent::Clear()
{
	for (USplineMeshComponent* SplineMesh : SplineMeshComponents)
	{
		if (IsValid(SplineMesh))
		{
			SplineMesh->DestroyComponent();
		}
	}

	SplineMeshComponents.Reset();
}

void UZiplineSplineVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Clear();
	Super::EndPlay(EndPlayReason);
}

void UZiplineSplineVisualComponent::CreateSegment(
	USplineComponent* Spline,
	const FVector& StartLocation,
	const FVector& StartTangent,
	const FVector& EndLocation,
	const FVector& EndTangent
)
{
	USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(GetOwner());
	SplineMesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	SplineMesh->SetMobility(EComponentMobility::Movable);
	SplineMesh->SetStaticMesh(VisualSplineMesh);
	SplineMesh->SetForwardAxis(ESplineMeshAxis::Z, false);
	SplineMesh->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, false);
	SplineMesh->SetStartScale(VisualMeshScale, false);
	SplineMesh->SetEndScale(VisualMeshScale, false);
	SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SplineMesh->SetCastShadow(false);

	if (VisualMaterial)
	{
		SplineMesh->SetMaterial(0, VisualMaterial);
	}

	SplineMesh->SetupAttachment(Spline);
	SplineMesh->RegisterComponent();
	SplineMeshComponents.Add(SplineMesh);
}
