#include "AnimNotify_SendGameplayEventToOwner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_SendGameplayEventToOwner::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	if (!EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Owner;
	Payload.Target = Owner;
	Payload.OptionalObject = Animation;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
}

FString UAnimNotify_SendGameplayEventToOwner::GetNotifyName_Implementation() const
{
	return EventTag.IsValid() ? FString::Printf(TEXT("GE: %s"), *EventTag.ToString()) : TEXT("Send Gameplay Event");
}
