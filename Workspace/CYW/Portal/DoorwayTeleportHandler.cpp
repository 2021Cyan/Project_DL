// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayTeleportHandler.h"

#include "Doorman/Doorway/DoorwayActor.h"
#include "Doorman/Doorway/DoorwayPortalMath.h"
#include "Doorman/Doorway/PortalTraversable.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameCore/Character/BasePlayerController.h"
#include "GameCore/Character/PlayerCharacter.h"

namespace
{
	bool IsCrossingPortal(
		const FVector& PreviousPosition,
		const FVector& CurrentPosition,
		const FVector& PortalLocation,
		const FVector& PortalNormal,
		bool bWasInFront,
		bool& bOutIsInFront)
	{
		bOutIsInFront = FVector::DotProduct(PortalNormal, CurrentPosition - PortalLocation) >= 0.0f;

		FVector Intersection = FVector::ZeroVector;
		const bool bIsIntersecting = FMath::SegmentPlaneIntersection(
			PreviousPosition,
			CurrentPosition,
			FPlane(PortalLocation, PortalNormal),
			Intersection);
		return bIsIntersecting && !bOutIsInFront && bWasInFront;
	}

	float GetExitClearance(float ExitSafeDistance, AActor& TraversableActor, const FVector& ExitNormal)
	{
		float CollisionClearance = 0.0f;
		if (const UPrimitiveComponent* RootPrimitive =
			Cast<UPrimitiveComponent>(TraversableActor.GetRootComponent()))
		{
			const FVector WorldExtent = RootPrimitive->Bounds.BoxExtent;
			CollisionClearance =
				FMath::Abs(ExitNormal.X) * WorldExtent.X
				+ FMath::Abs(ExitNormal.Y) * WorldExtent.Y
				+ FMath::Abs(ExitNormal.Z) * WorldExtent.Z;
		}

		return FMath::Max(ExitSafeDistance, CollisionClearance + 5.0f);
	}

}

void FDoorwayTeleportHandler::OnPlayerNearbyBeginOverlap(ADoorwayActor& Doorway, AActor* OtherActor)
{
	if (Doorway.HasAuthority())
	{
		if (ACharacter* Character = Cast<ACharacter>(OtherActor))
		{
			NearbyActors.Add(Character);
		}
	}
}

void FDoorwayTeleportHandler::TickServer(ADoorwayActor& Doorway)
{
	if (!Doorway.HasAuthority())
	{
		return;
	}

	CheckCharacterCrossings(Doorway);
}

void FDoorwayTeleportHandler::AddInitialOverlap(AActor* Actor)
{
	if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		NearbyActors.Add(Character);
	}
}

void FDoorwayTeleportHandler::OnPlayerNearbyEndOverlap(AActor* OtherActor)
{
	if (OtherActor)
	{
		NearbyActors.Remove(OtherActor);
		CrossingStates.Remove(OtherActor);
	}
}

void FDoorwayTeleportHandler::CheckCharacterCrossings(ADoorwayActor& Doorway)
{
	UBoxComponent* TeleportDetection = Doorway.GetTeleportDetection();
	if (!Doorway.HasAuthority() || !IsValid(Doorway.GetLinkedDoorway())
		|| !Doorway.GetPlayerNearbyBounds() || !TeleportDetection)
	{
		return;
	}

	// Snapshot the event-maintained set because teleporting can synchronously fire
	// EndOverlap and mutate NearbyActors during this loop.
	TArray<TWeakObjectPtr<AActor>> ActorsToCheck;
	ActorsToCheck.Reserve(NearbyActors.Num());
	for (const TWeakObjectPtr<AActor>& Actor : NearbyActors)
	{
		ActorsToCheck.Add(Actor);
	}

	for (const TWeakObjectPtr<AActor>& ActorPtr : ActorsToCheck)
	{
		ACharacter* PlayerCharacter = Cast<ACharacter>(ActorPtr.Get());
		if (!IsValid(PlayerCharacter))
		{
			NearbyActors.Remove(ActorPtr);
			CrossingStates.Remove(ActorPtr);
			continue;
		}
		if (!TeleportDetection->IsOverlappingActor(PlayerCharacter))
		{
			CrossingStates.Remove(ActorPtr);
			continue;
		}

		const DoorwayPortalMath::FDoorwayPortalFrame TeleportFrame =
			Doorway.GetTeleportCrossingFrame();
		const FVector CurrentPosition = PlayerCharacter->GetActorLocation();
		const bool bIsInFront = FVector::DotProduct(
			TeleportFrame.PlaneNormal, CurrentPosition - TeleportFrame.PlaneLocation) >= 0.0f;
		FDoorwayCrossingState& State = CrossingStates.FindOrAdd(ActorPtr);

		if (!State.bInitialized)
		{
			State.LastPosition = CurrentPosition;
			State.bLastInFront = bIsInFront;
			State.bInitialized = true;
			continue;
		}

		bool bCurrentInFront = false;
		const bool bIsCrossing = IsCrossingPortal(
			State.LastPosition,
			CurrentPosition,
			TeleportFrame.PlaneLocation,
			TeleportFrame.PlaneNormal,
			State.bLastInFront,
			bCurrentInFront);
		State.LastPosition = CurrentPosition;
		State.bLastInFront = bCurrentInFront;

		if (bIsCrossing)
		{
			CrossingStates.Remove(ActorPtr);
			TeleportCharacterInternal(Doorway, PlayerCharacter);
		}
	}
}

void FDoorwayTeleportHandler::TeleportCharacterInternal(ADoorwayActor& Doorway, ACharacter* PlayerCharacter)
{
	if (!Doorway.HasAuthority() || !IsValid(Doorway.GetLinkedDoorway()) || !IsValid(PlayerCharacter))
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController());
	UMovementComponent* MovementComponent = PlayerCharacter->GetMovementComponent();
	const FVector OldVelocity = MovementComponent ? MovementComponent->Velocity : FVector::ZeroVector;
	const FVector NewLocation = Doorway.ConvertTeleportLocationToLinkedPortal(PlayerCharacter->GetActorLocation());
	const FRotator NewActorRotation = Doorway.ConvertTeleportRotationToLinkedPortal(PlayerCharacter->GetActorRotation());
	const FRotator NewControlRotation = PlayerController
		? Doorway.ConvertTeleportRotationToLinkedPortal(PlayerController->GetControlRotation())
		: NewActorRotation;
	const FVector NewVelocity = Doorway.ConvertVelocityToLinkedPortal(OldVelocity);
	const FVector HorizontalVelocity = FVector(OldVelocity.X, OldVelocity.Y, 0.0f);
	const FVector HorizontalViewForward = PlayerController
		? FVector(PlayerController->GetControlRotation().Vector().X,
			PlayerController->GetControlRotation().Vector().Y, 0.0f)
		: FVector::ZeroVector;
	const bool bHasMovementDirection = !HorizontalVelocity.IsNearlyZero();
	const float ViewMovementAlignment = bHasMovementDirection
		? FMath::Abs(FVector::DotProduct(
			HorizontalVelocity.GetSafeNormal(), HorizontalViewForward.GetSafeNormal()))
		: 1.0f;
	const bool bUseDoorwayCameraTransition =
		ViewMovementAlignment >= Doorway.GetSideEntryCameraTransitionDotThreshold();

	if (bUseDoorwayCameraTransition)
	{
		if (ABasePlayerController* BasePlayerController = Cast<ABasePlayerController>(PlayerController))
		{
			// The owning client already caches its final PlayerCameraManager POV.
			// The server only signals which doorway initiated the transition.
			BasePlayerController->ClientBeginDoorwayCameraTransition(&Doorway, NewControlRotation);
		}
	}

	if (APlayerCharacter* DLPlayerCharacter = Cast<APlayerCharacter>(PlayerCharacter))
	{
		// Use the character's existing multicast teleport path so actor and control
		// rotations change on every machine in the same network update.
		DLPlayerCharacter->TeleportInstantly(
			NewLocation,
			NewActorRotation,
			NewControlRotation,
			NewVelocity,
			true);
	}
	else
	{
		PlayerCharacter->SetActorLocationAndRotation(
			NewLocation, NewActorRotation, false, nullptr, ETeleportType::TeleportPhysics);
		PlayerCharacter->ForceNetUpdate();
		if (PlayerController)
		{
			PlayerController->SetControlRotation(NewControlRotation);
			PlayerController->ClientSetRotation(NewControlRotation, false);
		}
	}

	if (MovementComponent)
	{
		MovementComponent->Velocity = NewVelocity;
	}
}

bool FDoorwayTeleportHandler::TeleportTraversableFromTrace(
	ADoorwayActor& Doorway,
	AActor* TraversableActor,
	const FVector& SourceLocation)
{
	ADoorwayActor* LinkedDoorway = Doorway.GetLinkedDoorway();
	if (!Doorway.HasAuthority() || !IsValid(LinkedDoorway) || !IsValid(TraversableActor))
	{
		return false;
	}

	IPortalTraversable* Traversable = Cast<IPortalTraversable>(TraversableActor);
	if (!Traversable || !Traversable->CanTraversePortal())
	{
		return false;
	}

	const FVector ExitNormal = LinkedDoorway->GetPortalForwardVector();
	const float ExitClearance = GetExitClearance(
		LinkedDoorway->GetExitSafeDistance(),
		*TraversableActor,
		ExitNormal);
	const FVector NewLocation = DoorwayPortalMath::TransformLocation(
		Doorway.GetTraversableTraceFrame(),
		LinkedDoorway->GetTraversableTraceFrame(),
		SourceLocation)
		+ ExitNormal * ExitClearance;
	const FRotator NewRotation = Doorway.ConvertTeleportRotationToLinkedPortal(TraversableActor->GetActorRotation());
	const FVector TransformedVelocity = Doorway.ConvertTeleportDirectionToLinkedPortal(
		Traversable->GetPortalTraversalVelocity());
	const FVector NewVelocity = Traversable->ResolvePortalExitVelocity(Doorway, NewLocation, TransformedVelocity);
	Traversable->ApplyPortalTraversal(NewLocation, NewRotation, NewVelocity);
	return true;
}
