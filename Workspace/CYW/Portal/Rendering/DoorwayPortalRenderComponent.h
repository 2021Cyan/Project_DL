// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Doorman/Doorway/DoorwayPortalRenderer.h"
#include "DoorwayPortalRenderComponent.generated.h"

class ADoorwayActor;

UCLASS(ClassGroup = (Doorway), meta = (BlueprintSpawnableComponent))
class TEAMPROJECT_DL_API UDoorwayPortalRenderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorwayPortalRenderComponent();

	void TickPortalRenderer(ADoorwayActor& Doorway, float DeltaSeconds);
	void InitializePortal(ADoorwayActor& Doorway);
	bool GetDesiredRenderTargetSize(ADoorwayActor& Doorway, int32& OutWidth, int32& OutHeight);
	void UpdateSceneCapture(ADoorwayActor& Doorway);
	void CheckResolution(ADoorwayActor& Doorway);
	void SetClipPlanes(ADoorwayActor& Doorway);

private:
	FDoorwayPortalRenderer Renderer;
};
