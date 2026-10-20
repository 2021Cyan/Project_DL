// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/GA_Doorway.h"
#include "Common/AbilityTags.h"

#include "Doorman/Doorway/DoorwayActor.h"
#include "AbilitySystemComponent.h"
#include "Common/AbilitySupport.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

UGA_Doorway::UGA_Doorway()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	FGameplayTagContainer AssetTags;
	const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Ability.Doorman.Doorway")), false);
	if (AbilityTag.IsValid())
	{
		AssetTags.AddTag(AbilityTag);
	}
	SetAssetTags(AssetTags);

	DoorwayActorClass = ADoorwayActor::StaticClass();
	TargetingMode = ESkillTargetingMode::Preview;
	bRequiresSkillEvent = true;
	bEndAbilityWhenCastMontageEnds = true;
	bEndAbilityWhenCastMontageCancelled = true;
	SkillEventParentTag = DLAbilityTags::SkillEvent();
	ExecuteEventTag = DLAbilityTags::SkillExecute();
	SpawnGameplayCueTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("GameplayCue.Skill.Doorway.Spawn")), false);
	LevelData.AddDefaulted();
}

void UGA_Doorway::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bHasExecuted = false;

	// Recasting with a connected pair removes both doors instead of starting another placement.
	// Handle this on the server before cost, targeting preview, or actor spawning begins.
	if (HasAuthority(&ActivationInfo)
		&& IsValid(EntranceDoorway)
		&& IsValid(ExitDoorway)
		&& EntranceDoorway->GetLinkedDoorway() == ExitDoorway
		&& ExitDoorway->GetLinkedDoorway() == EntranceDoorway)
	{
		ClearDoorwayPair();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	PlacementStep = IsValid(EntranceDoorway) && !IsValid(ExitDoorway)
		? EDoorwayPlacementStep::Exit
		: EDoorwayPlacementStep::Entrance;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Doorway::OnSkillEvent_Implementation(FGameplayTag EventTag, FGameplayEventData Payload)
{
	Super::OnSkillEvent_Implementation(EventTag, Payload);

	if (!EventTag.MatchesTagExact(ExecuteEventTag) || bHasExecuted)
	{
		return;
	}

	bHasExecuted = true;
	ExecuteDoorway();
}

void UGA_Doorway::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled && PlacementStep == EDoorwayPlacementStep::Exit && IsValid(EntranceDoorway) && !IsValid(ExitDoorway))
	{
		ClearDoorwayPair();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Doorway::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (PlacementStep == EDoorwayPlacementStep::Entrance)
	{
		return;
	}

	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
}

void UGA_Doorway::ExecuteDoorway()
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (PlacementStep == EDoorwayPlacementStep::Exit)
	{
		SpawnExit();
	}
	else
	{
		SpawnEntrance();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Doorway::SpawnEntrance()
{
	ClearDoorwayPair();

	EntranceDoorway = SpawnDoorwayActor(CachedPreviewTransform, true);
	if (EntranceDoorway)
	{
		ClearDoorwayTimers();
	}
}

void UGA_Doorway::SpawnExit()
{
	if (!IsValid(EntranceDoorway))
	{
		SpawnEntrance();
		return;
	}

	if (IsValid(ExitDoorway))
	{
		ExitDoorway->Destroy();
		ExitDoorway = nullptr;
	}

	ExitDoorway = SpawnDoorwayActor(CachedPreviewTransform, false);
	if (!ExitDoorway)
	{
		return;
	}

	EntranceDoorway->SetLinkedDoorway(ExitDoorway);
	ExitDoorway->SetLinkedDoorway(EntranceDoorway);
	EntranceDoorway->PlayConnectedMontage();
	ExitDoorway->PlayConnectedMontage();
	ScheduleDoorwayExpiration();
}

ADoorwayActor* UGA_Doorway::SpawnDoorwayActor(const FTransform& SpawnTransform, bool bIsEntrance) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UWorld* World = AvatarActor ? AvatarActor->GetWorld() : nullptr;
	if (!World || !DoorwayActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = SpawnCollisionHandlingMethod;

	const FTransform AdjustedTransform = BuildDoorwaySpawnTransform(SpawnTransform);
	ADoorwayActor* DoorwayActor = World->SpawnActor<ADoorwayActor>(
		DoorwayActorClass,
		AdjustedTransform,
		SpawnParams);

	if (DoorwayActor)
	{
		DoorwayActor->InitializeDoorwayEndpoint(
			bIsEntrance ? EDoorwayEndpointType::Entrance : EDoorwayEndpointType::Exit,
			nullptr);
		ExecuteSpawnGameplayCue(DoorwayActor);
	}

	return DoorwayActor;
}

FTransform UGA_Doorway::BuildDoorwaySpawnTransform(const FTransform& PreviewTransform) const
{
	FTransform SpawnTransform = PreviewTransform;
	if (WallClearanceOffset > 0.f)
	{
		SpawnTransform.AddToTranslation(PreviewTransform.GetUnitAxis(EAxis::Y) * WallClearanceOffset);
	}
	return SpawnTransform;
}

void UGA_Doorway::ExecuteSpawnGameplayCue(ADoorwayActor* SpawnedDoorway) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SpawnGameplayCueTag.IsValid() || !AvatarActor || !SourceASC || !SpawnedDoorway)
	{
		return;
	}

	FGameplayCueParameters CueParams = DLAbilitySupport::MakeCueParameters(
		SpawnedDoorway->GetActorLocation(), SpawnedDoorway->GetPortalForwardVector(), AvatarActor, SpawnedDoorway, this);
	SourceASC->ExecuteGameplayCue(SpawnGameplayCueTag, CueParams);
}

void UGA_Doorway::ClearDoorwayPair()
{
	ClearDoorwayTimers();

	if (IsValid(EntranceDoorway))
	{
		EntranceDoorway->Destroy();
	}
	if (IsValid(ExitDoorway))
	{
		ExitDoorway->Destroy();
	}

	EntranceDoorway = nullptr;
	ExitDoorway = nullptr;
}

void UGA_Doorway::ClearDoorwayTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DoorwayLifetimeHandle);
	}
}

void UGA_Doorway::ScheduleDoorwayExpiration()
{
	ClearDoorwayTimers();

	const float DoorLifetime = GetCurrentLevelData().DoorLifetime;
	if (DoorLifetime <= 0.f)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DoorwayLifetimeHandle,
			this,
			&ThisClass::OnDoorwayExpired,
			DoorLifetime,
			false);
	}
}

void UGA_Doorway::OnDoorwayExpired()
{
	ClearDoorwayPair();
}

float UGA_Doorway::GetEffectiveCooldownDuration() const
{
	return GetCurrentLevelData().CooldownDuration;
}

const FDoorwayLevelData& UGA_Doorway::GetCurrentLevelData() const
{
	return DLAbilitySupport::ResolveLevelData(LevelData, GetAbilityLevel());
}


void UGA_Doorway::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGA_Doorway, EntranceDoorway);
	DOREPLIFETIME(UGA_Doorway, ExitDoorway);
}

void UGA_Doorway::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (ActorInfo && ActorInfo->IsNetAuthority()) ClearDoorwayPair();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

bool UGA_Doorway::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return ActorInfo && ActorInfo->AvatarActor.IsValid() && (DoorwayActorClass && ExecuteEventTag.IsValid())
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}
