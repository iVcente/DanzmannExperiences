// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#include "Experiences/Actions/DanzmannExperienceAction_SplitscreenSetup.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Experiences/DanzmannExperienceManagerComponent.h"

int32 UDanzmannExperienceAction_SplitscreenSetup::DisableSplitscreenVoteCount = 0;

void UDanzmannExperienceAction_SplitscreenSetup::OnExperienceActivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent)
{
	if (!bDisableSplitscreen || !IsValid(ExperienceManagerComponent))
	{
		return;
	}

	// Reject re-activate from the same manager so the per-world vote stays one-per-manager even if
	// the manager somehow drives ActivateActions() twice.
	if (VotingManagers.Contains(ExperienceManagerComponent))
	{
		return;
	}

	const UWorld* World = ExperienceManagerComponent->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	UGameViewportClient* ViewportClient = GameInstance->GetGameViewportClient();
	if (!IsValid(ViewportClient))
	{
		return;
	}

	VotingManagers.Add(ExperienceManagerComponent);
	++DisableSplitscreenVoteCount;

	if (DisableSplitscreenVoteCount == 1)
	{
		ViewportClient->SetForceDisableSplitscreen(true);
	}
}

void UDanzmannExperienceAction_SplitscreenSetup::OnExperienceDeactivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent)
{
	// Only decrement if THIS manager actually voted. RemoveSingle returns 0 when the entry wasn't
	// present -- covers the case where activate early-outed (no viewport, disabled, etc.).
	if (VotingManagers.RemoveSingle(ExperienceManagerComponent) == 0)
	{
		return;
	}

	--DisableSplitscreenVoteCount;

	if (DisableSplitscreenVoteCount > 0 || !IsValid(ExperienceManagerComponent))
	{
		return;
	}

	const UWorld* World = ExperienceManagerComponent->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	UGameViewportClient* ViewportClient = GameInstance->GetGameViewportClient();
	if (!IsValid(ViewportClient))
	{
		return;
	}

	ViewportClient->SetForceDisableSplitscreen(false);
}
