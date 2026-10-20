// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayCameraTransitionComponent.h"

UDoorwayCameraTransitionComponent::UDoorwayCameraTransitionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDoorwayCameraTransitionComponent::TickCameraTransition(ADoorwayActor& Doorway, float DeltaSeconds)
{
	Transition.Tick(Doorway, DeltaSeconds);
}

void UDoorwayCameraTransitionComponent::DesyncCameraAndCharacter(ADoorwayActor& Doorway)
{
	Transition.DesyncCameraAndCharacter(Doorway);
}

void UDoorwayCameraTransitionComponent::UpdateCameraSynchronization(ADoorwayActor& Doorway)
{
	Transition.UpdateCameraSynchronization(Doorway);
}

void UDoorwayCameraTransitionComponent::BeginFromTeleport(
	ADoorwayActor& Doorway,
	const FRotator& DestinationControlRotation)
{
	Transition.BeginFromTeleport(Doorway, DestinationControlRotation);
}

void UDoorwayCameraTransitionComponent::FinishOwnerIfAny()
{
	Transition.FinishOwnerIfAny();
}

bool UDoorwayCameraTransitionComponent::CacheLocalReferences(ADoorwayActor& Doorway)
{
	return Transition.CacheLocalReferences(Doorway);
}

APlayerCameraManager* UDoorwayCameraTransitionComponent::GetCachedLocalCameraManager() const
{
	return Transition.GetCachedLocalCameraManager();
}

void UDoorwayCameraTransitionComponent::UpdateLinkedCamera(ADoorwayActor& Doorway)
{
	Transition.UpdateLinkedCamera(Doorway);
}

bool UDoorwayCameraTransitionComponent::IsTransitionOwnedBy(const ADoorwayActor& Doorway) const
{
	return Transition.IsOwnedBy(Doorway);
}

void UDoorwayCameraTransitionComponent::FinishTransition(ADoorwayActor& Doorway)
{
	Transition.Finish(Doorway);
}

void UDoorwayCameraTransitionComponent::SetTransitionOwner(ADoorwayActor& Doorway)
{
	Transition.SetOwner(Doorway);
}

void UDoorwayCameraTransitionComponent::SetTransitionIdle()
{
	Transition.SetIdle();
}

void UDoorwayCameraTransitionComponent::SetTransitionWaitingForDestination()
{
	Transition.SetWaitingForDestination();
}

void UDoorwayCameraTransitionComponent::ClearTransitionOwner()
{
	Transition.ClearOwner();
}
