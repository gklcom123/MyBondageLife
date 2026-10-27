// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyBondageLife : ModuleRules
{
	public MyBondageLife(ReadOnlyTargetRules Target) : base(Target)
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
			"UMG",
			"Slate",
			"SlateCore",
			"Sockets",
			"HTTP",
			"Json",
			"EOSSDK",
			"EOSShared",
			"SocketSubsystemEOS",
			"OnlineSubsystemUtils"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MyBondageLife",
			"MyBondageLife/Variant_Platforming",
			"MyBondageLife/Variant_Platforming/Animation",
			"MyBondageLife/Variant_Combat",
			"MyBondageLife/Variant_Combat/AI",
			"MyBondageLife/Variant_Combat/Animation",
			"MyBondageLife/Variant_Combat/Gameplay",
			"MyBondageLife/Variant_Combat/Interfaces",
			"MyBondageLife/Variant_Combat/UI",
			"MyBondageLife/Variant_SideScrolling",
			"MyBondageLife/Variant_SideScrolling/AI",
			"MyBondageLife/Variant_SideScrolling/Gameplay",
			"MyBondageLife/Variant_SideScrolling/Interfaces",
			"MyBondageLife/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
