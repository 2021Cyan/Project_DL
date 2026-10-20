// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "AnimationResetModifier.generated.h"

class UAnimSequence;

UCLASS()
class TEAMPROJECT_DLEDITOR_API UAnimationResetModifier : public UAnimationModifier
{
	GENERATED_BODY()

public:
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;

private:
	/** Rotation과 전용 Scale을 적용할 루트 본 */
	UPROPERTY(EditAnywhere, Category = "Animation Reset")
	FName RootBoneName = TEXT("root");

	/** root 본에 적용할 Scale */
	UPROPERTY(EditAnywhere, Category = "Animation Reset")
	FVector3f RootScale = FVector3f(0.00254f, 0.00254f, 0.00254f);
};
