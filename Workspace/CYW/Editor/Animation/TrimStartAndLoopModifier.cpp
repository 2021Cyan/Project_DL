// Fill out your copyright notice in the Description page of Project Settings.


#include "TrimStartAndLoopModifier.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceHelpers.h"

void UTrimStartAndLoopModifier::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
#if WITH_EDITOR
	if (!AnimationSequence)
	{
		return;
	}

	const int32 NumFrames = AnimationSequence->GetNumberOfSampledKeys();

	if (NumFrames <= RemoveStartFrameCount + 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Animation too short: %s"), *AnimationSequence->GetName());
		return;
	}

	const FFrameRate FrameRate = AnimationSequence->GetSamplingFrameRate();

	const FFrameNumber StartFrame(0);
	const FFrameNumber EndFrame(RemoveStartFrameCount);

	AnimationSequence->Modify();

	// 0~2 프레임 제거
	const TRange<FFrameNumber> TrimRange(
		TRangeBound<FFrameNumber>::Inclusive(StartFrame),
		TRangeBound<FFrameNumber>::Exclusive(EndFrame)
	);

	const bool bTrimmed = UE::Anim::AnimationData::Trim(AnimationSequence, TrimRange);

	if (!bTrimmed)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to trim animation: %s"), *AnimationSequence->GetName());
		return;
	}

	// 루프 보간 추가
	UE::Anim::AnimationData::AddLoopingInterpolation(AnimationSequence);

	AnimationSequence->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("Trimmed first %d frames and added looping interpolation: %s"),
		RemoveStartFrameCount,
		*AnimationSequence->GetName());
#endif
}
