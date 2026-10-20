// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayActor.h"
#include "Doorman/Doorway/DoorwayTeleportComponent.h"
#include "Doorman/Doorway/DoorwayTeleportHandler.h"

void ADoorwayActor::OnPlayerNearbyBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (TeleportComponent)
	{
		TeleportComponent->OnPlayerNearbyBeginOverlap(*this, OtherActor);
	}
}

void ADoorwayActor::OnPlayerNearbyEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	if (TeleportComponent)
	{
		TeleportComponent->OnPlayerNearbyEndOverlap(OtherActor);
	}
}

bool ADoorwayActor::TeleportTraversableFromTrace(
	AActor* TraversableActor,
	const FVector& SourceLocation)
{
	return TeleportComponent
		&& TeleportComponent->TeleportTraversableFromTrace(*this, TraversableActor, SourceLocation);
}
