// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBase.h"
#include "Doorman/Doorway/DoorwayActor.h"
#include "Common/SkillDamageLibrary.h"
#include "Common/DLActorPoolSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Common/AbilitySupport.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameCore/Debug/DLDebug.h"
#include "GameCore/Struct/DLCollisionTypes.h"
#include "GameCore/Struct/ACharacterAttributeSet.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Common/ProjectileMovementSupport.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Net/UnrealNetwork.h"
#include "TeamUtils.h"
#include "TimerManager.h"

// Sets default values
AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->OnComponentHit.AddDynamic(this, &AProjectileBase::OnProjectileHit);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));

	ProjectileEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileEffect"));
	ProjectileEffectComponent->SetupAttachment(RootComponent);
	ProjectileEffectComponent->bAutoActivate = false;

	ImpactGameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Impact"), false);

	ApplyComponentConfig();
	InitialLifeSpan = 0.0f;
}

void AProjectileBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyComponentConfig();
}

void AProjectileBase::ApplyComponentConfig()
{
	if (Collision)
	{
		Collision->SetSphereRadius(8.0f);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Collision->SetCollisionObjectType(ECC_Projectile);
		Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
		Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Collision->SetCollisionResponseToChannel(ECC_Team1, ECR_Block);
		Collision->SetCollisionResponseToChannel(ECC_Team2, ECR_Block);
		Collision->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
		Collision->SetGenerateOverlapEvents(true);
		Collision->SetNotifyRigidBodyCollision(true);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->UpdatedComponent = Collision;
		ProjectileMovement->InitialSpeed = 10000.0f;
		ProjectileMovement->MaxSpeed = 10000.0f;
		ProjectileMovement->ProjectileGravityScale = 0.0f;
		ProjectileMovement->bRotationFollowsVelocity = true;
		ProjectileMovement->bSweepCollision = true;
		ProjectileMovement->bForceSubStepping = true;
		ProjectileMovement->MaxSimulationTimeStep = 1.0f / 120.0f;
		ProjectileMovement->MaxSimulationIterations = 8;
		ProjectileMovement->bAutoActivate = false;
	}
}

// Called when the game starts or when spawned
void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	if (ProjectileMovement)
	{
		ProjectileMovement->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
	}

	LastDebugLocation = GetActorLocation();

	if (bShowDebug && HasAuthority())
	{
		DrawDebugSphere(
			GetWorld(),
			LastDebugLocation,
			12.0f,
			16,
			FColor::Green,
			false,
			2.0f
		);
	}

	if (DefaultProjectileEffect)
	{
		SetProjectileEffect(DefaultProjectileEffect);
	}

	ArmLifetimeTimer();
}

void AProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority() && IPortalTraversable::TryTraversePortal(this, DeltaTime))
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	if (bShowDebug && HasAuthority()) {
		DrawDebugLine(GetWorld(), LastDebugLocation, CurrentLocation, FColor::Green, false, 3.0f, 0, 2.0f);
	}

	LastDebugLocation = CurrentLocation;
}

void AProjectileBase::Launch(const FVector& Direction)
{
	if (!ProjectileMovement)
	{
		return;
	}

	const FVector LaunchDirection = Direction.IsNearlyZero()
		? GetActorForwardVector().GetSafeNormal()
		: Direction.GetSafeNormal();
	ProjectileMovement->SetUpdatedComponent(Collision);
	ProjectileMovement->Activate(true);
	ProjectileMovement->Velocity = LaunchDirection * ProjectileMovement->InitialSpeed;
	ProjectileMovement->UpdateComponentVelocity();
	ArmLifetimeTimer();
}

FVector AProjectileBase::ResolvePortalExitVelocity(const ADoorwayActor& Entrance,
	const FVector& ExitLocation, const FVector& TransformedVelocity)
{
	const DLProjectileAim::FPortalAim ShotPortalAim = PortalAim;
	PortalAim = DLProjectileAim::FPortalAim();
	if (ShotPortalAim.Entrance.Get() != &Entrance || !ShotPortalAim.Exit.IsValid()
		|| Entrance.GetLinkedDoorway() != ShotPortalAim.Exit.Get()
		|| TransformedVelocity.IsNearlyZero())
	{
		return TransformedVelocity;
	}
	const FVector TargetOffset = ShotPortalAim.Target - ExitLocation;
	const FVector ExitNormal = ShotPortalAim.Exit->GetPortalForwardVector();
	const FVector UnspreadExitDirection = Entrance.ConvertTeleportDirectionToLinkedPortal(
		ShotPortalAim.UnspreadDirection).GetSafeNormal();
	// A target skipped by exit clearance must not turn the projectile back into the doorway.
	if (FVector::DotProduct(TargetOffset, ExitNormal) <= KINDA_SMALL_NUMBER
		|| UnspreadExitDirection.IsNearlyZero())
	{
		return TransformedVelocity;
	}
	// Keep the shot's accuracy spread: correction must not make debuffed shots perfectly accurate.
	const FQuat Spread = FQuat::FindBetweenNormals(UnspreadExitDirection, TransformedVelocity.GetSafeNormal());
	const FVector CorrectedDirection = Spread.RotateVector(TargetOffset.GetSafeNormal());
	if (FVector::DotProduct(CorrectedDirection, ExitNormal) <= KINDA_SMALL_NUMBER)
	{
		return TransformedVelocity;
	}
	return CorrectedDirection * TransformedVelocity.Size();
}

void AProjectileBase::PrepareForPoolActivation()
{
	DeactivateForPool();
	bReleasedToPool = false;
	bProcessingImpact = false;
	bCanTriggerItemEvents = GetClass()->GetDefaultObject<AProjectileBase>()->bCanTriggerItemEvents;
	DamageCoefficient = GetClass()->GetDefaultObject<AProjectileBase>()->DamageCoefficient;
	BaseDamage = GetClass()->GetDefaultObject<AProjectileBase>()->BaseDamage;
	LastDebugLocation = GetActorLocation();

	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(Collision);
	}

	ApplyComponentConfig();
	SetActorEnableCollision(false);
}

void AProjectileBase::ActivateFromPool()
{
	SetPoolActive(true);
}

void AProjectileBase::DeactivateForPool()
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

	if (ProjectileEffectComponent)
	{
		ProjectileEffectComponent->Deactivate();
		ProjectileEffectComponent->SetAsset(nullptr);
	}

	if (Collision)
	{
		Collision->ClearMoveIgnoreActors();
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Collision->SetGenerateOverlapEvents(false);
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	SetOwner(nullptr);
	SetInstigator(nullptr);
	CurrentProjectileEffect = nullptr;
	PortalAim = DLProjectileAim::FPortalAim();
	LastDebugLocation = FVector::ZeroVector;
	SetPoolActive(false);
}

FVector AProjectileBase::GetPortalTraversalVelocity() const
{
	return ProjectileMovement ? ProjectileMovement->Velocity : GetVelocity();
}

void AProjectileBase::ApplyPortalTraversal(
	const FVector& NewLocation,
	const FRotator& NewRotation,
	const FVector& NewVelocity)
{
	const FRotator ExitRotation = ProjectileMovement && ProjectileMovement->bRotationFollowsVelocity
		&& !NewVelocity.IsNearlyZero() ? NewVelocity.Rotation() : NewRotation;
	SetActorLocationAndRotation(NewLocation, ExitRotation, false, nullptr, ETeleportType::TeleportPhysics);
	if (ProjectileMovement)
	{
		DLProjectileMovement::SetVelocity(ProjectileMovement, Collision, NewVelocity);
	}
	ForceNetUpdate();
	LastDebugLocation = NewLocation;
}

void AProjectileBase::SetProjectileEffect(UNiagaraSystem* NewProjectileEffect)
{
	CurrentProjectileEffect = NewProjectileEffect;
	ApplyProjectileEffect();
}

void AProjectileBase::ConfigureCollisionForSource(AActor* SourceActor)
{
	if (!Collision)
	{
		return;
	}

	ApplyComponentConfig();
	Collision->ClearMoveIgnoreActors();
	if (SourceActor)
	{
		Collision->IgnoreActorWhenMoving(SourceActor, true);
	}

	const int32 SourceTeamIdx = SkillTeamUtils::GetTeamIdx(SourceActor);
	if (SourceTeamIdx == INDEX_NONE)
	{
		return;
	}

	const ECollisionChannel SourceTeamChannel =
		(SourceTeamIdx & 1) == 0 ? ECC_Team1 : ECC_Team2;
	Collision->SetCollisionResponseToChannel(SourceTeamChannel, ECR_Ignore);
}

void AProjectileBase::OnRep_ProjectileEffect()
{
	ApplyProjectileEffect();
}

void AProjectileBase::ApplyProjectileEffect()
{
	if (!ProjectileEffectComponent)
	{
		return;
	}

	ProjectileEffectComponent->DeactivateImmediate();
	ProjectileEffectComponent->SetAsset(CurrentProjectileEffect);
	if (CurrentProjectileEffect && bPoolActive)
	{
		ProjectileEffectComponent->ResetSystem();
		ProjectileEffectComponent->Activate(true);
	}
}

void AProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AProjectileBase, bPoolActive);

	DOREPLIFETIME(AProjectileBase, CurrentProjectileEffect);
}

void AProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || !bPoolActive || bReleasedToPool || bProcessingImpact)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[Projectile] Hit ignored on client: Projectile=%s Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OtherActor));
		return;
	}

	AActor* Shooter = GetInstigator();
	if (!OtherActor || OtherActor == Shooter)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[Projectile] Hit ignored: Projectile=%s Target=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OtherActor));
		return;
	}

	if (!SkillTeamUtils::IsHostile(Shooter, OtherActor))
	{
		if (Collision)
		{
			Collision->IgnoreActorWhenMoving(OtherActor, true);
		}
		return;
	}

	bProcessingImpact = true;
	ProcessHostileImpact(Shooter, OtherActor, Hit);
}

void AProjectileBase::ProcessHostileImpact(AActor* Shooter, AActor* OtherActor, const FHitResult& Hit)
{
	float FinalCoefficient = DamageCoefficient;
	float FinalBaseDamage = BaseDamage;
	bool bIsHeadshot = false;
	ECharacterHitZone HitZone = ECharacterHitZone::Body;

	if (const ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(OtherActor))
	{
		HitZone = HitCharacter->GetHitZoneFromBone(Hit.BoneName);
		bIsHeadshot = HitZone == ECharacterHitZone::Head;
	}
	else if (Hit.BoneName == TEXT("head") || Hit.BoneName == TEXT("Head"))
	{
		HitZone = ECharacterHitZone::Head;
		bIsHeadshot = true;
	}

	if (bIsHeadshot)
	{
		FinalCoefficient *= HeadshotDamageMultiplier;
		FinalBaseDamage *= HeadshotDamageMultiplier;
	}

	UAbilitySystemComponent* SourceASC = Shooter
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Shooter)
		: nullptr;
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	const float SourceBulletDamage = SourceASC
		? SourceASC->GetNumericAttribute(UACharacterAttributeSet::GetBulletDamageAttribute())
		: 0.f;
	const float RawBulletDamage =
		SourceBulletDamage * FinalCoefficient + FinalBaseDamage;

	UE_LOG(LogTemp, Verbose,
		TEXT("[Projectile] Hit received: Projectile=%s Target=%s RawDamage=%.1f"),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		RawBulletDamage);

	ExecuteImpactGameplayCue(Hit, Shooter);

	// Item contracts: BulletHit -> damage -> actual health loss notification.
	NotifyBulletHit(Shooter, OtherActor, RawBulletDamage);

	if (TargetASC)
	{
		const float HealthBeforeDamage = TargetASC->GetNumericAttribute(
			UACharacterAttributeSet::GetHealthAttribute());

		const FString HitZoneText = HitZone == ECharacterHitZone::Head ? TEXT("Head") : TEXT("Body");
		const FColor HitDebugColor = bIsHeadshot ? FColor::Red : FColor::Yellow;

		DLDebug::Screen(
			FString::Printf(
				TEXT("[Hit] Target=%s Zone=%s Bone=%s Coefficient=%.2f BaseDamage=%.1f"),
				*GetNameSafe(OtherActor),
				*HitZoneText,
				*Hit.BoneName.ToString(),
				FinalCoefficient,
				FinalBaseDamage
			),
			HitDebugColor,
			2.0f
		);

		if (bShowDebug)
		{
			DrawDebugSphere(
				GetWorld(),
				Hit.ImpactPoint,
				14.0f,
				16,
				HitDebugColor,
				false,
				2.0f,
				0,
				2.0f
			);
		}

		USkillDamageLibrary::ApplyDamage(
			Shooter,
			OtherActor,
			FinalCoefficient,
			FinalBaseDamage,
			TAG_Data_Damage_Bullet);

		NotifyDamageReceived(Shooter, OtherActor, TargetASC, HealthBeforeDamage);
	}

	ReleaseToPool();
}

void AProjectileBase::NotifyBulletHit(AActor* Shooter, AActor* OtherActor, float RawBulletDamage) const
{
	if (!bCanTriggerItemEvents)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Projectile] BulletHit event skipped: Item events disabled. Projectile=%s"), *GetNameSafe(this));
		return;
	}
	if (!Shooter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Projectile] BulletHit event skipped: Instigator is missing. Projectile=%s"), *GetNameSafe(this));
		return;
	}
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("Event.Item.BulletHit"));
	FGameplayEventData Payload;
	Payload.EventTag = Tag;
	Payload.Instigator = Shooter;
	Payload.Target = OtherActor;
	Payload.EventMagnitude = RawBulletDamage;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Shooter, Tag, Payload);
	UE_LOG(LogTemp, Verbose, TEXT("[Projectile] BulletHit event sent: Shooter=%s Target=%s Damage=%.1f"),
		*GetNameSafe(Shooter), *GetNameSafe(OtherActor), RawBulletDamage);
}

void AProjectileBase::NotifyDamageReceived(AActor* Shooter, AActor* OtherActor,
	UAbilitySystemComponent* TargetASC, float HealthBeforeDamage) const
{
	if (!bCanTriggerItemEvents)
	{
		return;
	}
	const float HealthAfterDamage = TargetASC->GetNumericAttribute(UACharacterAttributeSet::GetHealthAttribute());
	const float AppliedDamage = FMath::Max(HealthBeforeDamage - HealthAfterDamage, 0.f);
	if (AppliedDamage <= 0.f)
	{
		return;
	}
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("Event.Item.DamageReceived.Bullet"));
	FGameplayEventData Payload;
	Payload.EventTag = Tag;
	Payload.Instigator = Shooter;
	Payload.Target = OtherActor;
	Payload.EventMagnitude = AppliedDamage;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OtherActor, Tag, Payload);
}

void AProjectileBase::ArmLifetimeTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifetimeTimerHandle);
		if (ProjectileLifetime <= 0.0f) return;
		World->GetTimerManager().SetTimer(
			LifetimeTimerHandle,
			this,
			&ThisClass::ReleaseToPool,
			ProjectileLifetime,
			false);
	}
}

void AProjectileBase::ReleaseToPool()
{
	if (!HasAuthority() || bReleasedToPool)
	{
		return;
	}

	bReleasedToPool = true;
	if (UDLActorPoolSubsystem* Pool = GetWorld() ? GetWorld()->GetSubsystem<UDLActorPoolSubsystem>() : nullptr)
	{
		Pool->Release(this, GetClass());
		return;
	}

	Destroy();
}

void AProjectileBase::ApplyPoolActive(bool bActive)
{
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
	SetActorTickEnabled(bActive);

	if (Collision)
	{
		Collision->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Collision->SetGenerateOverlapEvents(bActive);
	}

	if (ProjectileEffectComponent)
	{
		if (bActive && CurrentProjectileEffect)
		{
			ProjectileEffectComponent->DeactivateImmediate();
			ProjectileEffectComponent->ResetSystem();
			ProjectileEffectComponent->Activate(true);
		}
		else
		{
			ProjectileEffectComponent->Deactivate();
		}
	}
}

void AProjectileBase::ExecuteImpactGameplayCue(const FHitResult& Hit, AActor* Shooter) const
{
	if (!ImpactGameplayCueTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = Shooter
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Shooter)
		: nullptr;
	if (!SourceASC)
	{
		return;
	}

	FGameplayCueParameters CueParams = DLAbilitySupport::MakeCueParameters(
		Hit.ImpactPoint, Hit.ImpactNormal, Shooter, const_cast<AProjectileBase*>(this), this);
	CueParams.PhysicalMaterial = Hit.PhysMaterial;

	SourceASC->ExecuteGameplayCue(ImpactGameplayCueTag, CueParams);
}


void AProjectileBase::SetPoolActive(bool bActive)
{
	if (!HasAuthority()) return;
	bPoolActive = bActive;
	ApplyPoolActive(bActive);
	ForceNetUpdate();
}

void AProjectileBase::OnRep_PoolActive()
{
	ApplyPoolActive(bPoolActive);
}
