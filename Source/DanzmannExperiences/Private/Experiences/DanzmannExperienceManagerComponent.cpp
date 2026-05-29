// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#include "Experiences/DanzmannExperienceManagerComponent.h"

#include "DanzmannLogDanzmannExperiences.h"
#include "Engine/AssetManager.h"
#include "Experiences/Actions/DanzmannExperienceAction.h"
#include "Experiences/DanzmannExperienceDefinition.h"
#include "Net/UnrealNetwork.h"

UDanzmannExperienceManagerComponent::UDanzmannExperienceManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	SetIsReplicatedByDefault(true);
}

void UDanzmannExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	constexpr FDoRepLifetimeParams Params;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LoadedExperience, Params)
}

void UDanzmannExperienceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateActions();
	
	Super::EndPlay(EndPlayReason);
}

void UDanzmannExperienceManagerComponent::LoadExperience(const FPrimaryAssetId& ExperienceId)
{
	if (const AActor* Owner = GetOwner(); !IsValid(Owner) || !Owner->HasAuthority())
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] Ignoring call on non-authority."), __FUNCTION__);
		return;
	}

	if (!ensureMsgf(!bExperienceLoaded, TEXT("Trying to load an Experience when there's one already loaded.")))
	{
		return;
	}

	if (!ExperienceId.IsValid())
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] Invalid Experience ID."), __FUNCTION__);
		return;
	}

	const FSoftObjectPath ExperiencePath = UAssetManager::Get().GetPrimaryAssetPath(ExperienceId);
	if (!ExperiencePath.IsValid())
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] Could not resolve Experience ID \"%s\" to an asset path. Check PrimaryAssetTypesToScan in DefaultGame.ini."), __FUNCTION__, *ExperienceId.ToString());
		return;
	}
	
	// TODO(async-load): The priority-tier delegates and CallOrRegister entry points exist so this can become async without touching consumers. Drop in FStreamableManager::RequestAsyncLoad and broadcast from the completion callback.
	const UDanzmannExperienceDefinition* Experience = Cast<UDanzmannExperienceDefinition>(ExperiencePath.TryLoad());
	if (!IsValid(Experience))
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] Failed to load Experience \"%s\"."), __FUNCTION__, *ExperienceId.ToString());
		return;
	}

	LoadedExperience = Experience;
	NotifyExperienceLoaded();
	UE_LOG(LogDanzmannExperiences, Log, TEXT("[%hs] Experience \"%s\" loaded."), __FUNCTION__, *ExperienceId.ToString());
}

void UDanzmannExperienceManagerComponent::OnRep_LoadedExperience()
{
	NotifyExperienceLoaded();
}

void UDanzmannExperienceManagerComponent::NotifyExperienceLoaded()
{
	if (!IsValid(LoadedExperience) || bExperienceLoaded)
	{
		return;
	}

	bExperienceLoaded = true;

	OnExperienceLoaded_HighPriorityDelegate.Broadcast(LoadedExperience);
	OnExperienceLoaded_HighPriorityDelegate.Clear();

	OnExperienceLoaded_MediumPriorityDelegate.Broadcast(LoadedExperience);
	OnExperienceLoaded_MediumPriorityDelegate.Clear();

	OnExperienceLoaded_LowPriorityDelegate.Broadcast(LoadedExperience);
	OnExperienceLoaded_LowPriorityDelegate.Clear();

	ActivateActions();
}

void UDanzmannExperienceManagerComponent::ActivateActions()
{
	if (bActionsActivated || !IsValid(LoadedExperience))
	{
		return;
	}

	for (UDanzmannExperienceAction* Action : LoadedExperience->Actions)
	{
		if (IsValid(Action))
		{
			Action->OnExperienceActivated(this);
		}
	}

	bActionsActivated = true;
}

void UDanzmannExperienceManagerComponent::DeactivateActions()
{
	if (!bActionsActivated || !IsValid(LoadedExperience))
	{
		return;
	}

	for (int32 Index = LoadedExperience->Actions.Num() - 1; Index >= 0; --Index)
	{
		if (UDanzmannExperienceAction* Action = LoadedExperience->Actions[Index]; IsValid(Action))
		{
			Action->OnExperienceDeactivated(this);
		}
	}

	bActionsActivated = false;
}

void UDanzmannExperienceManagerComponent::OnExperienceLoaded_HighPriority(FDanzmannOnExperienceLoaded::FDelegate&& Delegate)
{
	if (bExperienceLoaded)
	{
		Delegate.Execute(LoadedExperience);
	}
	else
	{
		OnExperienceLoaded_HighPriorityDelegate.Add(MoveTemp(Delegate));
	}
}

void UDanzmannExperienceManagerComponent::OnExperienceLoaded_MediumPriority(FDanzmannOnExperienceLoaded::FDelegate&& Delegate)
{
	if (bExperienceLoaded)
	{
		Delegate.Execute(LoadedExperience);
	}
	else
	{
		OnExperienceLoaded_MediumPriorityDelegate.Add(MoveTemp(Delegate));
	}
}

void UDanzmannExperienceManagerComponent::OnExperienceLoaded_LowPriority(FDanzmannOnExperienceLoaded::FDelegate&& Delegate)
{
	if (bExperienceLoaded)
	{
		Delegate.Execute(LoadedExperience);
	}
	else
	{
		OnExperienceLoaded_LowPriorityDelegate.Add(MoveTemp(Delegate));
	}
}
