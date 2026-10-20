#include "GameCore/Combat/Ability/AbilityTask_HeavyMeleeLunge.h"

#include "Abilities/GameplayAbility.h"
#include "Curves/CurveFloat.h"
#include "GameCore/Character/BaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

UAbilityTask_HeavyMeleeLunge* UAbilityTask_HeavyMeleeLunge::ApplyHeavyMeleeLunge(
	UGameplayAbility* OwningAbility,
	float InDuration,
	float InMaxSpeed,
	UCurveFloat* InSpeedCurve,
	float InDirectionSteeringRate,
	bool bInStopHorizontalVelocityOnEnd
)
{
	UAbilityTask_HeavyMeleeLunge* Task = NewAbilityTask<UAbilityTask_HeavyMeleeLunge>(OwningAbility);
	Task->Duration = InDuration;
	Task->MaxSpeed = InMaxSpeed;
	Task->SpeedCurve = InSpeedCurve;
	Task->DirectionSteeringRate = InDirectionSteeringRate;
	Task->bStopHorizontalVelocityOnEnd = bInStopHorizontalVelocityOnEnd;
	Task->bTickingTask = true;
	return Task;
}

void UAbilityTask_HeavyMeleeLunge::Activate()
{
	Super::Activate();

	const FGameplayAbilityActorInfo* ActorInfo = Ability ? Ability->GetCurrentActorInfo() : nullptr;
	Character = ActorInfo ? Cast<ABaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	MovementComponent = Character.IsValid() ? Character->GetCharacterMovement() : nullptr;

	if (!Character.IsValid() || !MovementComponent.IsValid() || Duration <= 0.f || MaxSpeed <= 0.f)
	{
		FinishLunge();
		return;
	}

	CacheMovementState();
	ApplyLungeMovementState();

	RootMotionSource = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotionSource->InstanceName = TEXT("HeavyMeleeLungeRootMotionForce");
	RootMotionSource->AccumulateMode = MovementComponent->IsFalling()
		? ERootMotionAccumulateMode::Additive
		: ERootMotionAccumulateMode::Override;
	RootMotionSource->Priority = 500;
	RootMotionSource->Duration = Duration;
	CurrentDirection = GetCameraForwardDirection();
	UpdateCharacterRotation(0.f);
	RootMotionSource->Force = CurrentDirection * MaxSpeed * GetSpeedAlpha();
	RootMotionSource->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotionSource->FinishVelocityParams.SetVelocity = bStopHorizontalVelocityOnEnd ? FVector::ZeroVector : RootMotionSource->Force;

	RootMotionSourceID = MovementComponent->ApplyRootMotionSource(StaticCastSharedPtr<FRootMotionSource>(RootMotionSource));
}

void UAbilityTask_HeavyMeleeLunge::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (bFinished)
	{
		return;
	}

	if (!Character.IsValid() || !MovementComponent.IsValid() || !RootMotionSource.IsValid())
	{
		FinishLunge();
		return;
	}

	ElapsedTime += DeltaTime;
	if (ElapsedTime >= Duration)
	{
		FinishLunge();
		return;
	}

	UpdateLungeForce(DeltaTime);
}

void UAbilityTask_HeavyMeleeLunge::OnDestroy(bool bInOwnerFinished)
{
	RemoveRootMotionSource();
	RestoreMovementState();
	Super::OnDestroy(bInOwnerFinished);
}

void UAbilityTask_HeavyMeleeLunge::CacheMovementState()
{
	if (!MovementComponent.IsValid() || bMovementStateCached)
	{
		return;
	}

	bCachedOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
	bCachedUseControllerDesiredRotation = MovementComponent->bUseControllerDesiredRotation;
	CachedRotationRate = MovementComponent->RotationRate;
	bMovementStateCached = true;
}

void UAbilityTask_HeavyMeleeLunge::ApplyLungeMovementState()
{
	if (!Character.IsValid() || !MovementComponent.IsValid())
	{
		return;
	}

	Character->ConsumeMovementInputVector();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = false;

	FVector NewVelocity = MovementComponent->Velocity;
	NewVelocity.X = 0.f;
	NewVelocity.Y = 0.f;
	if (!MovementComponent->IsFalling())
	{
		NewVelocity.Z = 0.f;
	}
	MovementComponent->Velocity = NewVelocity;
}

void UAbilityTask_HeavyMeleeLunge::RestoreMovementState()
{
	if (!MovementComponent.IsValid() || !bMovementStateCached)
	{
		return;
	}

	MovementComponent->bOrientRotationToMovement = bCachedOrientRotationToMovement;
	MovementComponent->bUseControllerDesiredRotation = bCachedUseControllerDesiredRotation;
	MovementComponent->RotationRate = CachedRotationRate;
	bMovementStateCached = false;
}

FVector UAbilityTask_HeavyMeleeLunge::GetCameraForwardDirection() const
{
	if (!Character.IsValid())
	{
		return FVector::ForwardVector;
	}

	const FRotator ControlRotation = Character->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
	FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = Character->GetActorForwardVector().GetSafeNormal();
	}

	return Direction;
}

float UAbilityTask_HeavyMeleeLunge::GetSpeedAlpha() const
{
	const float NormalizedTime = Duration > 0.f ? FMath::Clamp(ElapsedTime / Duration, 0.f, 1.f) : 1.f;
	if (SpeedCurve)
	{
		return FMath::Max(0.f, SpeedCurve->GetFloatValue(NormalizedTime));
	}

	return FMath::Sin(NormalizedTime * PI);
}

void UAbilityTask_HeavyMeleeLunge::UpdateLungeForce(float DeltaTime)
{
	const FVector TargetDirection = GetCameraForwardDirection();
	if (CurrentDirection.IsNearlyZero())
	{
		CurrentDirection = TargetDirection;
	}
	else
	{
		CurrentDirection = FMath::VInterpTo(
			CurrentDirection,
			TargetDirection,
			DeltaTime,
			FMath::Max(0.f, DirectionSteeringRate)
		).GetSafeNormal();
	}

	const float CurrentSpeed = MaxSpeed * GetSpeedAlpha();
	RootMotionSource->AccumulateMode = MovementComponent->IsFalling()
		? ERootMotionAccumulateMode::Additive
		: ERootMotionAccumulateMode::Override;
	RootMotionSource->Force = CurrentDirection * CurrentSpeed;
	RootMotionSource->FinishVelocityParams.SetVelocity = bStopHorizontalVelocityOnEnd ? FVector::ZeroVector : RootMotionSource->Force;

	UpdateCharacterRotation(DeltaTime);
}

void UAbilityTask_HeavyMeleeLunge::UpdateCharacterRotation(float DeltaTime) const
{
	if (!Character.IsValid() || CurrentDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation(0.f, CurrentDirection.Rotation().Yaw, 0.f);
	const FRotator NewRotation = DeltaTime <= 0.f
		? TargetRotation
		: FMath::RInterpTo(
			Character->GetActorRotation(),
			TargetRotation,
			DeltaTime,
			FMath::Max(0.f, DirectionSteeringRate)
		);

	Character->SetActorRotation(NewRotation);
}

void UAbilityTask_HeavyMeleeLunge::FinishLunge()
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	RemoveRootMotionSource();

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFinish.Broadcast();
	}

	EndTask();
}

void UAbilityTask_HeavyMeleeLunge::RemoveRootMotionSource()
{
	if (MovementComponent.IsValid() && RootMotionSourceID != 0)
	{
		MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
		RootMotionSourceID = 0;
	}
}
