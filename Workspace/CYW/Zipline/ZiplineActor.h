#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zipline/ZiplineTypes.h"
#include "ZiplineActor.generated.h"

class USplineComponent;
class ACharacter;
class UZiplineMountIndicatorComponent;
class UZiplineSplineVisualComponent;

UCLASS(Blueprintable)
class TEAMPROJECT_DL_API AZiplineActor : public AActor
{
	GENERATED_BODY()

public:
	AZiplineActor();
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zipline")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Zipline|Components")
	TObjectPtr<UZiplineSplineVisualComponent> SplineVisualComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Zipline|Components")
	TObjectPtr<UZiplineMountIndicatorComponent> MountIndicatorComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline", meta = (ClampMin = "1.0"))
	float RideSpeed = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline", meta = (ClampMin = "0.0"))
	float MountApproachDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline")
	FVector RiderOffset = FVector(0.0f, 0.0f, -100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline|Team", meta = (ClampMin = "0"))
	int32 TeamIdx = 0;

	int32 GetTeamIdx() const { return TeamIdx; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline|Team")
	bool bRestrictInteractionToTeam = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline", meta = (ClampMin = "1.0", DisplayName = "View Search Distance"))
	float InteractionDistance = 1250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline", meta = (ClampMin = "1.0"))
	float MountSearchDistanceAlongSpline = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline", meta = (ClampMin = "1.0"))
	FVector InteractionBoxExtent = FVector(450.0f, 450.0f, 1250.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinimumViewDot = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline|Debug")
	bool bShowDebug = false;

	USplineComponent* GetSpline() const { return Spline; }
	UZiplineSplineVisualComponent* GetSplineVisualComponent() const { return SplineVisualComponent; }
	float FindClosestDistanceAlongSpline(const FVector& WorldLocation) const;
	bool FindMountDistanceAlongSpline(
		const ACharacter* Character,
		float InteractionDistanceAlongSpline,
		float& OutDistanceAlongSpline
	) const;
	bool FindMountDistanceAlongSpline(const ACharacter* Character, float InteractionDistanceAlongSpline,
		float& OutDistanceAlongSpline, const FVector& EyeLocation, const FVector& ViewDirection) const;
	bool CanReachInteractionPoint(const ACharacter* Character, float& OutDistanceAlongSpline) const;
	bool CanInteractWith(const ACharacter* Character) const;
	FVector GetRideLocation(float DistanceAlongSpline) const;
	FRotator GetRideRotation(float DistanceAlongSpline, float Direction) const;
	float GetSplineLength() const;
	FZiplineRideParams MakeRideParams() const;
	static AZiplineActor* FindViewedZipline(const ACharacter* Character);

private:
	void ShowDebug();
};
