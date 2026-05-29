// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "DanzmannAbilitySystemComponentOwner.generated.h"

/**
 * Owner type -- Pawn or Player State -- for an Ability System Component.
 */
UENUM(BlueprintType)
enum class EDanzmannAbilitySystemComponentOwner : uint8
{
	Pawn,
	PlayerState
};
