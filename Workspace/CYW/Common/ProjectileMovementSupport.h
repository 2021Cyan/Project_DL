#pragma once

#include "GameFramework/ProjectileMovementComponent.h"

namespace DLProjectileMovement
{
	inline void SetVelocity(UProjectileMovementComponent* Movement, USceneComponent* UpdatedComponent,
		const FVector& Velocity)
	{
		if (!Movement || !UpdatedComponent) return;
		Movement->SetUpdatedComponent(UpdatedComponent);
		Movement->Activate(true);
		Movement->Velocity = Velocity;
		Movement->UpdateComponentVelocity();
	}
}
