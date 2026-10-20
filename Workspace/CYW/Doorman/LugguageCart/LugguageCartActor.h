#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Common/PoolableActor.h"
#include "Common/CharacterMovementStateGuard.h"
#include "Doorman/Doorway/PortalTraversable.h"
#include "LugguageCartActor.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

USTRUCT()
struct FLugguageCartInitData
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<AActor> SourceActor;
	UPROPERTY() float DamageCoefficient = 0.75f;
	UPROPERTY() float BaseDamage = 80.f;
	UPROPERTY() float WallImpactDamage = 0.f;
	UPROPERTY() float WallImpactDamageCoefficient = 0.f;
	UPROPERTY() float MaxTravelDistance = 4500.f;
	UPROPERTY() FGameplayEffectSpecHandle StunEffectSpec;
};

UCLASS()
class TEAMPROJECT_DL_API ALugguageCartActor : public AActor, public IPortalTraversable, public IPoolableActor
{
	GENERATED_BODY()

public:
	ALugguageCartActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostNetReceiveLocationAndRotation() override;
	virtual void PrepareForPoolActivation() override;
	virtual void ActivateFromPool() override;
	void InitializeCart(const FLugguageCartInitData& InitData);
	void Launch(const FVector& Direction);
	virtual bool CanTraversePortal() const override;
	virtual FVector GetPortalTraversalVelocity() const override;
	virtual void ApplyPortalTraversal(const FVector& NewLocation, const FRotator& NewRotation,
		const FVector& NewVelocity) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lugguage Cart")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lugguage Cart")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lugguage Cart")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = "Lugguage Cart|FX")
	FGameplayTag DestroyGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Lugguage Cart|Movement", meta = (ClampMin = "0.0"))
	float MoveSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lugguage Cart|Capture")
	FVector CapturedTargetAttachOffset = FVector(0.f, 0.f, 40.f);

private:
	UFUNCTION()
	void OnCartHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnCartOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void CaptureTarget(class ACharacter* TargetCharacter);
	void ReleaseTarget();
	void ApplyStun(ACharacter* TargetCharacter) const;
	void HandleLanding();
	void HandleWallImpact();
	void DestroyCart();
	void ReleaseToPool();
	void ExecuteDestroyGameplayCue();
	void ResetTravelDistance();
	void ApplyComponentConfig();

	UPROPERTY(ReplicatedUsing = OnRep_PoolActive)
	bool bPoolActive = true;

	UFUNCTION()
	void OnRep_PoolActive();
	void SetPoolActive(bool bActive);
	void ApplyPoolActive(bool bActive);

	virtual void DeactivateForPool() override;

	UPROPERTY() TObjectPtr<AActor> SourceActor;
	float DamageCoefficient = 0.75f;
	float BaseDamage = 80.f;
	float WallImpactDamage = 0.f;
	float WallImpactDamageCoefficient = 0.f;
	float MaxTravelDistance = 4500.f;
	FGameplayEffectSpecHandle StunEffectSpec;
	FVector LastTravelLocation = FVector::ZeroVector;
	float AccumulatedTravelDistance = 0.0f;
	FVector TravelDirection = FVector::ForwardVector;
	UPROPERTY() TObjectPtr<ACharacter> CapturedTarget;
	ECollisionEnabled::Type CapturedTargetCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	FCharacterMovementStateGuard CapturedMovementState;
	TWeakObjectPtr<USceneComponent> PreviousAttachParent;
	FName PreviousAttachSocket;
	bool bHasLanded = false;
	bool bDestroyRequested = false;
};
