#pragma once

#include "CoreMinimal.h"
#include "ZiplineTypes.generated.h"

class AZiplineActor;

USTRUCT(BlueprintType)
struct FZiplineInteractionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AZiplineActor> Zipline = nullptr;

	UPROPERTY(BlueprintReadOnly)
	float InteractionDistanceAlongSpline = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float MountDistanceAlongSpline = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float ViewDot = -1.0f;

	UPROPERTY(BlueprintReadOnly)
	FVector InteractionPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector MountPoint = FVector::ZeroVector;

	bool IsValid() const
	{
		return Zipline != nullptr;
	}
};

USTRUCT(BlueprintType)
struct FZiplineRideParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float RideSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float MountApproachDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	FVector RiderOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	float SplineLength = 0.0f;
};
