// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "CoreMinimal.h"
#include "Experiences/DanzmannExperienceDefinition.h"

#include "DanzmannExperienceManagerComponent.generated.h"

/**
 * Delegate to notify Experience has loaded.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDanzmannOnExperienceLoaded, const UDanzmannExperienceDefinition* Experience);

/**
 * Game State Component responsible for managing Experiences -- loading, adding/removing Actions.
 */
UCLASS(ClassGroup = "Danzmann", HideCategories = ("Activation", "AssetUserData", "ComponentTick", "ComponentReplication", "Cooking", "Navigation", "Replication"), Meta = (BlueprintSpawnableComponent))
class DANZMANNEXPERIENCES_API UDanzmannExperienceManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

	public:
		UDanzmannExperienceManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

		/**
		 * @see more info in Object.h.
		 */
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

		/**
		 * @see more info in ActorComponent.h.
		 */
		virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

		/**
		 * Get the loaded active Experience.
		 * @return Loaded Experience.
		 */
		const UDanzmannExperienceDefinition* GetLoadedExperience() const
		{
			return LoadedExperience;
		}

		/**
		 * Resolve a Primary Asset ID to a loaded UDanzmannExperienceDefinition,
		 * store it as the loaded active Experience, and broadcast the three priority delegates in order -- server-only.
		 * OnRep_LoadedExperience() is triggered on clients which run the same broadcast.
		 * @param ExperienceId Primary Asset ID of an UDanzmannExperienceDefinition.
		 */
		void LoadExperience(const FPrimaryAssetId& ExperienceId);

		/**
		 * Get whether Experience is loaded or not.
		 * @return Whether Experience is loaded or not.
		 */
		bool IsExperienceLoaded() const
		{
			return IsValid(LoadedExperience) && bExperienceLoaded;
		}

		/**
		 * Experience-loaded notification entry points (High -> Medium -> Low broadcast order).
		 *
		 * Contract:
		 * - One-shot: each priority delegate is Broadcast() then Clear()-ed inside NotifyExperienceLoaded().
		 *   A consumer that registers AFTER the broadcast fires immediately via the bExperienceLoaded
		 *   fast-path below. There is no second event to listen for in a session.
		 * - Fires BEFORE ActivateActions(). Listeners see the LoadedExperience but cannot assume that
		 *   anything mutated by Experience Actions (e.g., Components attached by
		 *   UDanzmannExperienceAction_AddComponents) is present yet. Defer that work to the next
		 *   tick, a ModularGameplay init-state callback, or your own readiness signal.
		 * - Rvalue-only: delegates are moved in, not copied. Construct your FDelegate inline or wrap
		 *   in MoveTemp() at the call site.
		 */

		/**
		 * Experience loaded notification for High priority targets.
		 * In case Experience has already been loaded, trigger callback right away.
		 * Otherwise, wait until Experience finishes loading.
		 * @param Delegate Callback to notify loading completed.
		 * @note High priority fires first, then Medium, then Low.
		 */
		void OnExperienceLoaded_HighPriority(FDanzmannOnExperienceLoaded::FDelegate&& Delegate);

		/**
		 * Experience loaded notification for Medium priority targets.
		 * In case Experience has already been loaded, trigger callback right away.
		 * Otherwise, wait until Experience finishes loading.
		 * @param Delegate Callback to notify loading completed.
		 * @note High priority fires first, then Medium, then Low.
		 */
		void OnExperienceLoaded_MediumPriority(FDanzmannOnExperienceLoaded::FDelegate&& Delegate);

		/**
		 * Experience loaded notification for Low priority targets.
		 * In case Experience has already been loaded, trigger callback right away.
		 * Otherwise, wait until Experience finishes loading.
		 * @param Delegate Callback to notify loading completed.
		 * @note High priority fires first, then Medium, then Low.
		 */
		void OnExperienceLoaded_LowPriority(FDanzmannOnExperienceLoaded::FDelegate&& Delegate);

	private:
		/**
		 * Run the Experience's Actions list in forward order. Called from BroadcastLoaded() after the priority
		 * delegates fire, so consumers of OnExperienceLoaded see the experience before actions mutate state.
		 */
		void ActivateActions();

		/**
		 * Reverse-iterate the Experience's Actions list and tear them down. Called from EndPlay() so Component
		 * requests, splitscreen votes, and so on unwind together with the Game State's world.
		 */
		void DeactivateActions();

		/**
		 * Reference to loaded Experience.
		 */
		UPROPERTY(ReplicatedUsing = OnRep_LoadedExperience)
		TObjectPtr<const UDanzmannExperienceDefinition> LoadedExperience = nullptr;

		/**
		 * OnRep function for LoadedExperience.
		 */
		UFUNCTION()
		void OnRep_LoadedExperience();

		/**
		 * Notify Experience has been loaded.
		 */
		void NotifyExperienceLoaded();

		/**
		 * Whether Experience has been loaded or not already.
		 */
		bool bExperienceLoaded = false;

		/**
		 * Whether Experience's Action have been activated or not already.
		 */
		bool bActionsActivated = false;

		/**
		 * Delegate to notify high priority targets Experience has been loaded.
		 */
		FDanzmannOnExperienceLoaded OnExperienceLoaded_HighPriorityDelegate;
		
		/**
		 * Delegate to notify medium priority targets Experience has been loaded.
		 */
		FDanzmannOnExperienceLoaded OnExperienceLoaded_MediumPriorityDelegate;
		
		/**
		 * Delegate to notify low priority targets Experience has been loaded.
		 */
		FDanzmannOnExperienceLoaded OnExperienceLoaded_LowPriorityDelegate;
};
