#include "Doorman/LugguageCart/LugguageCartActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Common/AbilitySupport.h"
#include "Common/DLActorPoolSubsystem.h"
#include "Common/SkillDamageLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Common/ProjectileMovementSupport.h"
#include "GameCore/Character/ADLPlayerState.h"
#include "GameplayEffectTypes.h"
#include "Net/UnrealNetwork.h"

ALugguageCartActor::ALugguageCartActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->OnComponentHit.AddDynamic(this, &ThisClass::OnCartHit);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCartOverlap);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

	DestroyGameplayCueTag = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayCue.Skill.LugguageCart.Destroy")), false);
	ApplyComponentConfig();
}

void ALugguageCartActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyComponentConfig();
}

void ALugguageCartActor::ApplyComponentConfig()
{
	if (Collision)
	{
		Collision->SetCollisionProfileName(TEXT("Projectile"));
		Collision->SetCollisionObjectType(ECC_WorldDynamic);
		Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Collision->SetGenerateOverlapEvents(true);
	}

	if (Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->UpdatedComponent = Collision;
		ProjectileMovement->InitialSpeed = MoveSpeed;
		ProjectileMovement->MaxSpeed = MoveSpeed;
		ProjectileMovement->bSweepCollision = true;
		ProjectileMovement->bForceSubStepping = true;
		ProjectileMovement->MaxSimulationTimeStep = 1.0f / 120.0f;
		ProjectileMovement->MaxSimulationIterations = 8;
		ProjectileMovement->ProjectileGravityScale = 1.f;
		ProjectileMovement->bRotationFollowsVelocity = false;
		ProjectileMovement->bShouldBounce = true;
		ProjectileMovement->Bounciness = 0.f;
		ProjectileMovement->Friction = 0.f;
		ProjectileMovement->bBounceAngleAffectsFriction = false;
		ProjectileMovement->BounceVelocityStopSimulatingThreshold = 5.f;
		ProjectileMovement->bInterpMovement = true;
		ProjectileMovement->bInterpRotation = true;
		ProjectileMovement->InterpLocationTime = 0.08f;
		ProjectileMovement->InterpRotationTime = 0.05f;
		ProjectileMovement->InterpLocationMaxLagDistance = 300.f;
		ProjectileMovement->InterpLocationSnapToTargetDistance = 600.f;
		ProjectileMovement->bAutoActivate = false;
	}
}

void ALugguageCartActor::BeginPlay()
{
	Super::BeginPlay();
	if (ProjectileMovement)
	{
		ProjectileMovement->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
	}

	if (!HasAuthority() && ProjectileMovement)
	{
		ProjectileMovement->SetInterpolatedComponent(Mesh);
		ProjectileMovement->Velocity = FVector::ZeroVector;
		ProjectileMovement->bSimulationEnabled = false;
		ProjectileMovement->Activate();
	}
}

void ALugguageCartActor::PostNetReceiveLocationAndRotation()
{
	if (!HasAuthority() && ProjectileMovement && ProjectileMovement->GetInterpolatedComponent())
	{
		const FRepMovement& RepMovement = GetReplicatedMovement();
		const FVector NewLocation = FRepMovement::RebaseOntoLocalOrigin(RepMovement.Location, this);
		ProjectileMovement->MoveInterpolationTarget(NewLocation, RepMovement.Rotation);
		return;
	}

	Super::PostNetReceiveLocationAndRotation();
}

void ALugguageCartActor::PrepareForPoolActivation()
{
	DeactivateForPool();
	bDestroyRequested = false;
	bHasLanded = false;
	CapturedTargetCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	CapturedMovementState.Reset();
	PreviousAttachParent.Reset();
	ResetTravelDistance();
	TravelDirection = GetActorForwardVector().GetSafeNormal();
	if (TravelDirection.IsNearlyZero())
	{
		TravelDirection = FVector::ForwardVector;
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(Collision);
	}

	ApplyComponentConfig();
	SetActorEnableCollision(false);
}

void ALugguageCartActor::ActivateFromPool()
{
	SetPoolActive(true);
}

void ALugguageCartActor::InitializeCart(const FLugguageCartInitData& InitData)
{
	SourceActor = InitData.SourceActor;
	DamageCoefficient = InitData.DamageCoefficient;
	BaseDamage = InitData.BaseDamage;
	WallImpactDamage = InitData.WallImpactDamage;
	WallImpactDamageCoefficient = InitData.WallImpactDamageCoefficient;
	MaxTravelDistance = InitData.MaxTravelDistance;
	StunEffectSpec = InitData.StunEffectSpec;
	ResetTravelDistance();

	if (Collision && SourceActor)
	{
		Collision->IgnoreActorWhenMoving(SourceActor, true);
	}
}

void ALugguageCartActor::Launch(const FVector& Direction)
{
	TravelDirection = Direction.GetSafeNormal();
	if (TravelDirection.IsNearlyZero())
	{
		TravelDirection = GetActorForwardVector().GetSafeNormal();
	}

	ResetTravelDistance();

	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(Collision);
		ProjectileMovement->Activate(true);
		ProjectileMovement->InitialSpeed = MoveSpeed;
		ProjectileMovement->MaxSpeed = MoveSpeed;
		ProjectileMovement->Velocity = TravelDirection * MoveSpeed;
		ProjectileMovement->UpdateComponentVelocity();
	}
}

void ALugguageCartActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && IPortalTraversable::TryTraversePortal(this, DeltaSeconds))
	{
		return;
	}

	if (HasAuthority())
	{
		const FVector CurrentLocation = GetActorLocation();
		AccumulatedTravelDistance += FVector::Distance(LastTravelLocation, CurrentLocation);
		LastTravelLocation = CurrentLocation;
		if (MaxTravelDistance > 0.f && AccumulatedTravelDistance >= MaxTravelDistance)
		{
			DestroyCart();
		}
	}
}

bool ALugguageCartActor::CanTraversePortal() const
{
	return HasAuthority() && !bDestroyRequested && ProjectileMovement;
}

FVector ALugguageCartActor::GetPortalTraversalVelocity() const
{
	return ProjectileMovement ? ProjectileMovement->Velocity : GetVelocity();
}

void ALugguageCartActor::ApplyPortalTraversal(
	const FVector& NewLocation,
	const FRotator& NewRotation,
	const FVector& NewVelocity)
{
	SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
	TravelDirection = NewVelocity.GetSafeNormal();
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = NewVelocity;
		ProjectileMovement->UpdateComponentVelocity();
	}
	LastTravelLocation = NewLocation;
	ForceNetUpdate();
}

void ALugguageCartActor::OnCartHit(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, FVector, const FHitResult& Hit)
{
	if (!HasAuthority() || !bPoolActive || bDestroyRequested)
	{
		return;
	}
	if (Hit.ImpactNormal.Z > 0.5f)
	{
		HandleLanding();
		return;
	}
	if (!OtherActor || OtherActor == SourceActor)
	{
		return;
	}

	if (!IsValid(CapturedTarget))
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(OtherActor))
		{
			if (SourceActor && ADLPlayerState::IsHostileActor(SourceActor, TargetCharacter))
			{
				CaptureTarget(TargetCharacter);
				return;
			}
		}

		DestroyCart();
		return;
	}

	if (OtherActor != CapturedTarget)
	{
		HandleWallImpact();
	}
}

void ALugguageCartActor::OnCartOverlap(
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
	(void)SweepResult;

	if (!HasAuthority() || !bPoolActive || bDestroyRequested || CapturedTarget || !OtherActor || OtherActor == SourceActor)
	{
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(OtherActor);
	if (TargetCharacter && SourceActor && ADLPlayerState::IsHostileActor(SourceActor, TargetCharacter))
	{
		CaptureTarget(TargetCharacter);
	}
}

void ALugguageCartActor::HandleLanding()
{
	if (bHasLanded || !ProjectileMovement)
	{
		return;
	}

	bHasLanded = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->Velocity = TravelDirection * ProjectileMovement->MaxSpeed;
}

void ALugguageCartActor::CaptureTarget(ACharacter* TargetCharacter)
{
	CapturedTarget = TargetCharacter;
	USkillDamageLibrary::ApplyDamage(SourceActor, TargetCharacter, DamageCoefficient, BaseDamage, TAG_Data_Damage_Spirit);
	ApplyStun(TargetCharacter);

	if (UCharacterMovementComponent* Movement = TargetCharacter->GetCharacterMovement())
	{
		CapturedMovementState.Capture(TargetCharacter);
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	if (UCapsuleComponent* Capsule = TargetCharacter->GetCapsuleComponent())
	{
		CapturedTargetCollisionEnabled = Capsule->GetCollisionEnabled();
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USceneComponent* Root = TargetCharacter->GetRootComponent())
	{
		PreviousAttachParent = Root->GetAttachParent();
		PreviousAttachSocket = Root->GetAttachSocketName();
	}
	TargetCharacter->AttachToComponent(Mesh, FAttachmentTransformRules::KeepWorldTransform);
	if (const USceneComponent* TargetRoot = TargetCharacter->GetRootComponent())
	{
		TargetCharacter->SetActorRelativeLocation(TargetRoot->GetRelativeLocation() + CapturedTargetAttachOffset, false);
	}
	Collision->IgnoreActorWhenMoving(TargetCharacter, true);
}

void ALugguageCartActor::ApplyStun(ACharacter* TargetCharacter) const
{
	if (!TargetCharacter || !StunEffectSpec.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter))
	{
		TargetASC->ApplyGameplayEffectSpecToSelf(*StunEffectSpec.Data.Get());
	}
}

void ALugguageCartActor::HandleWallImpact()
{
	if (CapturedTarget && (WallImpactDamage > 0.f || WallImpactDamageCoefficient > 0.f))
	{
		USkillDamageLibrary::ApplyDamage(
			SourceActor, CapturedTarget, WallImpactDamageCoefficient, WallImpactDamage, TAG_Data_Damage_Spirit);
	}

	DestroyCart();
}

void ALugguageCartActor::DestroyCart()
{
	if (!HasAuthority() || !bPoolActive || bDestroyRequested)
	{
		return;
	}

	bDestroyRequested = true;
	ExecuteDestroyGameplayCue();
	ReleaseToPool();
}

void ALugguageCartActor::ReleaseToPool()
{
	if (UDLActorPoolSubsystem* Pool = GetWorld() ? GetWorld()->GetSubsystem<UDLActorPoolSubsystem>() : nullptr)
	{
		Pool->Release(this, GetClass());
		return;
	}

	Destroy();
}

void ALugguageCartActor::DeactivateForPool()
{
	ReleaseTarget();

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->ResetInterpolation();
		ProjectileMovement->Deactivate();
	}

	if (Collision)
	{
		if (SourceActor)
		{
			Collision->IgnoreActorWhenMoving(SourceActor, false);
		}
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Collision->SetGenerateOverlapEvents(false);
	}

	SourceActor = nullptr;
	StunEffectSpec = FGameplayEffectSpecHandle();
	CapturedTarget = nullptr;
	AccumulatedTravelDistance = 0.0f;
	SetPoolActive(false);
}

void ALugguageCartActor::ExecuteDestroyGameplayCue()
{
	if (!DestroyGameplayCueTag.IsValid() || !SourceActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
	if (!SourceASC)
	{
		return;
	}

	FGameplayCueParameters CueParams = DLAbilitySupport::MakeCueParameters(
		GetActorLocation(), TravelDirection, SourceActor, SourceActor, nullptr);
	SourceASC->ExecuteGameplayCue(DestroyGameplayCueTag, CueParams);
}

void ALugguageCartActor::ReleaseTarget()
{
	if (!IsValid(CapturedTarget))
	{
		return;
	}

	if (Collision)
	{
		Collision->IgnoreActorWhenMoving(CapturedTarget, false);
	}

	CapturedTarget->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	if (UCapsuleComponent* Capsule = CapturedTarget->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(CapturedTargetCollisionEnabled);
	}
	CapturedMovementState.Restore(CapturedTarget);
	if (PreviousAttachParent.IsValid())
		CapturedTarget->AttachToComponent(PreviousAttachParent.Get(), FAttachmentTransformRules::KeepWorldTransform, PreviousAttachSocket);
	PreviousAttachParent.Reset();

	CapturedTarget = nullptr;
}

void ALugguageCartActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseTarget();
	Super::EndPlay(EndPlayReason);
}

void ALugguageCartActor::ApplyPoolActive(bool bActive)
{
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
	SetActorTickEnabled(bActive);

	if (Collision)
	{
		Collision->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Collision->SetGenerateOverlapEvents(bActive);
	}

	if (Mesh)
	{
		Mesh->SetVisibility(bActive, false);
	}
}


void ALugguageCartActor::SetPoolActive(bool bActive)
{
	if (!HasAuthority()) return;
	bPoolActive = bActive;
	ApplyPoolActive(bActive);
	ForceNetUpdate();
}

void ALugguageCartActor::OnRep_PoolActive()
{
	ApplyPoolActive(bPoolActive);
}

void ALugguageCartActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALugguageCartActor, bPoolActive);
}

void ALugguageCartActor::ResetTravelDistance()
{
	LastTravelLocation = GetActorLocation();
	AccumulatedTravelDistance = 0.f;
}
