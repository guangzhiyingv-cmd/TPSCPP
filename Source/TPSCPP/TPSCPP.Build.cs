// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPSCPP : ModuleRules
{
	public TPSCPP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"OnlineSubsystem",
			"OnlineSubsystemSteam",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TPSCPP",
			"TPSCPP/Variant_Platforming",
			"TPSCPP/Variant_Platforming/Animation",
			"TPSCPP/Variant_Combat",
			"TPSCPP/Variant_Combat/AI",
			"TPSCPP/Variant_Combat/Animation",
			"TPSCPP/Variant_Combat/Gameplay",
			"TPSCPP/Variant_Combat/Interfaces",
			"TPSCPP/Variant_Combat/UI",
			"TPSCPP/Variant_SideScrolling",
			"TPSCPP/Variant_SideScrolling/AI",
			"TPSCPP/Variant_SideScrolling/Gameplay",
			"TPSCPP/Variant_SideScrolling/Interfaces",
			"TPSCPP/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
