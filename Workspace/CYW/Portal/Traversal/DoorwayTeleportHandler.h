// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AActor;
class ACharacter;
class ADoorwayActor;

struct FDoorwayCrossingState
{
	FVector LastPosition = FVector::ZeroVector;
	bool bLastInFront = false;
	bool bInitialized = false;
};

class FDoorwayTeleportHandler
{
public:
	void TickServer(ADoorwayActor& Doorway);
	void AddInitialOverlap(AActor* Actor);
	void OnPlayerNearbyBeginOverlap(ADoorwayActor& Doorway, AActor* OtherActor);
	void OnPlayerNearbyEndOverlap(AActor* OtherActor);
	bool TeleportTraversableFromTrace(
		ADoorwayActor& Doorway,
		AActor* TraversableActor,
		const FVector& SourceLocation);

private:
	void CheckCharacterCrossings(ADoorwayActor& Doorway);
	void TeleportCharacterInternal(ADoorwayActor& Doorway, ACharacter* Character);

	TMap<TWeakObjectPtr<AActor>, FDoorwayCrossingState> CrossingStates;
	TSet<TWeakObjectPtr<AActor>> NearbyActors;
};
