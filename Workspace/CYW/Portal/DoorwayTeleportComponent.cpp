// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayTeleportComponent.h"

UDoorwayTeleportComponent::UDoorwayTeleportComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDoorwayTeleportComponent::TickServer(ADoorwayActor& Doorway)
{
	Handler.TickServer(Doorway);
}

void UDoorwayTeleportComponent::AddInitialOverlap(AActor* Actor)
{
	Handler.AddInitialOverlap(Actor);
}

void UDoorwayTeleportComponent::OnPlayerNearbyBeginOverlap(ADoorwayActor& Doorway, AActor* OtherActor)
{
	Handler.OnPlayerNearbyBeginOverlap(Doorway, OtherActor);
}

void UDoorwayTeleportComponent::OnPlayerNearbyEndOverlap(AActor* OtherActor)
{
	Handler.OnPlayerNearbyEndOverlap(OtherActor);
}

bool UDoorwayTeleportComponent::TeleportTraversableFromTrace(
	ADoorwayActor& Doorway,
	AActor* TraversableActor,
	const FVector& SourceLocation)
{
	return Handler.TeleportTraversableFromTrace(Doorway, TraversableActor, SourceLocation);
}
