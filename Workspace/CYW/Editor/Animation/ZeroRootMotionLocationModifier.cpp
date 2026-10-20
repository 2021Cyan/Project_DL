#include "Animation/ZeroRootMotionLocationModifier.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "AnimationBlueprintLibrary.h"

void UZeroRootMotionLocationModifier::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
#if WITH_EDITOR
	if (!AnimationSequence) return;

	const FName RootMotionBoneName(TEXT("root_motion"));
	const FName ProxyRootBoneName(TEXT("models_heroes_wip_lash_lash_vmdl_c_ProxyTrueRootJoint"));

	const TArray<FName> BoneNames =
	{
		RootMotionBoneName,
		ProxyRootBoneName
	};

	int32 NumFrames = 0;
	UAnimationBlueprintLibrary::GetNumFrames(AnimationSequence, NumFrames);

	if (NumFrames <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Frames : %s"), *AnimationSequence->GetName());
		return;
	}

	const FQuat4f ZeroRotation = FQuat4f::Identity;
	const FQuat4f ProxyRootRotation(FQuat(FRotator(0.0f, 0.0f, 90.0f)));
	const FVector3f FixedScale(0.0254f, 0.0254f, 0.0254f);

	IAnimationDataController& Controller = AnimationSequence->GetController();

	AnimationSequence->Modify();
	Controller.OpenBracket(FText::FromString("Fix Root Bone Transform"));

	for (const FName& BoneName : BoneNames)
	{
		TArray<FVector3f> PosKeys;
		TArray<FQuat4f> RotKeys;
		TArray<FVector3f> ScaleKeys;

		PosKeys.SetNum(NumFrames);
		RotKeys.SetNum(NumFrames);
		ScaleKeys.SetNum(NumFrames);

		for (int32 Frame = 0; Frame < NumFrames; ++Frame)
		{
			PosKeys[Frame] = FVector3f::ZeroVector;

			if (BoneName == ProxyRootBoneName)
			{
				RotKeys[Frame] = ProxyRootRotation;
			}
			else
			{
				RotKeys[Frame] = ZeroRotation;
			}

			ScaleKeys[Frame] = FixedScale;
		}

		Controller.AddBoneTrack(BoneName);
		Controller.SetBoneTrackKeys(BoneName, PosKeys, RotKeys, ScaleKeys);

		UE_LOG(LogTemp, Warning, TEXT("Fixed Bone : %s"), *BoneName.ToString());
	}

	Controller.CloseBracket();

	AnimationSequence->MarkPackageDirty();

	UE_LOG(LogTemp, Warning, TEXT("Fixed Root Bone Transform : %s Frames=%d"),
		*AnimationSequence->GetName(),
		NumFrames);
#endif
}
