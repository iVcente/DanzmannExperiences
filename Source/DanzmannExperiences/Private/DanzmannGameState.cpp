// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#include "DanzmannGameState.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Experiences/DanzmannExperienceManagerComponent.h"

ADanzmannGameState::ADanzmannGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ExperienceManagerComponent = CreateDefaultSubobject<UDanzmannExperienceManagerComponent>(FName("DanzmannExperienceManagerComponent"));
}

void ADanzmannGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void ADanzmannGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	
	Super::EndPlay(EndPlayReason);
}
