// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimationResetModifier.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"

void UAnimationResetModifier::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
#if WITH_EDITOR
	if (!AnimationSequence)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AnimationResetModifier] AnimationSequence is null.")
		);

		return;
	}

	const IAnimationDataModel* DataModel = AnimationSequence->GetDataModel();

	if (!DataModel)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AnimationResetModifier] DataModel is null: %s"),
			*AnimationSequence->GetName()
		);

		return;
	}

	TArray<FName> BoneTrackNames;
	DataModel->GetBoneTrackNames(BoneTrackNames);

	if (BoneTrackNames.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AnimationResetModifier] No bone tracks: %s"),
			*AnimationSequence->GetName()
		);

		return;
	}

	if (!BoneTrackNames.Contains(RootBoneName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[AnimationResetModifier] Root bone track not found. "
				"Animation=%s RootBone=%s"
			),
			*AnimationSequence->GetName(),
			*RootBoneName.ToString()
		);
	}

	IAnimationDataController& Controller = AnimationSequence->GetController();

	AnimationSequence->Modify();

	Controller.OpenBracket(
		FText::FromString(TEXT("Reset Animation Bone Transform"))
	);

	const FVector3f UnitScale(1.0f, 1.0f, 1.0f);
	const FQuat4f IdentityRotation = FQuat4f::Identity;

	int32 ModifiedTrackCount = 0;

	for (const FName& BoneName : BoneTrackNames)
	{
		TArray<FTransform> BoneTransforms;
		DataModel->GetBoneTrackTransforms(
			BoneName,
			BoneTransforms
		);

		if (BoneTransforms.IsEmpty())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT(
					"[AnimationResetModifier] No transform keys: %s"
				),
				*BoneName.ToString()
			);

			continue;
		}

		const bool bIsRootBone = BoneName == RootBoneName;
		const int32 KeyCount = BoneTransforms.Num();

		TArray<FVector3f> PositionKeys;
		TArray<FQuat4f> RotationKeys;
		TArray<FVector3f> ScaleKeys;

		PositionKeys.Reserve(KeyCount);
		RotationKeys.Reserve(KeyCount);
		ScaleKeys.Reserve(KeyCount);

		for (const FTransform& BoneTransform : BoneTransforms)
		{
			// 모든 본의 위치 값은 기존 애니메이션 값을 유지합니다.
			PositionKeys.Add(
				FVector3f(BoneTransform.GetTranslation())
			);

			if (bIsRootBone)
			{
				// root 본의 Rotation을 0, 0, 0으로 고정합니다.
				RotationKeys.Add(IdentityRotation);

				// root 본의 Scale을 0.00254로 고정합니다.
				ScaleKeys.Add(RootScale);
			}
			else
			{
				// 나머지 본은 기존 Rotation을 유지합니다.
				FQuat ExistingRotation = BoneTransform.GetRotation();
				ExistingRotation.Normalize();

				RotationKeys.Add(
					FQuat4f(ExistingRotation)
				);

				// 나머지 본의 Scale을 1, 1, 1로 고정합니다.
				ScaleKeys.Add(UnitScale);
			}
		}

		Controller.SetBoneTrackKeys(
			BoneName,
			PositionKeys,
			RotationKeys,
			ScaleKeys
		);

		++ModifiedTrackCount;

		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"[AnimationResetModifier] Fixed Bone: %s "
				"Root=%s Keys=%d"
			),
			*BoneName.ToString(),
			bIsRootBone ? TEXT("true") : TEXT("false"),
			KeyCount
		);
	}

	Controller.CloseBracket();

	AnimationSequence->MarkPackageDirty();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[AnimationResetModifier] Complete: %s "
			"ModifiedTracks=%d RootBone=%s RootScale=(%f, %f, %f)"
		),
		*AnimationSequence->GetName(),
		ModifiedTrackCount,
		*RootBoneName.ToString(),
		RootScale.X,
		RootScale.Y,
		RootScale.Z
	);
#endif
}
