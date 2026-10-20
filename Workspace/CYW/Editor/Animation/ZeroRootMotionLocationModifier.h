// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "ZeroRootMotionLocationModifier.generated.h"

/**
 * 
 */
UCLASS()
class TEAMPROJECT_DLEDITOR_API UZeroRootMotionLocationModifier : public UAnimationModifier
{
	GENERATED_BODY()
public:
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
};
