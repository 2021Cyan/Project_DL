// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Doorman/Doorway/DoorwayPortalMath.h"
#include "GameFramework/Actor.h"
#include "DoorwayActor.generated.h"

class UDoorwayCameraTransitionComponent;
class UDoorwayPortalRenderComponent;
class UDoorwayTeleportComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UArrowComponent;
class UBoxComponent;
class USceneCaptureComponent2D;
class UCameraComponent;
class UAnimMontage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextureRenderTarget2D;
class ACharacter;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EDoorwayEndpointType : uint8
{
	Entrance,
	Exit
};

UCLASS()
class TEAMPROJECT_DL_API ADoorwayActor : public AActor
{
	GENERATED_BODY()

public:
	ADoorwayActor();
	virtual void Tick(float DeltaSeconds) override;

	void InitializeDoorwayEndpoint(EDoorwayEndpointType InEndpointType, ADoorwayActor* InLinkedDoorway);
	void SetLinkedDoorway(ADoorwayActor* InLinkedDoorway);
	void PlayConnectedMontage();

	UFUNCTION(BlueprintPure, Category = "Doorway")
	EDoorwayEndpointType GetEndpointType() const { return EndpointType; }

	UFUNCTION(BlueprintPure, Category = "Doorway")
	ADoorwayActor* GetLinkedDoorway() const { return LinkedDoorway; }

	USkeletalMeshComponent* GetDoorMesh() const { return DoorMesh; }
	USceneCaptureComponent2D* GetPortalCamera() const { return PortalCamera; }
	UCameraComponent* GetPlayerCamera() const { return PlayerCamera; }
	UBoxComponent* GetPlaneIntersectionTest() const { return PlaneIntersectionTest; }
	UBoxComponent* GetTeleportDetection() const { return TeleportDetection; }
	UBoxComponent* GetPlayerNearbyBounds() const { return PlayerNearbyBounds; }
	UDoorwayCameraTransitionComponent* GetCameraTransitionComponent() const { return CameraTransitionComponent; }
	UMaterialInstanceDynamic* GetPortalMaterialInstance() const { return PortalMat; }
	UMaterialInterface* GetPortalMaterial() const { return PortalMaterial; }
	UTextureRenderTarget2D* GetPortalRenderTarget() const { return PortalRT; }
	float GetPortalQuality() const { return PortalQuality; }
	float GetExitSafeDistance() const { return ExitSafeDistance; }
	float GetCameraTransitionMaximumDuration() const { return CameraTransitionMaximumDuration; }
	float GetCameraReturnBlendDuration() const { return CameraReturnBlendDuration; }
	float GetSideEntryCameraTransitionDotThreshold() const { return SideEntryCameraTransitionDotThreshold; }
	float GetResolutionCheckInterval() const { return ResolutionCheckInterval; }
	void SetPortalMaterialInstance(UMaterialInstanceDynamic* InPortalMat) { PortalMat = InPortalMat; }
	void SetPortalRenderTarget(UTextureRenderTarget2D* InPortalRT) { PortalRT = InPortalRT; }
	void RefreshPortalLink();

	void BeginCameraTransitionFromTeleport(const FRotator& DestinationControlRotation);
	bool TeleportTraversableFromTrace(AActor* TraversableActor, const FVector& SourceLocation);

	FVector ConvertLocationToLinkedPortal(const FVector& WorldLocation) const;
	FRotator ConvertRotationToLinkedPortal(const FRotator& WorldRotation) const;
	FVector ConvertTeleportDirectionToLinkedPortal(const FVector& WorldDirection) const;
	FRotator ConvertTeleportRotationToLinkedPortal(const FRotator& WorldRotation) const;
	FVector ConvertTeleportLocationToLinkedPortal(const FVector& WorldLocation) const;
	FVector ConvertVelocityToLinkedPortal(FVector OldVelocity) const;
	FVector GetTeleportPlaneLocation() const;
	FVector GetPortalForwardVector() const;
	DoorwayPortalMath::FDoorwayPortalFrame GetRenderPlaneFrame() const;
	DoorwayPortalMath::FDoorwayPortalFrame GetTeleportCrossingFrame() const;
	DoorwayPortalMath::FDoorwayPortalFrame GetTraversableTraceFrame() const;
	DoorwayPortalMath::FDoorwayPortalFrame GetSceneCaptureClipFrame() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void DesyncCameraAndCharacter();
	void UpdateCameraSynchronization();
	void UpdateLinkedCamera();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway")
	TObjectPtr<USkeletalMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway")
	TObjectPtr<UArrowComponent> ForwardDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway")
	TObjectPtr<USceneCaptureComponent2D> PortalCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway|Camera")
	TObjectPtr<UCameraComponent> PlayerCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway|Camera")
	TObjectPtr<UBoxComponent> PlaneIntersectionTest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway")
	TObjectPtr<UBoxComponent> TeleportDetection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway")
	TObjectPtr<UBoxComponent> PlayerNearbyBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway|Component")
	TObjectPtr<UDoorwayCameraTransitionComponent> CameraTransitionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway|Component")
	TObjectPtr<UDoorwayPortalRenderComponent> PortalRenderComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway|Component")
	TObjectPtr<UDoorwayTeleportComponent> TeleportComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Doorway|Portal")
	TObjectPtr<UMaterialInstanceDynamic> PortalMat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doorway|Portal")
	TObjectPtr<UMaterialInterface> PortalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doorway|Portal", meta = (ClampMin = "0.0"))
	float PortalQuality = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doorway|Teleport", meta = (ClampMin = "0.0"))
	float ExitSafeDistance = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doorway|Teleport", meta = (ClampMin = "0.0"))
	float TeleportPlaneOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doorway|Camera", meta = (ClampMin = "0.1"))
	float CameraTransitionMaximumDuration = 1.5f;

	/** Duration of the final camera handoff after the entrance-view distance is reached. Zero returns immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doorway|Camera", meta = (ClampMin = "0.0", Units = "s"))
	float CameraReturnBlendDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doorway|Camera",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SideEntryCameraTransitionDotThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doorway|Portal", meta = (ClampMin = "0.05"))
	float ResolutionCheckInterval = 0.25f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Doorway|Portal")
	TObjectPtr<UTextureRenderTarget2D> PortalRT;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Doorway|Animation")
	TObjectPtr<UAnimMontage> ConnectedMontage;

	UPROPERTY(ReplicatedUsing = OnRep_EndpointType, BlueprintReadOnly, Category = "Doorway")
	EDoorwayEndpointType EndpointType = EDoorwayEndpointType::Entrance;

	UPROPERTY(ReplicatedUsing = OnRep_LinkedDoorway, BlueprintReadOnly, Category = "Doorway")
	TObjectPtr<ADoorwayActor> LinkedDoorway;

	UPROPERTY(ReplicatedUsing = OnRep_PlayConnectedMontage)
	int32 ConnectedMontagePlayCount = 0;
	int32 LastPlayedConnectedMontageCount = 0;
	TWeakObjectPtr<ADoorwayActor> AppliedLinkedDoorway;

	UFUNCTION()
	void OnRep_EndpointType();

	UFUNCTION()
	void OnRep_LinkedDoorway();

	UFUNCTION()
	void OnRep_PlayConnectedMontage();

	UFUNCTION()
	void OnPlayerNearbyBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnPlayerNearbyEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayConnectedMontage();

	void TryPlayConnectedMontage();
	void InitializePortal();

	UFUNCTION(BlueprintPure, Category = "Doorway|Portal")
	FVector UpdateVelocity(FVector OldVelocity) const;

	FVector TransformDirectionThroughPortal(const FVector& WorldDirection) const;
	FRotator TransformRotationThroughPortal(const FRotator& WorldRotation) const;
	FVector TransformLocationThroughPortal(const FVector& WorldLocation) const;
	FVector TransformTeleportDirection(const FVector& WorldDirection) const;
	FRotator TransformTeleportRotation(const FRotator& WorldRotation) const;
	FVector TransformTeleportLocation(const FVector& WorldLocation) const;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
