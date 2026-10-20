#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableActor.generated.h"

UINTERFACE(MinimalAPI)
class UPoolableActor : public UInterface
{
	GENERATED_BODY()
};

class TEAMPROJECT_DL_API IPoolableActor
{
	GENERATED_BODY()

public:
	// Reset reuse state while the actor is still inactive; callers initialize next.
	virtual void PrepareForPoolActivation() { DeactivateForPool(); }
	virtual void ActivateFromPool() = 0;
	virtual void DeactivateForPool() = 0;
};
