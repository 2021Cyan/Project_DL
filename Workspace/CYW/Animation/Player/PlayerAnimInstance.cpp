#include "Animation/PlayerAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TeamProject_DL/GameCore/Character/BaseCharacter.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetMathLibrary.h"

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());

	if (OwnerCharacter)
	{
		OwnerMovementComponent = OwnerCharacter->GetCharacterMovement();
		BodyPrevRot = OwnerCharacter->GetActorRotation();
	}
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());

		if (OwnerCharacter)
		{
			OwnerMovementComponent = OwnerCharacter->GetCharacterMovement();
			BodyPrevRot = OwnerCharacter->GetActorRotation();
		}
	}

	if (!OwnerCharacter)
	{
		return;
	}

	const FVector Velocity = OwnerCharacter->GetVelocity();
	const FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
	VerticalVelocity = Velocity.Z;

	Speed = HorizontalVelocity.Size();

	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const FVector Right = OwnerCharacter->GetActorRightVector();
	const FVector NormalizedVelocity = HorizontalVelocity.GetSafeNormal();

	const float ForwardDot = FVector::DotProduct(Forward, NormalizedVelocity);
	const float RightDot = FVector::DotProduct(Right, NormalizedVelocity);

	MoveDirectionAngle = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));

	const FRotator BodyRot = OwnerCharacter->GetActorRotation();
	const FRotator BodyRotDelta = UKismetMathLibrary::NormalizedDeltaRotator(BodyRot, BodyPrevRot);
	BodyPrevRot = BodyRot;

	if (DeltaSeconds > 0.f)
	{
		YawSpeed = BodyRotDelta.Yaw / DeltaSeconds;
	}

	SmoothedYawSpeed = FMath::FInterpTo(
		SmoothedYawSpeed,
		YawSpeed,
		DeltaSeconds,
		SmoothAlpha
	);

	const FRotator AimRot = OwnerCharacter->GetBaseAimRotation();
	LookRotOffset = UKismetMathLibrary::NormalizedDeltaRotator(AimRot, BodyRot);

	if (const ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(OwnerCharacter))
	{
		LookRotOffset.Yaw = BaseCharacter->GetAimYaw();
		if (BaseCharacter->ASC)
		{
			static const FGameplayTag HasDoubleJumpedTag = FGameplayTag::RequestGameplayTag(
				FName("State.Movement.HasDoubleJumped"), false);
			bHasDoubleJumped = HasDoubleJumpedTag.IsValid() && BaseCharacter->ASC->HasMatchingGameplayTag(HasDoubleJumpedTag);
		}
		else
		{
			bHasDoubleJumped = false;
		}
	}
	else
	{
		bHasDoubleJumped = false;
	}

	if (OwnerMovementComponent)
	{
		bIsInAir = OwnerMovementComponent->IsFalling();
		bIsRising = bIsInAir && VerticalVelocity > JumpingVelocityThreshold;
		bIsFalling = bIsInAir && VerticalVelocity < FallingVelocityThreshold;
		bShouldPlayJumpStart = bIsRising && !bHasDoubleJumped;
		bIsJumping = bShouldPlayJumpStart;
	}
	else
	{
		bIsInAir = false;
		bIsRising = false;
		bIsFalling = false;
		bShouldPlayJumpStart = false;
		bIsJumping = false;
	}
}

void UPlayerAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}
