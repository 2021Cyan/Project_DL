#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace DLPlayerView
{
	struct FAimView
	{
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
	};
	// Gameplay aim belongs to the supplied pawn, including on the server.
	inline FAimView GetAimView(const AActor& Actor)
	{
		FAimView View{Actor.GetActorLocation(), Actor.GetActorRotation()};
		if (const APawn* Pawn = Cast<APawn>(&Actor))
		{
			View.Rotation = Pawn->GetBaseAimRotation();
			if (AController* Controller = Pawn->GetController()) Controller->GetPlayerViewPoint(View.Location, View.Rotation);
		}
		return View;
	}
	// Presentation currently supports the first local player (no split-screen).
	inline APlayerController* GetLocalController(const UWorld* World)
	{
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller && Controller->IsLocalController() ? Controller : nullptr;
	}
}
