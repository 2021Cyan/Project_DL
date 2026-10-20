// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Common/PoolableActor.h"
#include "Doorman/Doorway/PortalTraversable.h"
#include "CallBellProjectile.generated.h"

class UNiagaraComponent;
class UMaterialInterface;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FCallBellProjectileInitData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY()
	float ImpactDamage = 0.f;

	UPROPERTY()
	float ImpactDamageCoefficient = 0.f;

	UPROPERTY()
	float ExplosionDamage = 0.f;

	UPROPERTY()
	float ExplosionDamageCoefficient = 0.f;

	UPROPERTY()
	TArray<FGameplayEffectSpecHandle> StatusEffectSpecs;

	UPROPERTY()
	float ExplosionRadius = 0.f;

	UPROPERTY()
	float BellLifetime = 0.f;
};

UCLASS()
class TEAMPROJECT_DL_API ACallBellProjectile : public AActor, public IPortalTraversable, public IPoolableActor
{
	GENERATED_BODY()

public:
	ACallBellProjectile();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PrepareForPoolActivation() override;
	virtual void ActivateFromPool() override;
	void InitializeCallBell(const FCallBellProjectileInitData& InitData);

	void Launch(const FVector& Direction);
	virtual bool CanTraversePortal() const override;
	virtual FVector GetPortalTraversalVelocity() const override;
	virtual void ApplyPortalTraversal(const FVector& NewLocation, const FRotator& NewRotation,
		const FVector& NewVelocity) override;

	UFUNCTION(BlueprintCallable, Category = "Call Bell")
	void Detonate();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Call Bell")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Call Bell")
	TObjectPtr<USphereComponent> DetectCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Call Bell")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Call Bell")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Call Bell|FX")
	TObjectPtr<UNiagaraComponent> TrailEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Call Bell|FX")
	TObjectPtr<UStaticMeshComponent> RangeIndicator;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Collision", meta = (ClampMin = "0.0"))
	float CollisionRadius = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Collision", meta = (ClampMin = "0.0"))
	float DetectCollisionRadius = 36.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Call Bell|FX")
	FGameplayTag ExplosionGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Call Bell|FX")
	TObjectPtr<UMaterialInterface> RangeIndicatorMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Call Bell|FX")
	bool bShowExplosionRange = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Call Bell|FX", meta = (ClampMin = "0.0"))
	float RangeIndicatorZOffset = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Call Bell|FX", meta = (ClampMin = "1.0"))
	// Retained for asset compatibility. The current spherical indicator has no thickness axis.
	float RangeIndicatorThickness = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Collision")
	FName ExplosionOverlapProfileName = TEXT("Pawn");

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Movement", meta = (ClampMin = "0.0"))
	float InitialSpeed = 1600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 1600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Movement")
	float GravityScale = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Movement")
	bool bRotationFollowsVelocity = true;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Movement")
	bool bShouldBounce = false;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Movement", meta = (ClampMin = "0.2", ClampMax = "0.85", ToolTip = "Higher values produce a longer, more pronounced slowdown."))
	float BrakingCurveExponent = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Movement", meta = (ClampMin = "0.0"))
	float StopSpeed = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Call Bell|Movement", meta = (ClampMin = "0.0"))
	float MaxTravelDistance = 550.f;

private:
	UFUNCTION()
	void OnBellHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void OnDetectCollisionOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_ExplosionRadius();

	void ArmLifetimeTimer();
	void SettleAtImpact(const FHitResult& Hit);
	void ApplyImpactDamage(AActor* TargetActor, const FHitResult& Hit);
	void ApplyExplosion();
	void ApplyEffectSpecToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& SpecHandle);
	bool CanAffectTarget(AActor* TargetActor) const;
	bool IsSourceProjectile(const AActor* Actor) const;
	void ReleaseToPool();
	void ExecuteExplosionGameplayCue();
	void UpdateStraightMovement(float DeltaTime);
	void ApplyComponentConfig();
	void UpdateRangeIndicator();

	UPROPERTY(ReplicatedUsing = OnRep_PoolActive)
	bool bPoolActive = true;

	UFUNCTION()
	void OnRep_PoolActive();
	void SetPoolActive(bool bActive);
	void ApplyPoolActive(bool bActive);

	virtual void DeactivateForPool() override;

	bool bExploded = false;
	bool bLaunched = false;
	FVector LastMovementLocation = FVector::ZeroVector;
	float AccumulatedTravelDistance = 0.0f;
	float LaunchSpeed = 0.f;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	float ImpactDamage = 0.f;
	float ImpactDamageCoefficient = 0.f;
	float ExplosionDamage = 0.f;
	float ExplosionDamageCoefficient = 0.f;
	TArray<FGameplayEffectSpecHandle> StatusEffectSpecs;
	UPROPERTY(ReplicatedUsing = OnRep_ExplosionRadius)
	float ExplosionRadius = 550.f;
	float BellLifetime = 3.f;

	bool bHasSettled = false;
	TSet<AActor*> ImpactedActors;
	FTimerHandle LifetimeTimerHandle;
};
