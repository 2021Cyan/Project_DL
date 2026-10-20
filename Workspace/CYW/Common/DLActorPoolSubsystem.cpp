#include "Common/DLActorPoolSubsystem.h"

#include "GameFramework/Actor.h"

void UDLActorPoolSubsystem::Release(AActor* Actor, TSubclassOf<AActor> ActorClass)
{
	if (!IsValid(Actor) || !Actor->HasAuthority() || Actor->GetWorld() != GetWorld())
	{
		return;
	}

	// Buckets always use the concrete class; reject accidental cross-class release.
	if (ActorClass && ActorClass.Get() != Actor->GetClass()) return;
	const TSubclassOf<AActor> PoolClass = Actor->GetClass();
	if (IsActorPooled(Actor))
	{
		return;
	}

	if (IPoolableActor* PoolableActor = Cast<IPoolableActor>(Actor))
	{
		PoolableActor->DeactivateForPool();
	}
	else
	{
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
		Actor->SetActorTickEnabled(false);
	}

	Actor->SetOwner(nullptr);
	Actor->SetInstigator(nullptr);
	TArray<TObjectPtr<AActor>>& Pool = InactiveActors.FindOrAdd(PoolClass).Actors;
	Pool.Add(Actor);
	PooledActors.Add(Actor);
}

bool UDLActorPoolSubsystem::IsActorPooled(const AActor* Actor) const
{
	return Actor && PooledActors.Contains(Actor);
}
