// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayCameraTransition.h"

#include "Doorman/Doorway/DoorwayActor.h"
#include "Doorman/Doorway/DoorwayCameraTransitionComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameCore/Character/PlayerCharacter.h"
#include "Common/PlayerViewSupport.h"
#include "TimerManager.h"

void FDoorwayCameraTransition::Tick(ADoorwayActor& Doorway, float DeltaSeconds)
{
	(void)DeltaSeconds;
	CacheLocalPlayerCameraState(Doorway);
	if (IsOwnedBy(Doorway) && !IsIdle())
	{
		UpdateCameraSynchronization(Doorway);
	}
}

void FDoorwayCameraTransition::DesyncCameraAndCharacter(ADoorwayActor& Doorway)
{
	if (!CacheLocalReferences(Doorway) || !IsValid(Doorway.GetLinkedDoorway()))
	{
		return;
	}
	if (Data.State == EDoorwayCameraTransitionState::Idle)
	{
		Begin(Doorway);
		return;
	}

	if (Owner.Get() == &Doorway)
	{
		Finish(Doorway);
	}
}

void FDoorwayCameraTransition::UpdateCameraSynchronization(ADoorwayActor& Doorway)
{
	if (Owner.Get() != &Doorway || Data.State == EDoorwayCameraTransitionState::FinishQueued)
		return;
	if (!Doorway.GetWorld())
	{
		Finish(Doorway);
		return;
	}
	if (!IsValid(Doorway.GetLinkedDoorway()) || !CacheLocalReferences(Doorway)
		|| (Data.State == EDoorwayCameraTransitionState::WaitingForDestination
			&& Doorway.GetWorld()->GetTimeSeconds() - Data.RequestTime >= Doorway.GetCameraTransitionMaximumDuration()))
	{
		ScheduleFinish(Doorway);
		return;
	}

	ACharacter* PlayerCharacter = CachedLocalPlayerCharacter.Get();
	APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(PlayerCharacter);
	APlayerController* PlayerController = CachedLocalPlayerController.Get();
	ADoorwayActor* LinkedDoorway = Doorway.GetLinkedDoorway();
	UCameraComponent* PlayerCamera = Doorway.GetPlayerCamera();
	if (PlayerCharacter != TransitionCharacter.Get() || PlayerController != TransitionController.Get()
		|| !LocalPlayerCharacter || !LocalPlayerCharacter->FollowCamera || !LocalPlayerCharacter->CameraBoom
		|| !PlayerController
		|| !IsValid(LinkedDoorway) || !PlayerCamera)
	{
		ScheduleFinish(Doorway);
		return;
	}


	// Do not evaluate the exit condition until this client has received the
	// replicated teleport and the character has reached the destination doorway.
	if (Data.State == EDoorwayCameraTransitionState::WaitingForDestination)
	{
		UBoxComponent* LinkedPlayerNearbyBounds = LinkedDoorway->GetPlayerNearbyBounds();
		if (LinkedPlayerNearbyBounds
			&& LinkedPlayerNearbyBounds->IsOverlappingActor(PlayerCharacter))
		{
			Data.DestinationBoomPivot =
				LinkedDoorway->ConvertLocationToLinkedPortal(
					LocalPlayerCharacter->CameraBoom->GetComponentLocation());
			Data.State = EDoorwayCameraTransitionState::FollowingCharacter;
		}
		else
		{
			return;
		}
	}

	if (Data.State == EDoorwayCameraTransitionState::ReturningToCharacter)
	{
		UpdateReturnCamera(Doorway, *LocalPlayerCharacter, *PlayerCamera);
		return;
	}
	UpdateEntranceCamera(*LocalPlayerCharacter, *PlayerController, *LinkedDoorway, *PlayerCamera);

	const UBoxComponent* LinkedPlaneIntersectionTest = LinkedDoorway->GetPlaneIntersectionTest();
	const FVector ExitPlaneLocation = LinkedPlaneIntersectionTest
		? LinkedPlaneIntersectionTest->GetComponentLocation()
		: LinkedDoorway->GetTeleportPlaneLocation();
	const FVector ExitPlaneNormal = LinkedDoorway->GetPortalForwardVector();
	const float ArmLength = LocalPlayerCharacter->CameraBoom->TargetArmLength;
	const float ExitCharacterDistance = FMath::Abs(FVector::DotProduct(
		ExitPlaneNormal, PlayerCharacter->GetActorLocation() - ExitPlaneLocation));

	if (ExitCharacterDistance >= ArmLength)
	{
		// Preserve the entrance view until the original distance condition is met.
		// Only then map that view into destination space and smooth the final handoff.
		const UCameraComponent* FollowCamera = LocalPlayerCharacter->FollowCamera;
		const FVector ReturnLocation = Doorway.ConvertLocationToLinkedPortal(PlayerCamera->GetComponentLocation());
		const FQuat ReturnRotation = Doorway.ConvertRotationToLinkedPortal(
			PlayerCamera->GetComponentRotation()).Quaternion();
		// Character distance alone does not guarantee either camera has cleared
		// the doorway wall. Relocating a view on/behind that plane can render black.
		const auto ClipFrame = LinkedDoorway->GetSceneCaptureClipFrame();
		constexpr float CameraPlaneClearance = 5.f;
		const float ReturnCameraDistance = FVector::DotProduct(
			ReturnLocation - ClipFrame.PlaneLocation, ClipFrame.PlaneNormal);
		const float FollowCameraDistance = FVector::DotProduct(
			FollowCamera->GetComponentLocation() - ClipFrame.PlaneLocation, ClipFrame.PlaneNormal);
		if (ReturnCameraDistance < CameraPlaneClearance || FollowCameraDistance < CameraPlaneClearance)
		{
			return;
		}
		Data.ReturnLocationOffset = ReturnLocation - FollowCamera->GetComponentLocation();
		Data.ReturnRotationOffset = (ReturnRotation * FollowCamera->GetComponentQuat().Inverse()).GetNormalized();
		Data.ReturnStartTime = Doorway.GetWorld()->GetTimeSeconds();
		Data.State = EDoorwayCameraTransitionState::ReturningToCharacter;
		UpdateReturnCamera(Doorway, *LocalPlayerCharacter, *PlayerCamera);
	}
}

void FDoorwayCameraTransition::UpdateReturnCamera(ADoorwayActor& Doorway,
	const APlayerCharacter& Character, UCameraComponent& Camera)
{
	const float ReturnDuration = Doorway.GetCameraReturnBlendDuration();
	const float Alpha = ReturnDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(static_cast<float>(
			(Doorway.GetWorld()->GetTimeSeconds() - Data.ReturnStartTime) / ReturnDuration), 0.f, 1.f)
		: 1.f;
	const float SmoothAlpha = Alpha * Alpha * (3.f - 2.f * Alpha);
	const UCameraComponent* FollowCamera = Character.FollowCamera;
	const FQuat RotationOffset = FQuat::Slerp(Data.ReturnRotationOffset, FQuat::Identity, SmoothAlpha);
	Camera.SetWorldLocationAndRotation(
		FollowCamera->GetComponentLocation() + Data.ReturnLocationOffset * (1.f - SmoothAlpha),
		(RotationOffset * FollowCamera->GetComponentQuat()).GetNormalized());
	Camera.SetFieldOfView(FollowCamera->FieldOfView);
	if (Alpha >= 1.f)
	{
		ScheduleFinish(Doorway);
	}
}

void FDoorwayCameraTransition::UpdateEntranceCamera(
	const APlayerCharacter& LocalPlayerCharacter, const APlayerController& PlayerController,
	const ADoorwayActor& LinkedDoorway, UCameraComponent& PlayerCamera) const
{
	// FollowCamera's world transform can be stale for one or more frames after a
	// replicated teleport. Reconstruct the entrance camera solely from the local
	// control rotation and spring-arm settings instead.
	const FQuat BaseControl = Data.DestinationControlRotation.Quaternion();
	const FQuat CurrentControl = PlayerController.GetControlRotation().Quaternion();
	const FQuat CameraRotationDelta = (CurrentControl * BaseControl.Inverse()).GetNormalized();
	const FVector BaseCameraOffset =
		Data.StartTransform.GetLocation() - Data.EntranceBoomPivot;
	const FVector RotatedCameraOffset = CameraRotationDelta.RotateVector(BaseCameraOffset);
	// Move the entrance camera by the same amount as the destination boom pivot.
	// Convert through the exit doorway so the delta is expressed in entrance space;
	// this preserves movement through portals with different world rotations.
	const FVector CurrentBoomPivotInEntranceSpace =
		LinkedDoorway.ConvertLocationToLinkedPortal(
			LocalPlayerCharacter.CameraBoom->GetComponentLocation());
	const FVector BoomPivotTranslation =
		CurrentBoomPivotInEntranceSpace - Data.DestinationBoomPivot;
	const float ArmLengthDelta =
		LocalPlayerCharacter.CameraBoom->TargetArmLength - Data.BoomArmLength;
	const FVector ArmDirection = RotatedCameraOffset.GetSafeNormal();
	const FVector SocketOffsetDelta =
		LocalPlayerCharacter.CameraBoom->SocketOffset - Data.BoomSocketOffset;
	const FVector NewCameraLocation = Data.EntranceBoomPivot
		+ BoomPivotTranslation
		+ RotatedCameraOffset
		+ ArmDirection * ArmLengthDelta
		+ CurrentControl.RotateVector(SocketOffsetDelta);
	const FQuat NewCameraRotation =
		(CameraRotationDelta * Data.StartTransform.GetRotation()).GetNormalized();

	PlayerCamera.SetWorldLocationAndRotation(NewCameraLocation, NewCameraRotation);
	PlayerCamera.SetFieldOfView(LocalPlayerCharacter.FollowCamera->FieldOfView);

}

void FDoorwayCameraTransition::ScheduleFinish(ADoorwayActor& Doorway)
{
	if (Data.State == EDoorwayCameraTransitionState::FinishQueued
		|| Owner.Get() != &Doorway || !Doorway.GetWorld())
	{
		return;
	}

	Data.State = EDoorwayCameraTransitionState::FinishQueued;

	Doorway.GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(&Doorway, [&Doorway]
	{
		UDoorwayCameraTransitionComponent* CameraTransitionComponent =
			Doorway.GetCameraTransitionComponent();
		if (CameraTransitionComponent
			&& CameraTransitionComponent->IsTransitionOwnedBy(Doorway))
		{
			CameraTransitionComponent->FinishTransition(Doorway);
		}
	}));
}

void FDoorwayCameraTransition::BeginFromTeleport(
	ADoorwayActor& Doorway,
	const FRotator& DestinationControlRotation)
{
	if (Owner.IsValid())
	{
		return;
	}

	if (Data.CachedSnapshot.bIsValid)
	{
		Data.PendingSnapshot = Data.CachedSnapshot;
		Data.PendingDestinationControlRotation = DestinationControlRotation;
	}
	else
	{
		Data.PendingSnapshot = FDoorwayCameraSnapshot();
	}
	Begin(Doorway);
}

void FDoorwayCameraTransition::Begin(ADoorwayActor& Doorway)
{
	ADoorwayActor* LinkedDoorway = Doorway.GetLinkedDoorway();
	if (!CacheLocalReferences(Doorway) || !IsValid(LinkedDoorway) || !Doorway.GetPlayerCamera()
		|| !Doorway.GetWorld())
	{
		return;
	}

	APlayerController* PlayerController = CachedLocalPlayerController.Get();
	TransitionController = PlayerController;
	TransitionCharacter = CachedLocalPlayerCharacter;
	TransitionDestination = LinkedDoorway;
	PreviousViewTarget = PlayerController->GetViewTarget();
	if (const APlayerCharacter* Character = Cast<APlayerCharacter>(TransitionCharacter.Get()))
		bPreviousUseControlRotationForAim = Character->GetUseControlRotationForAim();
	Owner = &Doorway;
	if (UDoorwayCameraTransitionComponent* LinkedCameraTransitionComponent =
		LinkedDoorway->GetCameraTransitionComponent())
	{
		LinkedCameraTransitionComponent->SetTransitionOwner(Doorway);
		LinkedCameraTransitionComponent->SetTransitionWaitingForDestination();
	}
	Data.State = EDoorwayCameraTransitionState::WaitingForDestination;
	Data.RequestTime = Doorway.GetWorld()->GetTimeSeconds();
	Data.ReturnStartTime = 0.0;
	Data.ReturnLocationOffset = FVector::ZeroVector;
	Data.ReturnRotationOffset = FQuat::Identity;

	// Preserve the exact pre-teleport view at the entrance. The blend does not
	// begin until the owning client observes the character at the destination.
	APlayerCameraManager* CameraManager = CachedLocalCameraManager.Get();
	const bool bHasPendingSnapshot = Data.PendingSnapshot.bIsValid;
	Data.StartTransform = bHasPendingSnapshot
		? FTransform(Data.PendingSnapshot.View.Rotation,
			Data.PendingSnapshot.View.Location)
		: FTransform(CameraManager->GetCameraRotation(), CameraManager->GetCameraLocation());
	Data.EntranceBoomPivot = bHasPendingSnapshot
		? Data.PendingSnapshot.BoomPivot
		: Data.StartTransform.GetLocation();
	Data.DestinationControlRotation = bHasPendingSnapshot
		? Data.PendingDestinationControlRotation
		: PlayerController->GetControlRotation();
	Data.BoomArmLength = bHasPendingSnapshot
		? Data.PendingSnapshot.BoomArmLength
		: 0.0f;
	Data.BoomSocketOffset = bHasPendingSnapshot
		? Data.PendingSnapshot.BoomSocketOffset
		: FVector::ZeroVector;
	UCameraComponent* PlayerCamera = Doorway.GetPlayerCamera();
	PlayerCamera->SetWorldTransform(Data.StartTransform);
	PlayerCamera->SetFieldOfView(
		bHasPendingSnapshot ? Data.PendingSnapshot.View.FOV : CameraManager->GetFOVAngle());
	Data.PendingSnapshot = FDoorwayCameraSnapshot();

	if (PlayerCamera)
	{
		PlayerCamera->SetActive(true);
	}
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(CachedLocalPlayerCharacter.Get()))
	{
		PlayerCharacter->SetUseControlRotationForAim(true);
	}
	PlayerController->SetViewTargetWithBlend(&Doorway, 0.0f);
}

void FDoorwayCameraTransition::Finish(ADoorwayActor& Doorway)
{
	if (Owner.Get() != &Doorway)
	{
		return;
	}

	if (APlayerController* PlayerController = TransitionController.Get())
	{
		if (APlayerCharacter* Character = Cast<APlayerCharacter>(TransitionCharacter.Get()))
		{
			Character->SetUseControlRotationForAim(bPreviousUseControlRotationForAim);
		}
		// Do not overwrite a view target installed by another system during the transition.
		if (PlayerController->GetViewTarget() == &Doorway)
		{
			AActor* RestoreTarget = PreviousViewTarget.IsValid()
				? PreviousViewTarget.Get() : PlayerController->GetPawn();
			if (RestoreTarget) PlayerController->SetViewTargetWithBlend(RestoreTarget, 0.0f);
		}
	}
	Data.State = EDoorwayCameraTransitionState::Idle;
	Owner.Reset();
	// Keep PlayerCamera active after changing the view target. This actor ticks in
	// PostUpdateWork, so PlayerCameraManager can still hold this camera's cached POV
	// until its next update. Deactivating it here exposes one invalid/stale frame.

	if (ADoorwayActor* LinkedDoorway = TransitionDestination.Get())
	{
		if (UDoorwayCameraTransitionComponent* LinkedCameraTransitionComponent =
			LinkedDoorway->GetCameraTransitionComponent())
		{
			if (LinkedCameraTransitionComponent->IsTransitionOwnedBy(Doorway))
			{
				LinkedCameraTransitionComponent->SetTransitionIdle();
				LinkedCameraTransitionComponent->ClearTransitionOwner();
			}
		}
	}
	TransitionDestination.Reset();
	TransitionController.Reset();
	TransitionCharacter.Reset();
	PreviousViewTarget.Reset();
}

void FDoorwayCameraTransition::FinishOwnerIfAny()
{
	if (ADoorwayActor* TransitionOwner = Owner.Get())
	{
		if (UDoorwayCameraTransitionComponent* CameraTransitionComponent =
			TransitionOwner->GetCameraTransitionComponent())
		{
			CameraTransitionComponent->FinishTransition(*TransitionOwner);
		}
	}
}

bool FDoorwayCameraTransition::CacheLocalReferences(ADoorwayActor& Doorway)
{
	APlayerController* PlayerController = CachedLocalPlayerController.Get();
	ACharacter* PlayerCharacter = CachedLocalPlayerCharacter.Get();
	APlayerCameraManager* CameraManager = CachedLocalCameraManager.Get();
	if (PlayerController && PlayerController->IsLocalController()
		&& PlayerCharacter && PlayerCharacter == Cast<ACharacter>(PlayerController->GetPawn())
		&& CameraManager && CameraManager == PlayerController->PlayerCameraManager)
	{
		return true;
	}

	PlayerController = DLPlayerView::GetLocalController(Doorway.GetWorld());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		CachedLocalPlayerController.Reset();
		CachedLocalPlayerCharacter.Reset();
		CachedLocalCameraManager.Reset();
		return false;
	}

	PlayerCharacter = Cast<ACharacter>(PlayerController->GetPawn());
	CameraManager = PlayerController->PlayerCameraManager;
	if (!PlayerCharacter || !CameraManager)
	{
		return false;
	}

	CachedLocalPlayerController = PlayerController;
	CachedLocalPlayerCharacter = PlayerCharacter;
	CachedLocalCameraManager = CameraManager;
	return true;
}

void FDoorwayCameraTransition::CacheLocalPlayerCameraState(ADoorwayActor& Doorway)
{
	if (Owner.IsValid() || !CacheLocalReferences(Doorway))
	{
		return;
	}

	APlayerController* PlayerController = CachedLocalPlayerController.Get();
	APlayerCameraManager* CameraManager = CachedLocalCameraManager.Get();
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(CachedLocalPlayerCharacter.Get());
	if (!PlayerController || !CameraManager || !PlayerCharacter || !PlayerCharacter->CameraBoom
		|| PlayerController->GetViewTarget() != PlayerCharacter)
	{
		return;
	}

	// Retain the last final POV observed while the local player is at this doorway.
	// After replication moves the character to the exit this condition becomes false,
	// so a late transition RPC cannot overwrite the entrance-side camera state.
	UBoxComponent* PlayerNearbyBounds = Doorway.GetPlayerNearbyBounds();
	if (PlayerNearbyBounds && !PlayerNearbyBounds->IsOverlappingActor(PlayerCharacter))
	{
		return;
	}

	Data.CachedSnapshot.View = CameraManager->GetCameraCacheView();
	Data.CachedSnapshot.BoomPivot = PlayerCharacter->CameraBoom->GetComponentLocation();
	Data.CachedSnapshot.BoomArmLength = PlayerCharacter->CameraBoom->TargetArmLength;
	Data.CachedSnapshot.BoomSocketOffset = PlayerCharacter->CameraBoom->SocketOffset;
	Data.CachedSnapshot.bIsValid = true;
}

void FDoorwayCameraTransition::UpdateLinkedCamera(ADoorwayActor& Doorway)
{
	ADoorwayActor* LinkedDoorway = Doorway.GetLinkedDoorway();
	UCameraComponent* PlayerCamera = Doorway.GetPlayerCamera();
	if (!CacheLocalReferences(Doorway) || !IsValid(LinkedDoorway) || !PlayerCamera)
	{
		return;
	}

	APlayerCameraManager* CameraManager = CachedLocalCameraManager.Get();
	if (!CameraManager)
	{
		return;
	}

	const FMinimalViewInfo& PlayerView = CameraManager->GetCameraCacheView();
	PlayerCamera->SetWorldLocationAndRotation(
		LinkedDoorway->ConvertLocationToLinkedPortal(PlayerView.Location),
		LinkedDoorway->ConvertRotationToLinkedPortal(PlayerView.Rotation));
	PlayerCamera->SetFieldOfView(PlayerView.FOV);
}

bool FDoorwayCameraTransition::IsOwnedBy(const ADoorwayActor& Doorway) const
{
	return Owner.Get() == &Doorway;
}

bool FDoorwayCameraTransition::IsIdle() const
{
	return Data.State == EDoorwayCameraTransitionState::Idle;
}

APlayerCameraManager* FDoorwayCameraTransition::GetCachedLocalCameraManager() const
{
	return CachedLocalCameraManager.Get();
}

void FDoorwayCameraTransition::SetOwner(ADoorwayActor& Doorway)
{
	Owner = &Doorway;
}

void FDoorwayCameraTransition::SetIdle()
{
	Data.State = EDoorwayCameraTransitionState::Idle;
}

void FDoorwayCameraTransition::SetWaitingForDestination()
{
	Data.State = EDoorwayCameraTransitionState::WaitingForDestination;
}

void FDoorwayCameraTransition::ClearOwner()
{
	Owner.Reset();
}
