// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#include "Experiences/DanzmannExperienceDefinition.h"

const FPrimaryAssetType UDanzmannExperienceDefinition::PrimaryAssetType(TEXT("ExperienceDefinition"));

FPrimaryAssetId UDanzmannExperienceDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}
