#pragma once

#include "GameplayTagContainer.h"

namespace DLAbilityTags
{
	// Config/DefaultGameplayTags.ini remains the registration source. Optional tags
	// retain their existing invalid-tag fallback; required setups validate before commit.
	inline FGameplayTag Find(FName Name) { return FGameplayTag::RequestGameplayTag(Name, false); }
	inline FGameplayTag SkillEvent() { static const FGameplayTag Tag = Find(TEXT("Event.Skill")); return Tag; }
	inline FGameplayTag SkillExecute() { static const FGameplayTag Tag = Find(TEXT("Event.Skill.Execute")); return Tag; }
	inline FGameplayTag Duration() { static const FGameplayTag Tag = Find(TEXT("Data.Duration")); return Tag; }
	inline FGameplayTag ReloadComplete() { static const FGameplayTag Tag = Find(TEXT("Event.Reload.Complete")); return Tag; }
	inline FGameplayTag ZiplineMounted() { static const FGameplayTag Tag = Find(TEXT("Event.Zipline.Mounted")); return Tag; }
}
