// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

using UnrealBuildTool;

public class DanzmannExperiences : ModuleRules
{
	public DanzmannExperiences(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DanzmannAbilities",
				"DanzmannInput",
				"DanzmannMovement",
				"Engine",
				"GameplayAbilities",
				"GameplayCameras",
				"GameplayTags",
				"ModularGameplay"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"NetCore"
			}
		);
	}
}
