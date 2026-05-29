// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "DanzmannExperienceAction.generated.h"

class UDanzmannExperienceManagerComponent;

/**
 * A lighter version of UGameFeatureAction. A base class for instanced Experience Action stored 
 * on a UDanzmannExperienceDefinition. 
 * Subclasses do work when the Experience activates (e.g., register modular Components, force-disable splitscreen)
 * and undo it when the Experience deactivates. 
 * UDanzmannExperienceManagerComponent calls these hooks once each per session: 
 * - Activation after BroadcastLoaded() fires the priority-tier delegates
 * - Deactivation on EndPlay().
 */
UCLASS(Abstract, BlueprintType, CollapseCategories, EditInlineNew, DefaultToInstanced, MinimalAPI)
class UDanzmannExperienceAction : public UObject
{
	GENERATED_BODY()

	public:
		/**
		 * Invoked when the owning Experience becomes active. 
		 * Server runs this after the synchronous load.
		 * Each remote client runs it after OnRep_LoadedExperience().
		 */
		virtual void OnExperienceActivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent)
		{
		}

		/**
		 * Invoked when the manager tears down -- world's EndPlay(). Actions undo whatever
		 * OnExperienceActivated() registered (e.g., release Component request handles).
		 */
		virtual void OnExperienceDeactivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent)
		{
		}
};
