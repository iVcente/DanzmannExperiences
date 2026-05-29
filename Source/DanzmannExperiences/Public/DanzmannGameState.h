// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "DanzmannGameState.generated.h"

class UDanzmannExperienceManagerComponent;

/**
 * Base Game State class hosting the UDanzmannExperienceManagerComponent 
 * that loads and replicates the active Experience.
 */
UCLASS()
class DANZMANNEXPERIENCES_API ADanzmannGameState : public AGameStateBase
{
	GENERATED_BODY()

	public:
		ADanzmannGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
		 * Get Experience Manager Component.
		 * @return Experience Manager Component.
		 */
		UFUNCTION(BlueprintPure, Category = "Danzmann")
		UDanzmannExperienceManagerComponent* GetExperienceManagerComponent() const
		{
			return ExperienceManagerComponent;
		}

	protected:
		/**
		 * Experience Manager Component.
		 */
		UPROPERTY(VisibleAnywhere, Category = "Experience")
		TObjectPtr<UDanzmannExperienceManagerComponent> ExperienceManagerComponent = nullptr;
};
