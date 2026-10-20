// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Doorman/Doorway/DoorwayCameraTransition.h"
#include "DoorwayCameraTransitionComponent.generated.h"

class ADoorwayActor;

UCLASS(ClassGroup = (Doorway), meta = (BlueprintSpawnableComponent))
class TEAMPROJECT_DL_API UDoorwayCameraTransitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorwayCameraTransitionComponent();

	void TickCameraTransition(ADoorwayActor& Doorway, float DeltaSeconds);
	void DesyncCameraAndCharacter(ADoorwayActor& Doorway);
	void UpdateCameraSynchronization(ADoorwayActor& Doorway);
	void BeginFromTeleport(ADoorwayActor& Doorway, const FRotator& DestinationControlRotation);
	void FinishOwnerIfAny();
	bool CacheLocalReferences(ADoorwayActor& Doorway);
	APlayerCameraManager* GetCachedLocalCameraManager() const;
	void UpdateLinkedCamera(ADoorwayActor& Doorway);
	bool IsTransitionOwnedBy(const ADoorwayActor& Doorway) const;
	void FinishTransition(ADoorwayActor& Doorway);
	void SetTransitionOwner(ADoorwayActor& Doorway);
	void SetTransitionIdle();
	void SetTransitionWaitingForDestination();
	void ClearTransitionOwner();

private:
	FDoorwayCameraTransition Transition;
};
