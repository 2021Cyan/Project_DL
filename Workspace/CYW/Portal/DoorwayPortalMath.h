// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

namespace DoorwayPortalMath
{
	constexpr ECollisionChannel TraversalTraceChannel = ECC_GameTraceChannel6;

	struct FDoorwayPortalFrame
	{
		FTransform PlaneTransform = FTransform::Identity;
		FVector PlaneLocation = FVector::ZeroVector;
		FVector PlaneNormal = FVector::ForwardVector;

		static FDoorwayPortalFrame FromTransform(const FTransform& Transform)
		{
			FDoorwayPortalFrame Frame;
			Frame.PlaneTransform = Transform;
			Frame.PlaneLocation = Transform.GetLocation();
			Frame.PlaneNormal = Transform.GetUnitAxis(EAxis::X);
			return Frame;
		}

		void SetLocation(const FVector& Location)
		{
			PlaneLocation = Location;
			PlaneTransform.SetLocation(Location);
		}
	};

	FVector TransformLocation(
		const FTransform& SourcePlaneTransform,
		const FTransform& DestinationPlaneTransform,
		const FVector& WorldLocation);

	FVector TransformLocation(
		const FDoorwayPortalFrame& SourceFrame,
		const FDoorwayPortalFrame& DestinationFrame,
		const FVector& WorldLocation);

	FVector TransformDirection(
		const FTransform& SourcePlaneTransform,
		const FTransform& DestinationPlaneTransform,
		const FVector& WorldDirection);

	FVector TransformDirection(
		const FDoorwayPortalFrame& SourceFrame,
		const FDoorwayPortalFrame& DestinationFrame,
		const FVector& WorldDirection);

	FRotator TransformRotation(
		const FTransform& SourcePlaneTransform,
		const FTransform& DestinationPlaneTransform,
		const FRotator& WorldRotation);

	FRotator TransformRotation(
		const FDoorwayPortalFrame& SourceFrame,
		const FDoorwayPortalFrame& DestinationFrame,
		const FRotator& WorldRotation);

	FVector PreserveSpeedThroughPortal(
		const FTransform& SourcePlaneTransform,
		const FTransform& DestinationPlaneTransform,
		const FVector& Velocity);

	FVector PreserveSpeedThroughPortal(
		const FDoorwayPortalFrame& SourceFrame,
		const FDoorwayPortalFrame& DestinationFrame,
		const FVector& Velocity);
}
