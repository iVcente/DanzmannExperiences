// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#include "Experiences/Actions/DanzmannExperienceAction_AddComponents.h"

#include "DanzmannLogDanzmannExperiences.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Experiences/DanzmannExperienceManagerComponent.h"

#pragma region DanzmannExperienceAddComponentEntry

	void FDanzmannExperienceAddComponentEntry::PostSerialize(const FArchive& Ar)
	{
		#if WITH_EDITORONLY_DATA
		if (IsValid(ComponentClass) && !ActorClass.IsNull())
		{
			FString ComponentClassName = ComponentClass->GetName();
			ComponentClassName.RemoveFromEnd(TEXT("_C"));
			FString ActorClassName = ActorClass->GetName();
			ActorClassName.RemoveFromEnd(TEXT("_C"));
			EditorDisplayName = FString::Printf(TEXT("%s -> %s"), *ComponentClassName, *ActorClassName);	
		}
		#endif
	}

#pragma endregion DanzmannExperienceAddComponentEntry

void UDanzmannExperienceAction_AddComponents::OnExperienceActivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent)
{
	if (!IsValid(ExperienceManagerComponent))
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

	UGameFrameworkComponentManager* GameFrameworkComponentManager = GameInstance->GetSubsystem<UGameFrameworkComponentManager>();
	if (!IsValid(GameFrameworkComponentManager))
	{
		UE_LOG(LogDanzmannExperiences, Warning, TEXT("[%hs] UGameFrameworkComponentManager invalid. AddComponents action no-op."), __FUNCTION__);
		return;
	}

	const ENetMode NetMode = World->GetNetMode();
	const bool bIsServer = NetMode != NM_Client;
	const bool bIsClient = NetMode != NM_DedicatedServer;

	for (const FDanzmannExperienceAddComponentEntry& Entry : ComponentsToAdd)
	{
		const bool bShouldAdd = (bIsServer && Entry.bServerComponent) || (bIsClient && Entry.bClientComponent);
		if (!bShouldAdd)
		{
			continue;
		}

		if (!IsValid(Entry.ComponentClass) || Entry.ActorClass.IsNull())
		{
			continue;
		}

		ComponentRequests.Add(GameFrameworkComponentManager->AddComponentRequest(Entry.ActorClass, Entry.ComponentClass));
	}
}

void UDanzmannExperienceAction_AddComponents::OnExperienceDeactivated(UDanzmannExperienceManagerComponent* /*ExperienceManagerComponent*/)
{
	// Dropping the request handles releases the Component requests -- no manager lookup needed.
	ComponentRequests.Reset();
}
