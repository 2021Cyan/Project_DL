// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Doorman/Doorway/DoorwayTeleportHandler.h"
#include "DoorwayTeleportComponent.generated.h"

class ADoorwayActor;

UCLASS(ClassGroup = (Doorway), meta = (BlueprintSpawnableComponent))
class TEAMPROJECT_DL_API UDoorwayTeleportComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorwayTeleportComponent();

	void TickServer(ADoorwayActor& Doorway);
	void AddInitialOverlap(AActor* Actor);
	void OnPlayerNearbyBeginOverlap(ADoorwayActor& Doorway, AActor* OtherActor);
	void OnPlayerNearbyEndOverlap(AActor* OtherActor);
	bool TeleportTraversableFromTrace(
		ADoorwayActor& Doorway,
		AActor* TraversableActor,
		const FVector& SourceLocation);

private:
	FDoorwayTeleportHandler Handler;
};
