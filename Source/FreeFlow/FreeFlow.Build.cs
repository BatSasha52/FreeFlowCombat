// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FreeFlow : ModuleRules
{
	public FreeFlow(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
