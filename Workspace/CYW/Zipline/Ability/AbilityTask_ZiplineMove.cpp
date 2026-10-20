#include "Zipline/Ability/AbilityTask_ZiplineMove.h"

#include "GameCore/Character/BaseCharacter.h"
#include "Components/SplineComponent.h"
#include "Zipline/ZiplineActor.h"
#include "Zipline/ZiplineRideMath.h"

UAbilityTask_ZiplineMove* UAbilityTask_ZiplineMove::MoveAlongZipline(
	UGameplayAbility* OwningAbility,
	ABaseCharacter* InCharacter,
	AZiplineActor* InZipline,
	const FZiplineInteractionResult& InteractionResult
)
{
	UAbilityTask_ZiplineMove* Task = NewAbilityTask<UAbilityTask_ZiplineMove>(OwningAbility);
	Task->Character = InCharacter;
	Task->Zipline = InZipline;
	Task->DistanceAlongSpline = InteractionResult.MountDistanceAlongSpline;
	Task->bTickingTask = true;
	Task->MoveState = EZiplineMoveState::MountApproach;
	return Task;
}

void UAbilityTask_ZiplineMove::Activate()
{
	Super::Activate();

	if (!Character.IsValid() || !Zipline.IsValid() || !Zipline->GetSpline() || Zipline->GetSplineLength() <= 0.0f)
	{
		FinishTask(false);
		return;
	}

	RideParams = Zipline->MakeRideParams();
	if (!FMath::IsFinite(RideParams.RideSpeed) || RideParams.RideSpeed <= 0.f)
	{
		FinishTask(false);
		return;
	}
	DistanceAlongSpline = FMath::Clamp(DistanceAlongSpline, 0.f, RideParams.SplineLength);

	const FVector Tangent = Zipline->GetSpline()->GetDirectionAtDistanceAlongSpline(
		DistanceAlongSpline,
		ESplineCoordinateSpace::World
	);
	RideDirection = FVector::DotProduct(Character->GetActorForwardVector(), Tangent) >= 0.0f ? 1.0f : -1.0f;
	ApproachStartLocation = Character->GetActorLocation();
	ApproachStartRotation = Character->GetActorQuat();
}

void UAbilityTask_ZiplineMove::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (MoveState == EZiplineMoveState::Finished)
	{
		return;
	}

	if (!Character.IsValid() || !Zipline.IsValid())
	{
		FinishTask(false);
		return;
	}

	if (!ShouldUpdateCharacter())
	{
		return;
	}

	switch (MoveState)
	{
	case EZiplineMoveState::MountApproach:
		UpdateMountApproach(DeltaTime);
		return;

	case EZiplineMoveState::Riding:
		UpdateRide(DeltaTime);
		return;

	case EZiplineMoveState::Finished:
	default:
		return;
	}
}

void UAbilityTask_ZiplineMove::StartRiding()
{
	if (MoveState != EZiplineMoveState::MountApproach || !Character.IsValid() || !Zipline.IsValid())
	{
		return;
	}

	MoveState = EZiplineMoveState::Riding;
	if (!ShouldUpdateCharacter()) return;
	Character->SetActorLocationAndRotation(
		GetRideLocation(DistanceAlongSpline),
		GetRideRotation(DistanceAlongSpline),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);
}

bool UAbilityTask_ZiplineMove::ShouldUpdateCharacter() const
{
	return Character.IsValid() && (Character->HasAuthority() || Character->IsLocallyControlled());
}

FVector UAbilityTask_ZiplineMove::GetRideLocation(float InDistanceAlongSpline) const
{
	return DLZiplineRide::GetLocation(Zipline.IsValid() ? Zipline->GetSpline() : nullptr, InDistanceAlongSpline, RideParams.RiderOffset);
}

FRotator UAbilityTask_ZiplineMove::GetRideRotation(float InDistanceAlongSpline) const
{
	return DLZiplineRide::GetRotation(Zipline.IsValid() ? Zipline->GetSpline() : nullptr, InDistanceAlongSpline, RideDirection);
}

void UAbilityTask_ZiplineMove::UpdateMountApproach(float DeltaTime)
{
	ApproachElapsedTime += DeltaTime;
	const float Duration = RideParams.MountApproachDuration;
	const float Alpha = Duration > 0.0f
		? FMath::Clamp(ApproachElapsedTime / Duration, 0.0f, 1.0f)
		: 1.0f;
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	const FVector TargetLocation = GetRideLocation(DistanceAlongSpline);
	const FQuat TargetRotation = GetRideRotation(DistanceAlongSpline).Quaternion();

	Character->SetActorLocationAndRotation(
		FMath::Lerp(ApproachStartLocation, TargetLocation, SmoothAlpha),
		FQuat::Slerp(ApproachStartRotation, TargetRotation, SmoothAlpha),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);
}

void UAbilityTask_ZiplineMove::UpdateRide(float DeltaTime)
{
	DistanceAlongSpline += RideParams.RideSpeed * RideDirection * DeltaTime;
	const float SplineLength = RideParams.SplineLength;
	const bool bReachedEnd = DistanceAlongSpline <= 0.0f || DistanceAlongSpline >= SplineLength;
	DistanceAlongSpline = FMath::Clamp(DistanceAlongSpline, 0.0f, SplineLength);

	Character->SetActorLocationAndRotation(
		GetRideLocation(DistanceAlongSpline),
		GetRideRotation(DistanceAlongSpline),
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	if (bReachedEnd)
	{
		FinishTask(true);
	}
}

void UAbilityTask_ZiplineMove::FinishTask(bool bSucceeded)
{
	if (MoveState == EZiplineMoveState::Finished)
	{
		return;
	}

	MoveState = EZiplineMoveState::Finished;

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bSucceeded)
		{
			OnFinished.Broadcast();
		}
		else
		{
			OnFailed.Broadcast();
		}
	}

	EndTask();
}
