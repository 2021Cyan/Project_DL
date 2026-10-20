#include "Doorman/Doorway/PortalTraversable.h"
#include "Doorman/Doorway/DoorwayActor.h"
#include "Engine/World.h"

bool IPortalTraversable::TryTraversePortal(AActor* TraversableActor, float DeltaSeconds)
{
	if (!IsValid(TraversableActor) || !TraversableActor->GetWorld() || !TraversableActor->HasAuthority()
		|| !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
	{
		return false;
	}

	IPortalTraversable* Traversable = Cast<IPortalTraversable>(TraversableActor);
	if (!Traversable || !Traversable->CanTraversePortal())
	{
		return false;
	}

	const FVector Velocity = Traversable->GetPortalTraversalVelocity();
	if (Velocity.IsNearlyZero())
	{
		return false;
	}

	// Look far enough ahead that the collision shape cannot hit the wall before
	// its center ray reaches the portal plane.
	const float TracePadding = FMath::Max(20.0f, TraversableActor->GetSimpleCollisionRadius() + 20.0f);
	const FVector TraceStart = TraversableActor->GetActorLocation();
	const FVector TraceEnd = TraceStart + Velocity * DeltaSeconds + Velocity.GetSafeNormal() * TracePadding;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PortalTraversalTrace), false, TraversableActor);
	FHitResult Hit;
	if (!TraversableActor->GetWorld()->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd, DoorwayPortalMath::TraversalTraceChannel, QueryParams))
	{
		return false;
	}

	ADoorwayActor* Doorway = Cast<ADoorwayActor>(Hit.GetActor());
	if (!Doorway)
	{
		return false;
	}

	// Use a point immediately through the actual plane. Doorway adds its configured
	// exit clearance after converting this point to the linked plane.
	const FVector Direction = Velocity.GetSafeNormal();
	const FVector SourceLocation = Hit.ImpactPoint + Direction;
	return Doorway->TeleportTraversableFromTrace(TraversableActor, SourceLocation);
}
