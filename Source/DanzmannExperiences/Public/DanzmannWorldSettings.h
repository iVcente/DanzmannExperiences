// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"

#include "DanzmannWorldSettings.generated.h"

class UDanzmannExperienceDefinition;

/**
 * World Settings subclass letting each map declare which UDanzmannExperienceDefinition to activate.
 * Resolved by ADanzmannGameMode::InitGame() after the ?Experience= URL option.
 */
UCLASS()
class DANZMANNEXPERIENCES_API ADanzmannWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

	public:
		/**
		 * Experience activated for this map -- if no ?Experience= URL option is supplied.
		 */
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode")
		TSoftObjectPtr<UDanzmannExperienceDefinition> DefaultExperience = nullptr;
};
