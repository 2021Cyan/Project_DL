// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraTypes.h"

class ADoorwayActor;
class ACharacter;
class APlayerCharacter;
class UCameraComponent;
class APlayerCameraManager;
class APlayerController;

enum class EDoorwayCameraTransitionState : uint8
{
	Idle,
	WaitingForDestination,
	FollowingCharacter,
	ReturningToCharacter,
	FinishQueued
};

struct FDoorwayCameraSnapshot
{
	FMinimalViewInfo View;
	FVector BoomPivot = FVector::ZeroVector;
	float BoomArmLength = 0.0f;
	FVector BoomSocketOffset = FVector::ZeroVector;
	bool bIsValid = false;
};

struct FDoorwayCameraTransitionData
{
	EDoorwayCameraTransitionState State = EDoorwayCameraTransitionState::Idle;
	double RequestTime = 0.0;
	double ReturnStartTime = 0.0;
	FVector ReturnLocationOffset = FVector::ZeroVector;
	FQuat ReturnRotationOffset = FQuat::Identity;
	FTransform StartTransform = FTransform::Identity;
	FVector EntranceBoomPivot = FVector::ZeroVector;
	FVector DestinationBoomPivot = FVector::ZeroVector;
	FRotator DestinationControlRotation = FRotator::ZeroRotator;
	float BoomArmLength = 0.0f;
	FVector BoomSocketOffset = FVector::ZeroVector;
	FDoorwayCameraSnapshot CachedSnapshot;
	FDoorwayCameraSnapshot PendingSnapshot;
	FRotator PendingDestinationControlRotation = FRotator::ZeroRotator;
};

class FDoorwayCameraTransition
{
public:
	void Tick(ADoorwayActor& Doorway, float DeltaSeconds);
	void DesyncCameraAndCharacter(ADoorwayActor& Doorway);
	void UpdateCameraSynchronization(ADoorwayActor& Doorway);
	void ScheduleFinish(ADoorwayActor& Doorway);
	void BeginFromTeleport(ADoorwayActor& Doorway, const FRotator& DestinationControlRotation);
	void Begin(ADoorwayActor& Doorway);
	void Finish(ADoorwayActor& Doorway);
	void FinishOwnerIfAny();
	bool CacheLocalReferences(ADoorwayActor& Doorway);
	void CacheLocalPlayerCameraState(ADoorwayActor& Doorway);
	void UpdateLinkedCamera(ADoorwayActor& Doorway);
	bool IsOwnedBy(const ADoorwayActor& Doorway) const;
	bool IsIdle() const;
	APlayerCameraManager* GetCachedLocalCameraManager() const;
	void SetOwner(ADoorwayActor& Doorway);
	void SetIdle();
	void SetWaitingForDestination();
	void ClearOwner();

private:
	void UpdateReturnCamera(ADoorwayActor& Doorway, const APlayerCharacter& Character,
		UCameraComponent& Camera);
	void UpdateEntranceCamera(const APlayerCharacter& Character, const APlayerController& Controller,
		const ADoorwayActor& Destination, UCameraComponent& Camera) const;

	TWeakObjectPtr<APlayerController> CachedLocalPlayerController;
	TWeakObjectPtr<ACharacter> CachedLocalPlayerCharacter;
	TWeakObjectPtr<APlayerCameraManager> CachedLocalCameraManager;
	TWeakObjectPtr<ADoorwayActor> Owner;
	TWeakObjectPtr<ADoorwayActor> TransitionDestination;
	TWeakObjectPtr<APlayerController> TransitionController;
	TWeakObjectPtr<ACharacter> TransitionCharacter;
	TWeakObjectPtr<AActor> PreviousViewTarget;
	bool bPreviousUseControlRotationForAim = false;
	FDoorwayCameraTransitionData Data;
};
