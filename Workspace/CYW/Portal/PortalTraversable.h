#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PortalTraversable.generated.h"

class ADoorwayActor;

UINTERFACE(MinimalAPI)
class UPortalTraversable : public UInterface
{
	GENERATED_BODY()
};

class TEAMPROJECT_DL_API IPortalTraversable
{
	GENERATED_BODY()

public:
	static bool TryTraversePortal(AActor* TraversableActor, float DeltaSeconds);
	virtual bool CanTraversePortal() const { return true; }
	virtual FVector GetPortalTraversalVelocity() const = 0;
	// Called once per traversal; implementations may consume per-shot correction state.
	virtual FVector ResolvePortalExitVelocity(const ADoorwayActor& Entrance,
		const FVector& ExitLocation, const FVector& TransformedVelocity)
	{
		return TransformedVelocity;
	}
	virtual void ApplyPortalTraversal(const FVector& NewLocation, const FRotator& NewRotation, const FVector& NewVelocity) = 0;
};
