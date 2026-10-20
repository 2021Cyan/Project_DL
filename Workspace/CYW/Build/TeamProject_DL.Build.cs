// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TeamProject_DL : ModuleRules
{
	public TeamProject_DL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"Niagara",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"DeveloperSettings",
			"AIModule",
			"NavigationSystem",
            "UMG",
            "Slate",
            "SlateCore",
            "UMG",
            "MoviePlayer",
            "DeveloperSettings"
        });

		PrivateDependencyModuleNames.AddRange(new string[] { "GameplayAbilities", "HTTP", "Json" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

		PrivateIncludePaths.AddRange(new string[]
{
	"TeamProject_DL"
});
	}
}
