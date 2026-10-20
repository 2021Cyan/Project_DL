// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class ADoorwayActor;

class FDoorwayPortalRenderer
{
public:
	void Tick(ADoorwayActor& Doorway, float DeltaSeconds);
	void InitializePortal(ADoorwayActor& Doorway);
	bool GetDesiredRenderTargetSize(ADoorwayActor& Doorway, int32& OutWidth, int32& OutHeight);
	void UpdateSceneCapture(ADoorwayActor& Doorway);
	void CheckResolution(ADoorwayActor& Doorway);
	void SetClipPlanes(ADoorwayActor& Doorway);

private:
	float ResolutionCheckElapsed = 0.0f;
};
