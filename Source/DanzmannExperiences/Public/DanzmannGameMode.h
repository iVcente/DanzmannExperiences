// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "DanzmannGameMode.generated.h"

class UDanzmannExperienceDefinition;
class UDanzmannPawnData;

/**
 * Base Game Mode class for the Experience system. Resolves the active UDanzmannExperienceDefinition
 * (URL option ?Experience= -> ADanzmannWorldSettings.DefaultExperience), pushes the resolved ID
 * into the Game State's Experience Manager Component, and spawns each player's Pawn via the class 
 * declared on the Experience's UDanzmannPawnData.
 */
UCLASS()
class DANZMANNEXPERIENCES_API ADanzmannGameMode : public AGameModeBase
{
	GENERATED_BODY()

	public:
		ADanzmannGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

		/**
		 * Add this class as a Game Framework Component Receiver through an Experience.
		 * @see more info in Actor.h.
		 */
		virtual void PreInitializeComponents() override;
		
		/**
		 * Remove this class as a Game Framework Component Receiver through an Experience.
		 * @see more info in Actor.h.
		 */
		virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

		/**
		 * Resolve Experience ID.
		 * @see more info in GameModeBase.h.
		 */
		virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

		/**
		 * Load resolved Experience ID.
		 * @see more info in GameModeBase.h.
		 */
		virtual void InitGameState() override;
		
		/**
		 * Get Pawn class from Experience's Pawn Data instead of the inherited DefaultPawnClass.
		 * It's the single source of truth for Pawn-class resolution: which class should be spawned?
		 * @see more info in GameModeBase.h.
		 */
		virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
		
		/**
		 * Just choosing the Pawn class with GetDefaultPawnClassForController() is not enough. How do spawn and initialize it?
		 * Here we set Pawn's own initialization -- abilities, input, mesh, etc. -- from Pawn Data.
		 * @see more info in GameModeBase.h.
		 */
		virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
		
		/**
		 * Pawn Data used to spawn a Pawn for Controller. Defaults to the active Experience's DefaultPawnData.
		 * Subclasses may override to pick per-Controller/per-team variants.
		 * @param Controller Controller to get Pawn Data for.
		 */
		virtual const UDanzmannPawnData* GetPawnDataForController(const AController* Controller) const;

	private:
		/**
		 * Try to resolve Experience ID giving higher priority to URL options,
		 * and then falling back to World Settings's default Experience if necessary.
		 * @param Options URL options.
		 * @return Experience ID -- if present in URL options, or World Settings.
		 */
		FPrimaryAssetId ResolveExperienceId(const FString& Options) const;

		/**
		 * Experience ID.
		 */
		FPrimaryAssetId ExperienceId;
};
