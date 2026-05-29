// Copyright (C) 2025 Vicente Danzmann. All Rights Reserved.

using UnrealBuildTool;

public class DanzmannPlugin : ModuleRules
{
	public DanzmannPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine"
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject"
			}
		);
	}
}
