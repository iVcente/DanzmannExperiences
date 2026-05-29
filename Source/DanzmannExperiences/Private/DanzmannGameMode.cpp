// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#include "DanzmannGameMode.h"

#include "Components/GameFrameworkComponentManager.h"
#include "DanzmannLogDanzmannExperiences.h"
#include "Engine/AssetManager.h"
#include "DanzmannGameState.h"
#include "DanzmannWorldSettings.h"
#include "Experiences/DanzmannExperienceDefinition.h"
#include "Experiences/DanzmannExperienceManagerComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Pawns/DanzmannPawnData.h"
#include "Pawns/DanzmannPawnDataComponent.h"

ADanzmannGameMode::ADanzmannGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = ADanzmannGameState::StaticClass();
}

void ADanzmannGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void ADanzmannGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	
	Super::EndPlay(EndPlayReason);
}

void ADanzmannGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	ExperienceId = ResolveExperienceId(Options);

	UE_LOG(LogDanzmannExperiences, Log, TEXT("[%hs] Experience \"%s\" activated."), __FUNCTION__, *ExperienceId.ToString());
}

void ADanzmannGameMode::InitGameState()
{
	Super::InitGameState();

	if (!ExperienceId.IsValid())
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] No Experience resolved -- neither by URL options or World Settings -- and activated. Pawn spawning will fall back to engine defaults."), __FUNCTION__);
		return;
	}

	const AGameStateBase* GameStateBase = GetGameState<AGameStateBase>();
	if (!IsValid(GameStateBase))
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] No Game State available."), __FUNCTION__);
		return;
	}

	UDanzmannExperienceManagerComponent* ExperienceManagerComponent = GameStateBase->FindComponentByClass<UDanzmannExperienceManagerComponent>();
	if (!IsValid(ExperienceManagerComponent))
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] Game State has no UDanzmannExperienceManagerComponent. Use ADanzmannGameState or add the Component to your Game State."), __FUNCTION__);
		return;
	}
	
	ExperienceManagerComponent->LoadExperience(ExperienceId);
}

UClass* ADanzmannGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const UDanzmannPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (IsValid(PawnData->PawnClass))
		{
			return PawnData->PawnClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* ADanzmannGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	const UDanzmannPawnData* PawnData = GetPawnDataForController(NewPlayer);
	UClass* PawnClass = IsValid(PawnData) && IsValid(PawnData->PawnClass) ? PawnData->PawnClass.Get() : Super::GetDefaultPawnClassForController_Implementation(NewPlayer);
	
	if (!IsValid(PawnClass))
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] No Pawn class resolved for Controller \"%s\"."), __FUNCTION__, *GetNameSafe(NewPlayer));
		return nullptr;
	}

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.Instigator = GetInstigator();
	ActorSpawnParameters.ObjectFlags |= RF_Transient; // We never want to save default player Pawns into a map
	ActorSpawnParameters.bDeferConstruction = true;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, ActorSpawnParameters);
	if (!IsValid(SpawnedPawn))
	{
		UE_LOG(LogDanzmannExperiences, Error, TEXT("[%hs] World->SpawnActor() returned null for Pawn class \"%s\"."), __FUNCTION__, *GetNameSafe(PawnClass));
		return nullptr;
	}

	// Set Pawn Data for chosen Pawn class
	if (IsValid(PawnData))
	{
		if (UDanzmannPawnDataComponent* PawnDataComponent = SpawnedPawn->FindComponentByClass<UDanzmannPawnDataComponent>(); IsValid(PawnDataComponent))
		{
			PawnDataComponent->SetPawnData(PawnData);
		}
	}

	UGameplayStatics::FinishSpawningActor(SpawnedPawn, SpawnTransform);

	return SpawnedPawn;
}

const UDanzmannPawnData* ADanzmannGameMode::GetPawnDataForController(const AController* Controller) const
{
	const AGameStateBase* GameStateBase = GetGameState<AGameStateBase>();
	if (!IsValid(GameStateBase))
	{
		return nullptr;
	}

	const UDanzmannExperienceManagerComponent* ExperienceManagerComponent = GameStateBase->FindComponentByClass<UDanzmannExperienceManagerComponent>();
	if (!IsValid(ExperienceManagerComponent) || !ExperienceManagerComponent->IsExperienceLoaded())
	{
		return nullptr;
	}

	const UDanzmannExperienceDefinition* Experience = ExperienceManagerComponent->GetLoadedExperience();
	if (!IsValid(Experience))
	{
		return nullptr;
	}

	// TODO(async-load): Preload DefaultPawnData via UDanzmannAssetManager (e.g. on Experience activation) and resolve here without blocking the spawn pipeline.
	return Experience->DefaultPawnData.LoadSynchronous();
}

FPrimaryAssetId ADanzmannGameMode::ResolveExperienceId(const FString& Options) const
{
	// Experience from URL options have the highest priority
	const FString UrlOptionsExperience = UGameplayStatics::ParseOption(Options, TEXT("Experience"));
	if (!UrlOptionsExperience.IsEmpty())
	{
		FPrimaryAssetId UrlExperienceId = FPrimaryAssetId::ParseTypeAndName(UrlOptionsExperience);
		if (!UrlExperienceId.PrimaryAssetType.IsValid())
		{
			UrlExperienceId = FPrimaryAssetId(UDanzmannExperienceDefinition::PrimaryAssetType, FName(*UrlOptionsExperience));
		}

		if (UrlExperienceId.IsValid())
		{
			return UrlExperienceId;
		}
	}

	// Try to fall back to default Experience set in World Settings
	if (const ADanzmannWorldSettings* WorldSettings = Cast<ADanzmannWorldSettings>(GetWorldSettings()))
	{
		if (const FSoftObjectPath WorldSettingsExperience = WorldSettings->DefaultExperience.ToSoftObjectPath(); WorldSettingsExperience.IsValid())
		{
			if (const FPrimaryAssetId WorldSettingsId = UAssetManager::Get().GetPrimaryAssetIdForPath(WorldSettingsExperience); WorldSettingsId.IsValid())
			{
				return WorldSettingsId;
			}
		}
	}

	return FPrimaryAssetId();
}
