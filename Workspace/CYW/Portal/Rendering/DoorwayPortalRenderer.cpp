// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayPortalRenderer.h"

#include "Doorman/Doorway/DoorwayActor.h"
#include "Doorman/Doorway/DoorwayCameraTransitionComponent.h"
#include "Doorman/Doorway/DoorwayPortalMath.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr int32 PortalMaterialSlot = 1;
	const FName PortalTextureParameter(TEXT("Texture"));
}

void FDoorwayPortalRenderer::Tick(ADoorwayActor& Doorway, float DeltaSeconds)
{
	UpdateSceneCapture(Doorway);
	SetClipPlanes(Doorway);
	ResolutionCheckElapsed += DeltaSeconds;
	if (ResolutionCheckElapsed >= Doorway.GetResolutionCheckInterval())
	{
		ResolutionCheckElapsed = 0.0f;
		CheckResolution(Doorway);
	}
}

void FDoorwayPortalRenderer::InitializePortal(ADoorwayActor& Doorway)
{
	Doorway.SetTickGroup(TG_PostUpdateWork);
	USkeletalMeshComponent* DoorMesh = Doorway.GetDoorMesh();
	if (!DoorMesh)
	{
		return;
	}

	UMaterialInterface* MaterialSource = Doorway.GetPortalMaterial()
		? Doorway.GetPortalMaterial()
		: DoorMesh->GetMaterial(PortalMaterialSlot);
	if (!Doorway.GetPortalMaterialInstance())
		Doorway.SetPortalMaterialInstance(DoorMesh->CreateDynamicMaterialInstance(PortalMaterialSlot, MaterialSource));
	if (UMaterialInstanceDynamic* PortalMat = Doorway.GetPortalMaterialInstance())
	{
		DoorMesh->SetMaterial(PortalMaterialSlot, PortalMat);
	}

	int32 Width = 0;
	int32 Height = 0;
	if (!GetDesiredRenderTargetSize(Doorway, Width, Height))
	{
		return;
	}

	if (!Doorway.GetPortalRenderTarget())
		Doorway.SetPortalRenderTarget(UKismetRenderingLibrary::CreateRenderTarget2D(
			&Doorway, Width, Height, RTF_RGBA16f, FLinearColor::Black, false, false));
	if (UMaterialInstanceDynamic* PortalMat = Doorway.GetPortalMaterialInstance())
	{
		if (UTextureRenderTarget2D* PortalRT = Doorway.GetPortalRenderTarget())
		{
			PortalMat->SetTextureParameterValue(PortalTextureParameter, PortalRT);
		}
	}
	Doorway.RefreshPortalLink();
	SetClipPlanes(Doorway);
}

bool FDoorwayPortalRenderer::GetDesiredRenderTargetSize(
	ADoorwayActor& Doorway,
	int32& OutWidth,
	int32& OutHeight)
{
	const float Quality = Doorway.GetPortalQuality();
	if (!FMath::IsFinite(Quality) || Quality <= 0.f) return false;
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(&Doorway);
	OutWidth = FMath::TruncToInt(FMath::Min(ViewportSize.X * Quality, 8192.0));
	OutHeight = FMath::TruncToInt(FMath::Min(ViewportSize.Y * Quality, 8192.0));
	return OutWidth > 0 && OutHeight > 0;
}

void FDoorwayPortalRenderer::UpdateSceneCapture(ADoorwayActor& Doorway)
{
	ADoorwayActor* LinkedDoorway = Doorway.GetLinkedDoorway();
	if (!IsValid(LinkedDoorway) || !LinkedDoorway->GetPortalCamera())
	{
		return;
	}

	UDoorwayCameraTransitionComponent* CameraTransitionComponent =
		Doorway.GetCameraTransitionComponent();
	if (!CameraTransitionComponent
		|| !CameraTransitionComponent->CacheLocalReferences(Doorway))
	{
		return;
	}
	APlayerCameraManager* CameraManager =
		CameraTransitionComponent->GetCachedLocalCameraManager();
	if (!CameraManager)
	{
		return;
	}
	const FMinimalViewInfo& PlayerView = CameraManager->GetCameraCacheView();

	LinkedDoorway->GetPortalCamera()->SetWorldLocationAndRotation(
		Doorway.ConvertLocationToLinkedPortal(PlayerView.Location),
		Doorway.ConvertRotationToLinkedPortal(PlayerView.Rotation));
}

void FDoorwayPortalRenderer::CheckResolution(ADoorwayActor& Doorway)
{
	UTextureRenderTarget2D* PortalRT = Doorway.GetPortalRenderTarget();
	if (!PortalRT)
	{
		InitializePortal(Doorway);
		return;
	}

	int32 Width = 0;
	int32 Height = 0;
	if (GetDesiredRenderTargetSize(Doorway, Width, Height)
		&& (PortalRT->SizeX != Width || PortalRT->SizeY != Height))
	{
		UKismetRenderingLibrary::ResizeRenderTarget2D(PortalRT, Width, Height);
	}
}

void FDoorwayPortalRenderer::SetClipPlanes(ADoorwayActor& Doorway)
{
	USceneCaptureComponent2D* PortalCamera = Doorway.GetPortalCamera();
	if (!PortalCamera || !Doorway.GetDoorMesh())
	{
		return;
	}

	const DoorwayPortalMath::FDoorwayPortalFrame ClipFrame = Doorway.GetSceneCaptureClipFrame();
	PortalCamera->bEnableClipPlane = true;
	PortalCamera->ClipPlaneBase = ClipFrame.PlaneLocation;
	PortalCamera->ClipPlaneNormal = ClipFrame.PlaneNormal;
}
