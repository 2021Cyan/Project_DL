#pragma once

#include "Components/SplineComponent.h"

namespace DLZiplineRide
{
	// Caller chooses live actor config or the task's activation snapshot explicitly.
	inline FVector GetLocation(const USplineComponent* Spline, float Distance, const FVector& RiderOffset)
	{
		return Spline ? Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World) + RiderOffset : RiderOffset;
	}
	inline FRotator GetRotation(const USplineComponent* Spline, float Distance, float Direction)
	{
		const FVector Tangent = Spline ? Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World) : FVector::ForwardVector;
		return (Tangent * FMath::Sign(Direction)).Rotation();
	}
}
