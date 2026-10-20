// Fill out your copyright notice in the Description page of Project Settings.

#include "Doorman/CallBell/CallBellProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Common/AbilitySupport.h"
#include "Common/DLActorPoolSubsystem.h"
#include "Common/SkillDamageLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Common/ProjectileMovementSupport.h"
#include "GameCore/Character/ADLPlayerState.h"
#include "GameCore/Struct/DLCollisionTypes.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/ProjectileBase.h"

ACallBellProjectile::ACallBellProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->OnComponentHit.AddDynamic(this, &ThisClass::OnBellHit);

	DetectCollision = CreateDefaultSubobject<USphereComponent>(TEXT("DetectCollision"));
	DetectCollision->SetupAttachment(RootComponent);
	DetectCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnDetectCollisionOverlap);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bSweepCollision = true;
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->MaxSimulationTimeStep = 1.0f / 120.0f;
	ProjectileMovement->MaxSimulationIterations = 8;

	TrailEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	TrailEffectComponent->SetupAttachment(RootComponent);
	TrailEffectComponent->bAutoActivate = true;

	RangeIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangeIndicator"));
	RangeIndicator->SetupAttachment(RootComponent);
	RangeIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeIndicator->SetGenerateOverlapEvents(false);
	RangeIndicator->SetCastShadow(false);
	RangeIndicator->SetHiddenInGame(true);
	RangeIndicator->SetUsingAbsoluteRotation(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		RangeIndicator->SetStaticMesh(SphereMeshFinder.Object);
	}

	ExplosionGameplayCueTag = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayCue.Skill.CallBell.Explosion")), false);

	ApplyComponentConfig();

	InitialLifeSpan = 0.f;
}

void ACallBellProjectile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyComponentConfig();
}

void ACallBellProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACallBellProjectile, bPoolActive);

	DOREPLIFETIME(ACallBellProjectile, ExplosionRadius);
}

void ACallBellProjectile::PrepareForPoolActivation()
{
	DeactivateForPool();
	bExploded = false;
	bHasSettled = false;
	LastMovementLocation = GetActorLocation();

	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(Collision);
	}

	ApplyComponentConfig();
	SetActorEnableCollision(false);
}

void ACallBellProjectile::ActivateFromPool()
{
	UpdateRangeIndicator();
	SetPoolActive(true);
}

void ACallBellProjectile::ApplyComponentConfig()
{
	if (Collision)
	{
		// Movement collision: stop only on map/world objects.
		Collision->SetSphereRadius(CollisionRadius);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Collision->SetCollisionObjectType(ECC_AimTarget);
		Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
		Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Collision->SetCollisionResponseToChannel(ECC_Projectile, ECR_Ignore);
		Collision->SetGenerateOverlapEvents(false);
		Collision->SetNotifyRigidBodyCollision(true);
	}

	if (DetectCollision)
	{
		// Detection collision: overlap attacks and character hit volumes without blocking movement.
		DetectCollision->SetSphereRadius(DetectCollisionRadius);
		DetectCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DetectCollision->SetCollisionObjectType(ECC_Projectile);
		DetectCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		DetectCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		DetectCollision->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
		DetectCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		DetectCollision->SetCollisionResponseToChannel(ECC_Team1, ECR_Overlap);
		DetectCollision->SetCollisionResponseToChannel(ECC_Team2, ECR_Overlap);
		DetectCollision->SetCollisionResponseToChannel(ECC_Monster, ECR_Overlap);
		DetectCollision->SetGenerateOverlapEvents(true);
		DetectCollision->SetNotifyRigidBodyCollision(false);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = InitialSpeed;
		ProjectileMovement->MaxSpeed = MaxSpeed;
		ProjectileMovement->ProjectileGravityScale = GravityScale;
		ProjectileMovement->bRotationFollowsVelocity = bRotationFollowsVelocity;
		ProjectileMovement->bShouldBounce = bShouldBounce;
		ProjectileMovement->bAutoActivate = false;
	}

	UpdateRangeIndicator();
}

void ACallBellProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (ProjectileMovement)
	{
		ProjectileMovement->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
	}

	if (!HasAuthority() && ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	ArmLifetimeTimer();
}

void ACallBellProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority() && !bExploded && !bHasSettled
		&& IPortalTraversable::TryTraversePortal(this, DeltaTime))
	{
		return;
	}

	if (HasAuthority() && !bExploded && !bHasSettled)
	{
		UpdateStraightMovement(DeltaTime);
	}

}

bool ACallBellProjectile::CanTraversePortal() const
{
	return HasAuthority() && !bExploded && !bHasSettled && ProjectileMovement;
}

FVector ACallBellProjectile::GetPortalTraversalVelocity() const
{
	return ProjectileMovement ? ProjectileMovement->Velocity : GetVelocity();
}

void ACallBellProjectile::ApplyPortalTraversal(
	const FVector& NewLocation,
	const FRotator& NewRotation,
	const FVector& NewVelocity)
{
	SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
	if (ProjectileMovement)
	{
		DLProjectileMovement::SetVelocity(ProjectileMovement, Collision, NewVelocity);
	}
	LastMovementLocation = NewLocation;
	ForceNetUpdate();
}

void ACallBellProjectile::InitializeCallBell(const FCallBellProjectileInitData& InitData)
{
	SourceActor = InitData.SourceActor;
	if (Collision && SourceActor)
	{
		Collision->IgnoreActorWhenMoving(SourceActor, true);
	}
	if (DetectCollision && SourceActor)
	{
		DetectCollision->IgnoreActorWhenMoving(SourceActor, true);
	}

	ImpactDamage = InitData.ImpactDamage;
	ImpactDamageCoefficient = InitData.ImpactDamageCoefficient;
	ExplosionDamage = InitData.ExplosionDamage;
	ExplosionDamageCoefficient = InitData.ExplosionDamageCoefficient;
	StatusEffectSpecs = InitData.StatusEffectSpecs;
	ExplosionRadius = InitData.ExplosionRadius;
	BellLifetime = InitData.BellLifetime;

	UpdateRangeIndicator();
	ArmLifetimeTimer();
}

void ACallBellProjectile::Launch(const FVector& Direction)
{
	if (ProjectileMovement)
	{
		LastMovementLocation = GetActorLocation();
		AccumulatedTravelDistance = 0.0f;
		bLaunched = true;
		LaunchSpeed = InitialSpeed;
		ProjectileMovement->SetUpdatedComponent(Collision);
		ProjectileMovement->Activate(true);
		ProjectileMovement->Velocity = Direction.GetSafeNormal() * LaunchSpeed;
		ProjectileMovement->UpdateComponentVelocity();
	}
}

void ACallBellProjectile::Detonate()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bExploded)
	{
		return;
	}

	bExploded = true;
	ApplyExplosion();
	ExecuteExplosionGameplayCue();
	ReleaseToPool();
}

void ACallBellProjectile::OnBellHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	(void)HitComponent;
	(void)OtherComp;
	(void)NormalImpulse;

	if (!HasAuthority())
	{
		return;
	}

	if (bExploded || !OtherActor || OtherActor == SourceActor)
	{
		return;
	}

	if (IsSourceProjectile(OtherActor))
	{
		Detonate();
		return;
	}

	SettleAtImpact(Hit);
}

void ACallBellProjectile::OnDetectCollisionOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;

	if (!HasAuthority() || !bPoolActive || bExploded)
	{
		return;
	}

	if (IsSourceProjectile(OtherActor))
	{
		Detonate();
		return;
	}

	ApplyImpactDamage(OtherActor, SweepResult);
}

void ACallBellProjectile::OnRep_ExplosionRadius()
{
	UpdateRangeIndicator();
}

void ACallBellProjectile::ArmLifetimeTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifetimeTimerHandle);
		if (BellLifetime <= 0.f) return;
		World->GetTimerManager().SetTimer(LifetimeTimerHandle, this, &ThisClass::Detonate, BellLifetime, false);
	}
}

void ACallBellProjectile::SettleAtImpact(const FHitResult& Hit)
{
	bHasSettled = true;
	SetActorLocation(Hit.ImpactPoint + Hit.ImpactNormal * 8.f);

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}
}

void ACallBellProjectile::ApplyImpactDamage(AActor* TargetActor, const FHitResult& Hit)
{
	(void)Hit;

	if (!CanAffectTarget(TargetActor))
	{
		return;
	}

	if (ImpactedActors.Contains(TargetActor))
	{
		return;
	}

	ImpactedActors.Add(TargetActor);
	if (ImpactDamage > 0.f || ImpactDamageCoefficient > 0.f)
	{
		USkillDamageLibrary::ApplyDamage(
			SourceActor, TargetActor, ImpactDamageCoefficient, ImpactDamage, TAG_Data_Damage_Spirit);
	}
}

void ACallBellProjectile::ApplyExplosion()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CallBellExplosion), false, this);
	if (SourceActor)
	{
		QueryParams.AddIgnoredActor(SourceActor);
	}

	World->OverlapMultiByProfile(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ExplosionOverlapProfileName,
		FCollisionShape::MakeSphere(ExplosionRadius),
		QueryParams);

	TSet<AActor*> AffectedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!CanAffectTarget(TargetActor) || AffectedActors.Contains(TargetActor))
		{
			continue;
		}

		AffectedActors.Add(TargetActor);
		if (ExplosionDamage > 0.f || ExplosionDamageCoefficient > 0.f)
		{
			USkillDamageLibrary::ApplyDamage(
				SourceActor, TargetActor, ExplosionDamageCoefficient, ExplosionDamage, TAG_Data_Damage_Spirit);
		}
		for (const FGameplayEffectSpecHandle& StatusEffectSpec : StatusEffectSpecs)
		{
			ApplyEffectSpecToTarget(TargetActor, StatusEffectSpec);
		}
	}

}

void ACallBellProjectile::ApplyEffectSpecToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& SpecHandle)
{
	if (!SpecHandle.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

bool ACallBellProjectile::CanAffectTarget(AActor* TargetActor) const
{
	if (!TargetActor || TargetActor == SourceActor)
	{
		return false;
	}

	if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		return false;
	}

	return !SourceActor || ADLPlayerState::IsHostileActor(SourceActor, TargetActor);
}

bool ACallBellProjectile::IsSourceProjectile(const AActor* Actor) const
{
	return Cast<AProjectileBase>(Actor) != nullptr;
}

void ACallBellProjectile::ReleaseToPool()
{
	if (UDLActorPoolSubsystem* Pool = GetWorld() ? GetWorld()->GetSubsystem<UDLActorPoolSubsystem>() : nullptr)
	{
		Pool->Release(this, GetClass());
		return;
	}

	Destroy();
}

void ACallBellProjectile::DeactivateForPool()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifetimeTimerHandle);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	if (Collision)
	{
		if (SourceActor)
		{
			Collision->IgnoreActorWhenMoving(SourceActor, false);
		}
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (DetectCollision)
	{
		if (SourceActor)
		{
			DetectCollision->IgnoreActorWhenMoving(SourceActor, false);
		}
		DetectCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DetectCollision->SetGenerateOverlapEvents(false);
	}

	if (TrailEffectComponent)
	{
		TrailEffectComponent->Deactivate();
	}

	SourceActor = nullptr;
	StatusEffectSpecs.Reset();
	ImpactedActors.Reset();
	bLaunched = false;
	AccumulatedTravelDistance = 0.0f;
	LaunchSpeed = 0.f;
	SetPoolActive(false);
}

void ACallBellProjectile::ExecuteExplosionGameplayCue()
{
	if (!ExplosionGameplayCueTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = SourceActor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor) : nullptr;
	if (!SourceASC)
	{
		return;
	}

	FGameplayCueParameters CueParams = DLAbilitySupport::MakeCueParameters(
		GetActorLocation(), GetActorForwardVector(), SourceActor, this, this);

	SourceASC->ExecuteGameplayCue(ExplosionGameplayCueTag, CueParams);
}

void ACallBellProjectile::UpdateRangeIndicator()
{
	if (!RangeIndicator)
	{
		return;
	}

	const bool bHasRangeVisual = bShowExplosionRange && ExplosionRadius > 0.f;
	RangeIndicator->SetHiddenInGame(!bHasRangeVisual);
	RangeIndicator->SetVisibility(bHasRangeVisual);
	if (!bHasRangeVisual)
	{
		return;
	}

	if (RangeIndicatorMaterial)
	{
		RangeIndicator->SetMaterial(0, RangeIndicatorMaterial);
	}
	RangeIndicator->SetRelativeLocation(FVector(0.f, 0.f, RangeIndicatorZOffset));
	RangeIndicator->SetWorldRotation(FRotator::ZeroRotator);

	float MeshRadius = 1.f;
	if (const UStaticMesh* RangeMesh = RangeIndicator->GetStaticMesh())
	{
		MeshRadius = FMath::Max(RangeMesh->GetBounds().BoxExtent.GetMax(), KINDA_SMALL_NUMBER);
	}

	const float Scale = ExplosionRadius / MeshRadius;
	RangeIndicator->SetWorldScale3D(FVector(Scale));
}

void ACallBellProjectile::UpdateStraightMovement(float DeltaTime)
{
	if (!ProjectileMovement)
	{
		return;
	}

	const FVector Velocity = ProjectileMovement->Velocity;
	const float CurrentSpeed = Velocity.Size();
	if (CurrentSpeed <= StopSpeed)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		bHasSettled = true;
		return;
	}

	if (!bLaunched || MaxTravelDistance <= 0.f || LaunchSpeed <= 0.f)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	AccumulatedTravelDistance += FVector::Distance(LastMovementLocation, CurrentLocation);
	LastMovementLocation = CurrentLocation;
	const float RemainingDistance = FMath::Max(0.f, MaxTravelDistance - AccumulatedTravelDistance);
	if (RemainingDistance <= FMath::Max(1.f, CurrentSpeed * DeltaTime))
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		bHasSettled = true;
		return;
	}

	const float RemainingRatio = RemainingDistance / MaxTravelDistance;
	const float TargetSpeed = LaunchSpeed * FMath::Pow(RemainingRatio, BrakingCurveExponent);
	const float NewSpeed = FMath::Min(CurrentSpeed, TargetSpeed);
	ProjectileMovement->Velocity = Velocity.GetSafeNormal() * NewSpeed;
}

void ACallBellProjectile::ApplyPoolActive(bool bActive)
{
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
	SetActorTickEnabled(bActive);

	if (Collision)
	{
		Collision->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}

	if (DetectCollision)
	{
		DetectCollision->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		DetectCollision->SetGenerateOverlapEvents(bActive);
	}

	if (Mesh)
	{
		Mesh->SetVisibility(bActive, true);
	}

	if (RangeIndicator)
	{
		RangeIndicator->SetVisibility(bActive && bShowExplosionRange && ExplosionRadius > 0.f);
	}

	if (TrailEffectComponent)
	{
		if (bActive)
		{
			TrailEffectComponent->Activate(true);
		}
		else
		{
			TrailEffectComponent->Deactivate();
		}
	}
}

void ACallBellProjectile::SetPoolActive(bool bActive)
{
	if (!HasAuthority()) return;
	bPoolActive = bActive;
	ApplyPoolActive(bActive);
	ForceNetUpdate();
}

void ACallBellProjectile::OnRep_PoolActive()
{
	ApplyPoolActive(bPoolActive);
}
