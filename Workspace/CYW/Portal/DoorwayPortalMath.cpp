// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayPortalMath.h"

#include "Kismet/KismetMathLibrary.h"
#include "Math/RotationMatrix.h"

namespace DoorwayPortalMath
{
	FVector TransformLocation(
		const FTransform& SourcePlaneTransform,
		const FTransform& DestinationPlaneTransform,
		const FVector& WorldLocation)
	{
		FVector LocalLocation = SourcePlaneTransform.InverseTransformPosition(WorldLocation);
		LocalLocation.X *= -1.0f;
		LocalLocation.Y *= -1.0f;
		return DestinationPlaneTransform.TransformPosition(LocalLocation);
	}

	FVector TransformLocation(
		const FDoorwayPortalFrame& SourceFrame,
		const FDoorwayPortalFrame& DestinationFrame,
		const FVector& WorldLocation)
	{
		return TransformLocation(
			SourceFrame.PlaneTransform,
			DestinationFrame.PlaneTransform,
			WorldLocation);
	}

	FVector TransformDirection(
		const FTransform& SourcePlaneTransform,
		const FTransform& DestinationPlaneTransform,
		const FVector& WorldDirection)
	{
		FVector LocalDirection = SourcePlaneTransform.InverseTransformVectorNoScale(WorldDirection);
		LocalDirection = FMath::GetReflectionVector(LocalDirection, FVector::ForwardVector);
		LocalDirection = FMath::GetReflectionVector(LocalDirection, FVector::RightVector);
		return DestinationPlaneTransform.TransformVectorNoScale(LocalDirection);
	}

	FVector TransformDirection(
		const FDoorwayPortalFrame& SourceFrame,
		const FDoorwayPortalFrame& DestinationFrame,
		const FVector& WorldDirection)
	{
		return TransformDirection(
			SourceFrame.PlaneTransform,
			DestinationFrame.PlaneTransform,
			WorldDirection);
	}

	FRotator TransformRotation(
		const FTransform& SourcePlaneTransform,
		const FTransform& DestinationPlaneTransform,
		const FRotator& WorldRotation)
	{
		const FRotationMatrix RotationMatrix(WorldRotation);
		return UKismetMathLibrary::MakeRotationFromAxes(
			TransformDirection(
				SourcePlaneTransform,
				DestinationPlaneTransform,
				RotationMatrix.GetScaledAxis(EAxis::X)),
			TransformDirection(
				SourcePlaneTransform,
				DestinationPlaneTransform,
				RotationMatrix.GetScaledAxis(EAxis::Y)),
			TransformDirection(
				SourcePlaneTransform,
				DestinationPlaneTransform,
				RotationMatrix.GetScaledAxis(EAxis::Z)));
	}

	FRotator TransformRotation(
		const FDoorwayPortalFrame& SourceFrame,
		const FDoorwayPortalFrame& DestinationFrame,
		const FRotator& WorldRotation)
	{
		return TransformRotation(
			SourceFrame.PlaneTransform,
			DestinationFrame.PlaneTransform,
			WorldRotation);
	}

	FVector PreserveSpeedThroughPortal(
		const FTransform& SourcePlaneTransform,
		const FTransform& DestinationPlaneTransform,
		const FVector& Velocity)
	{
		return TransformDirection(
			SourcePlaneTransform,
			DestinationPlaneTransform,
			Velocity.GetSafeNormal()) * Velocity.Size();
	}

	FVector PreserveSpeedThroughPortal(
		const FDoorwayPortalFrame& SourceFrame,
		const FDoorwayPortalFrame& DestinationFrame,
		const FVector& Velocity)
	{
		return PreserveSpeedThroughPortal(
			SourceFrame.PlaneTransform,
			DestinationFrame.PlaneTransform,
			Velocity);
	}
}
