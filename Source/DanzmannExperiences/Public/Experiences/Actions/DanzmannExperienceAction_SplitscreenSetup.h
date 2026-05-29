// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Experiences/Actions/DanzmannExperienceAction.h"

#include "DanzmannExperienceAction_SplitscreenSetup.generated.h"

/**
 * Experience Action that force-disables splitscreen on the active game viewport while the Experience
 * is active. Uses a static vote counter so overlapping experiences (PIE re-entry, future swap support)
 * unwind cleanly: only the 0->1 transition disables splitscreen, only the 1->0 transition re-enables it.
 */
UCLASS(MinimalAPI, DisplayName = "Splitscreen Config")
class UDanzmannExperienceAction_SplitscreenSetup final : public UDanzmannExperienceAction
{
	GENERATED_BODY()

	public:
		/**
		 * @see more info in DanzmannExperienceAction.h.
		 */
		virtual void OnExperienceActivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent) override;
		
		/**
		 * @see more info in DanzmannExperienceAction.h.
		 */
		virtual void OnExperienceDeactivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent) override;

	private:
		/**
		 * Whether splitscreen should be disabled.
		 */
		UPROPERTY(EditAnywhere)
		bool bDisableSplitscreen = true;

		/**
		 * Managers that successfully cast a vote through this Action. Tracking per-manager (rather than
		 * a single bHasVoted bool) is what makes the vote correct in multi-client PIE: the server and
		 * each client world own distinct UDanzmannExperienceManagerComponent instances but share THIS
		 * Action instance via the loaded UDanzmannExperienceDefinition, so per-instance bookkeeping
		 * would let the second activate find bHasVoted already-true and skip the increment, then leave
		 * the static counter stuck on after the last world tears down.
		 *
		 * Weak pointers so a GC'd manager doesn't dangle. RemoveSingle by pointer-identity still works.
		 */
		TArray<TWeakObjectPtr<UDanzmannExperienceManagerComponent>> VotingManagers;

		/**
		 * Shared across all instances; mirrors Lyra's pattern so multiple active Experiences cooperate.
		 */
		static int32 DisableSplitscreenVoteCount;
};
