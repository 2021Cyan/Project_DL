// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "TrimStartAndLoopModifier.generated.h"

/**
 * 
 */
UCLASS()
class TEAMPROJECT_DLEDITOR_API UTrimStartAndLoopModifier : public UAnimationModifier
{
	GENERATED_BODY()

public:
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;

private:
	UPROPERTY(EditAnywhere, Category = "Trim")
	int32 RemoveStartFrameCount = 3;
};
