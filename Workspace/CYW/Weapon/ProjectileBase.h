// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Common/PoolableActor.h"
#include "Doorman/Doorway/PortalTraversable.h"
#include "Weapon/ProjectileAimLibrary.h"
#include "ProjectileBase.generated.h"

class USphereComponent;
class UAbilitySystemComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
enum class ECharacterHitZone : uint8;

UCLASS()
class TEAMPROJECT_DL_API AProjectileBase : public AActor, public IPortalTraversable, public IPoolableActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AProjectileBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PrepareForPoolActivation() override;
	virtual void ActivateFromPool() override;
	virtual void DeactivateForPool() override;
	void SetDamageParameters(float NewCoefficient, float NewBaseDamage)
	{
		DamageCoefficient = NewCoefficient;
		BaseDamage = NewBaseDamage;
	}
	void SetProjectileEffect(UNiagaraSystem* NewProjectileEffect);
	void ConfigureCollisionForSource(AActor* SourceActor);
	void Launch(const FVector& Direction);
	void SetPortalAim(const DLProjectileAim::FPortalAim& Aim) { PortalAim = Aim; }
	virtual FVector ResolvePortalExitVelocity(const ADoorwayActor& Entrance,
		const FVector& ExitLocation, const FVector& TransformedVelocity) override;
	/** 아이템이 생성한 반격 탄환에서 추가 아이템 이벤트와 연쇄 반사를 막는다. */
	void SetCanTriggerItemEvents(bool bNewCanTrigger) { bCanTriggerItemEvents = bNewCanTrigger; }
	virtual FVector GetPortalTraversalVelocity() const override;
	virtual void ApplyPortalTraversal(const FVector& NewLocation, const FRotator& NewRotation,
		const FVector& NewVelocity) override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UNiagaraComponent> ProjectileEffectComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> DefaultProjectileEffect;

	UPROPERTY(ReplicatedUsing = OnRep_ProjectileEffect)
	TObjectPtr<UNiagaraSystem> CurrentProjectileEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float DamageCoefficient = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float BaseDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float HeadshotDamageMultiplier = 1.5f;

	/** false면 BulletHit과 DamageReceived.Bullet 이벤트를 모두 전송하지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Items")
	bool bCanTriggerItemEvents = true;

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bShowDebug = false;

	UPROPERTY(EditDefaultsOnly, Category = "Lifetime", meta = (ClampMin = "0.0"))
	float ProjectileLifetime = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GameplayCue")
	FGameplayTag ImpactGameplayCueTag;

	UFUNCTION()
	void OnRep_ProjectileEffect();

	void NotifyBulletHit(AActor* Shooter, AActor* Target, float RawDamage) const;
	void NotifyDamageReceived(AActor* Shooter, AActor* Target, UAbilitySystemComponent* TargetASC, float HealthBeforeDamage) const;
	void ProcessHostileImpact(AActor* Shooter, AActor* OtherActor, const FHitResult& Hit);
	bool bProcessingImpact = false;
	void ApplyComponentConfig();
	void ApplyProjectileEffect();
	void ExecuteImpactGameplayCue(const FHitResult& Hit, AActor* Shooter) const;
	void ArmLifetimeTimer();
	void ReleaseToPool();

	UPROPERTY(ReplicatedUsing = OnRep_PoolActive)
	bool bPoolActive = true;

	UFUNCTION()
	void OnRep_PoolActive();
	void SetPoolActive(bool bActive);
	void ApplyPoolActive(bool bActive);

	FVector LastDebugLocation = FVector::ZeroVector;
	bool bReleasedToPool = false;
	FTimerHandle LifetimeTimerHandle;
	DLProjectileAim::FPortalAim PortalAim;

	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);
};
