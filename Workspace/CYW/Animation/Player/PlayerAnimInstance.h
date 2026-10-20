#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS()
class UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool IsMoving() const { return Speed > 3.f; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE float GetYawSpeed() const { return YawSpeed; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE float GetSmoothedYawSpeed() const { return SmoothedYawSpeed; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool GetIsJumping() const { return bIsJumping; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool GetShouldPlayJumpStart() const { return bShouldPlayJumpStart; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool GetShouldUseGroundLocomotion() const { return !bIsInAir; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool GetIsInAir() const { return bIsInAir; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool GetIsFalling() const { return bIsFalling; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool GetIsRising() const { return bIsRising; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE bool GetHasDoubleJumped() const { return bHasDoubleJumped; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE float GetVerticalVelocity() const { return VerticalVelocity; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE float GetLookYawOffset() const { return LookRotOffset.Yaw; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE float GetLookPitchOffset() const { return LookRotOffset.Pitch; }

	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	FORCEINLINE float GetMoveDirectionAngle() const { return MoveDirectionAngle; }

private:
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;

	UPROPERTY()
	UCharacterMovementComponent* OwnerMovementComponent = nullptr;

	float Speed = 0.f;
	float YawSpeed = 0.f;
	float SmoothedYawSpeed = 0.f;
	FRotator BodyPrevRot = FRotator::ZeroRotator;
	FRotator LookRotOffset = FRotator::ZeroRotator;

	bool bIsJumping = false;
	bool bIsInAir = false;
	bool bIsFalling = false;
	bool bIsRising = false;
	bool bHasDoubleJumped = false;
	bool bShouldPlayJumpStart = false;
	float VerticalVelocity = 0.f;

	UPROPERTY(EditAnywhere, Category = "Animation")
	float SmoothAlpha = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Animation|Jump")
	float JumpingVelocityThreshold = 10.f;

	UPROPERTY(EditAnywhere, Category = "Animation|Jump")
	float FallingVelocityThreshold = -10.f;

	float MoveDirectionAngle = 0.f;
};
