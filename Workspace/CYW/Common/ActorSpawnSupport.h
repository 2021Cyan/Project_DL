#pragma once

#include "Common/DLActorPoolSubsystem.h"
#include "Engine/World.h"

namespace DLActorSpawn
{
	// Initialization must finish before collision, visual activation, or launch.
	template<typename T, typename Initializer>
	T* AcquireOrSpawn(UWorld* World, TSubclassOf<T> Class, const FTransform& Transform,
		AActor* Owner, APawn* Instigator, ESpawnActorCollisionHandlingMethod CollisionPolicy,
		Initializer&& Initialize)
	{
		if (!World || World->GetNetMode() == NM_Client || !Class) return nullptr;
		if (UDLActorPoolSubsystem* Pool = World->GetSubsystem<UDLActorPoolSubsystem>())
			return Pool->AcquireInitialized<T>(Class, Transform, Owner, Instigator,
				CollisionPolicy, Forward<Initializer>(Initialize));
		T* Actor = World->SpawnActorDeferred<T>(Class, Transform, Owner, Instigator, CollisionPolicy);
		if (!Actor) return nullptr;
		Actor->SetActorEnableCollision(false);
		Actor->FinishSpawning(Transform);
		return UDLActorPoolSubsystem::InitializeAndActivate(
			Actor, Owner, Instigator, Forward<Initializer>(Initialize));
	}
}
