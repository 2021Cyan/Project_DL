#pragma once

#include "CoreMinimal.h"
#include "Common/PoolableActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "DLActorPoolSubsystem.generated.h"

USTRUCT()
struct FDLActorPoolList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Actors;
};

UCLASS()
class TEAMPROJECT_DL_API UDLActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	template<typename T>
	T* Acquire(TSubclassOf<T> Class, const FTransform& Transform, AActor* Owner,
		APawn* Instigator, ESpawnActorCollisionHandlingMethod Policy = ESpawnActorCollisionHandlingMethod::AlwaysSpawn)
	{
		return AcquireInitialized<T>(Class, Transform, Owner, Instigator, Policy, [](T*) {});
	}

	template<typename T, typename Initializer>
	T* AcquireInitialized(TSubclassOf<T> Class, const FTransform& Transform, AActor* Owner,
		APawn* Instigator, ESpawnActorCollisionHandlingMethod Policy, Initializer&& Initialize)
	{
		UWorld* World = GetWorld();
		if (!World || World->GetNetMode() == NM_Client || !Class) return nullptr;
		T* Actor = nullptr;
		TArray<TObjectPtr<AActor>>& Pool = InactiveActors.FindOrAdd(Class).Actors;
		while (!Pool.IsEmpty() && !Actor)
		{
			AActor* Candidate = Pool.Pop(EAllowShrinking::No);
			PooledActors.Remove(Candidate);
			if (IsValid(Candidate) && Candidate->GetClass() == Class.Get()) Actor = Cast<T>(Candidate);
		}
		if (!Actor) Actor = SpawnInactive<T>(Class, Transform, Owner, Instigator, Policy);
		if (!Actor) return nullptr;
		Actor->SetActorEnableCollision(false);
		Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
		return InitializeAndActivate(Actor, Owner, Instigator, Forward<Initializer>(Initialize));
	}

	// Shared lifecycle for pooled actors and the non-pool spawning fallback.
	template<typename T, typename Initializer>
	static T* InitializeAndActivate(T* Actor, AActor* Owner, APawn* Instigator, Initializer&& Initialize)
	{
		if (!IsValid(Actor))
		{
			return nullptr;
		}
		if (IPoolableActor* Poolable = Cast<IPoolableActor>(Actor))
		{
			Poolable->PrepareForPoolActivation();
		}
		Actor->SetOwner(Owner);
		Actor->SetInstigator(Instigator);
		Initialize(Actor);
		if (!IsValid(Actor))
		{
			return nullptr;
		}
		if (IPoolableActor* Poolable = Cast<IPoolableActor>(Actor))
		{
			Poolable->ActivateFromPool();
		}
		else
		{
			Actor->SetActorHiddenInGame(false);
			Actor->SetActorEnableCollision(true);
			Actor->SetActorTickEnabled(true);
		}
		return Actor;
	}

	template<typename T>
	void Prewarm(TSubclassOf<T> Class, int32 Count, AActor* Owner, APawn* Instigator,
		ESpawnActorCollisionHandlingMethod Policy = ESpawnActorCollisionHandlingMethod::AlwaysSpawn)
	{
		UWorld* World = GetWorld();
		if (!World || World->GetNetMode() == NM_Client || !Class || Count <= 0) return;
		TArray<TObjectPtr<AActor>>& Pool = InactiveActors.FindOrAdd(Class).Actors;
		Pool.RemoveAll([this](AActor* Actor)
		{
			if (IsValid(Actor)) return false;
			PooledActors.Remove(Actor);
			return true;
		});
		const int32 SpawnCount = FMath::Max(0, Count - Pool.Num());
		const FTransform Transform(FVector(0.f, 0.f, -100000.f));
		for (int32 Index = 0; Index < SpawnCount; ++Index)
			if (T* Actor = SpawnInactive<T>(Class, Transform, Owner, Instigator, Policy)) Release(Actor);
	}

	void Release(AActor* Actor, TSubclassOf<AActor> ActorClass = nullptr);

private:
	template<typename T>
	T* SpawnInactive(TSubclassOf<T> Class, const FTransform& Transform, AActor* Owner,
		APawn* Instigator, ESpawnActorCollisionHandlingMethod Policy)
	{
		T* Actor = GetWorld()->SpawnActorDeferred<T>(Class, Transform, Owner, Instigator, Policy);
		if (Actor)
		{
			Actor->SetActorEnableCollision(false);
			Actor->FinishSpawning(Transform);
		}
		return Actor;
	}

	bool IsActorPooled(const AActor* Actor) const;

	UPROPERTY()
	TMap<TSubclassOf<AActor>, FDLActorPoolList> InactiveActors;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> PooledActors;
};
