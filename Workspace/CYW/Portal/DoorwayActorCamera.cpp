// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayActor.h"
#include "Doorman/Doorway/DoorwayCameraTransitionComponent.h"

void ADoorwayActor::DesyncCameraAndCharacter()
{
	if (CameraTransitionComponent)
	{
		CameraTransitionComponent->DesyncCameraAndCharacter(*this);
	}
}

void ADoorwayActor::UpdateCameraSynchronization()
{
	if (CameraTransitionComponent)
	{
		CameraTransitionComponent->UpdateCameraSynchronization(*this);
	}
}

void ADoorwayActor::BeginCameraTransitionFromTeleport(
	const FRotator& DestinationControlRotation)
{
	if (CameraTransitionComponent)
	{
		CameraTransitionComponent->BeginFromTeleport(*this, DestinationControlRotation);
	}
}

void ADoorwayActor::UpdateLinkedCamera()
{
	if (CameraTransitionComponent)
	{
		CameraTransitionComponent->UpdateLinkedCamera(*this);
	}
}
