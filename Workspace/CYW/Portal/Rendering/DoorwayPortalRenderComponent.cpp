// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayPortalRenderComponent.h"

UDoorwayPortalRenderComponent::UDoorwayPortalRenderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDoorwayPortalRenderComponent::TickPortalRenderer(ADoorwayActor& Doorway, float DeltaSeconds)
{
	Renderer.Tick(Doorway, DeltaSeconds);
}

void UDoorwayPortalRenderComponent::InitializePortal(ADoorwayActor& Doorway)
{
	Renderer.InitializePortal(Doorway);
}

bool UDoorwayPortalRenderComponent::GetDesiredRenderTargetSize(
	ADoorwayActor& Doorway,
	int32& OutWidth,
	int32& OutHeight)
{
	return Renderer.GetDesiredRenderTargetSize(Doorway, OutWidth, OutHeight);
}

void UDoorwayPortalRenderComponent::UpdateSceneCapture(ADoorwayActor& Doorway)
{
	Renderer.UpdateSceneCapture(Doorway);
}

void UDoorwayPortalRenderComponent::CheckResolution(ADoorwayActor& Doorway)
{
	Renderer.CheckResolution(Doorway);
}

void UDoorwayPortalRenderComponent::SetClipPlanes(ADoorwayActor& Doorway)
{
	Renderer.SetClipPlanes(Doorway);
}
