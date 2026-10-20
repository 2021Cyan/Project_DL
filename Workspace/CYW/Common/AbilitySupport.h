#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

namespace DLAbilitySupport
{
	// Ability levels are zero-based tiers in this project.
	template <typename T>
	const T& ResolveLevelData(const TArray<T>& Levels, int32 Tier)
	{
		static const T Fallback{};
		return Levels.IsEmpty() ? Fallback : Levels[FMath::Clamp(Tier, 0, Levels.Num() - 1)];
	}

	inline FGameplayEffectSpecHandle MakeEffectSpec(UAbilitySystemComponent* ASC,
		TSubclassOf<UGameplayEffect> EffectClass, float Level, AActor* SourceActor, const UObject* SourceObject)
	{
		if (!ASC || !EffectClass || !IsValid(SourceActor)) return {};
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(SourceObject);
		Context.AddInstigator(SourceActor, SourceActor);
		return ASC->MakeOutgoingSpec(EffectClass, Level, Context);
	}

	inline FGameplayCueParameters MakeCueParameters(
		const FVector& Location, const FVector& Normal, AActor* Instigator,
		AActor* EffectCauser, const UObject* SourceObject = nullptr)
	{
		FGameplayCueParameters Params;
		Params.Location = Location;
		Params.Normal = Normal;
		Params.Instigator = Instigator;
		Params.EffectCauser = EffectCauser;
		Params.SourceObject = SourceObject;
		return Params;
	}
}
