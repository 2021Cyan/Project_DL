// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayActor.h"
#include "Doorman/Doorway/DoorwayPortalRenderComponent.h"
#include "Doorman/Doorway/DoorwayPortalMath.h"
#include "Doorman/Doorway/DoorwayPortalRenderer.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"

void ADoorwayActor::InitializePortal()
{
	if (PortalRenderComponent)
	{
		PortalRenderComponent->InitializePortal(*this);
	}
}

FVector ADoorwayActor::TransformLocationThroughPortal(const FVector& WorldLocation) const
{
	if (!IsValid(LinkedDoorway))
	{
		return WorldLocation;
	}

	return DoorwayPortalMath::TransformLocation(
		GetRenderPlaneFrame(),
		LinkedDoorway->GetRenderPlaneFrame(),
		WorldLocation);
}

FVector ADoorwayActor::TransformDirectionThroughPortal(const FVector& WorldDirection) const
{
	if (!IsValid(LinkedDoorway))
	{
		return WorldDirection;
	}

	return DoorwayPortalMath::TransformDirection(
		GetRenderPlaneFrame(),
		LinkedDoorway->GetRenderPlaneFrame(),
		WorldDirection);
}

FRotator ADoorwayActor::TransformRotationThroughPortal(const FRotator& WorldRotation) const
{
	if (!IsValid(LinkedDoorway))
	{
		return WorldRotation;
	}

	return DoorwayPortalMath::TransformRotation(
		GetRenderPlaneFrame(),
		LinkedDoorway->GetRenderPlaneFrame(),
		WorldRotation);
}

FVector ADoorwayActor::ConvertLocationToLinkedPortal(const FVector& WorldLocation) const
{
	return TransformLocationThroughPortal(WorldLocation);
}

FRotator ADoorwayActor::ConvertRotationToLinkedPortal(const FRotator& WorldRotation) const
{
	return TransformRotationThroughPortal(WorldRotation);
}

FVector ADoorwayActor::ConvertTeleportDirectionToLinkedPortal(const FVector& WorldDirection) const
{
	return TransformTeleportDirection(WorldDirection);
}

FRotator ADoorwayActor::ConvertTeleportRotationToLinkedPortal(const FRotator& WorldRotation) const
{
	return TransformTeleportRotation(WorldRotation);
}

FVector ADoorwayActor::ConvertTeleportLocationToLinkedPortal(const FVector& WorldLocation) const
{
	return TransformTeleportLocation(WorldLocation);
}

FVector ADoorwayActor::ConvertVelocityToLinkedPortal(FVector OldVelocity) const
{
	return UpdateVelocity(OldVelocity);
}

FVector ADoorwayActor::GetTeleportPlaneLocation() const
{
	return GetActorLocation() + GetPortalForwardVector() * TeleportPlaneOffset;
}

FVector ADoorwayActor::GetPortalForwardVector() const
{
	return ForwardDirection ? ForwardDirection->GetForwardVector() : GetActorForwardVector();
}

DoorwayPortalMath::FDoorwayPortalFrame ADoorwayActor::GetRenderPlaneFrame() const
{
	const FQuat PortalRotation = ForwardDirection
		? ForwardDirection->GetComponentQuat()
		: GetActorQuat();

	return DoorwayPortalMath::FDoorwayPortalFrame::FromTransform(
		FTransform(PortalRotation, GetActorLocation(), GetActorScale3D()));
}

DoorwayPortalMath::FDoorwayPortalFrame ADoorwayActor::GetTeleportCrossingFrame() const
{
	DoorwayPortalMath::FDoorwayPortalFrame Frame = GetRenderPlaneFrame();
	Frame.SetLocation(GetTeleportPlaneLocation());
	return Frame;
}

DoorwayPortalMath::FDoorwayPortalFrame ADoorwayActor::GetTraversableTraceFrame() const
{
	DoorwayPortalMath::FDoorwayPortalFrame Frame = GetTeleportCrossingFrame();
	if (PlaneIntersectionTest)
	{
		Frame.SetLocation(PlaneIntersectionTest->GetComponentLocation());
	}
	return Frame;
}

DoorwayPortalMath::FDoorwayPortalFrame ADoorwayActor::GetSceneCaptureClipFrame() const
{
	DoorwayPortalMath::FDoorwayPortalFrame Frame = GetRenderPlaneFrame();
	if (DoorMesh)
	{
		constexpr float ClipPlaneInset = 3.0f;
		Frame.SetLocation(DoorMesh->GetComponentLocation() - Frame.PlaneNormal * ClipPlaneInset);
	}
	return Frame;
}

FVector ADoorwayActor::TransformTeleportLocation(const FVector& WorldLocation) const
{
	if (!ForwardDirection || !IsValid(LinkedDoorway) || !LinkedDoorway->ForwardDirection)
	{
		return WorldLocation;
	}

	return TransformLocationThroughPortal(WorldLocation)
		+ LinkedDoorway->ForwardDirection->GetForwardVector() * LinkedDoorway->ExitSafeDistance;
}

FVector ADoorwayActor::TransformTeleportDirection(const FVector& WorldDirection) const
{
	if (!ForwardDirection || !IsValid(LinkedDoorway) || !LinkedDoorway->ForwardDirection)
	{
		return WorldDirection;
	}

	return TransformDirectionThroughPortal(WorldDirection);
}

FRotator ADoorwayActor::TransformTeleportRotation(const FRotator& WorldRotation) const
{
	return TransformRotationThroughPortal(WorldRotation);
}

FVector ADoorwayActor::UpdateVelocity(FVector OldVelocity) const
{
	if (!IsValid(LinkedDoorway))
	{
		return OldVelocity;
	}

	return DoorwayPortalMath::PreserveSpeedThroughPortal(
		GetRenderPlaneFrame(),
		LinkedDoorway->GetRenderPlaneFrame(),
		OldVelocity);
}
