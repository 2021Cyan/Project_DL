#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_SendGameplayEventToOwner.generated.h"

UCLASS(meta = (DisplayName = "Send Gameplay Event To Owner"))
class TEAMPROJECT_DL_API UAnimNotify_SendGameplayEventToOwner : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay Event")
	FGameplayTag EventTag;
};
