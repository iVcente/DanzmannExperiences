// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "DanzmannExperienceDefinition.generated.h"

class UDanzmannExperienceAction;
class UDanzmannPawnData;

/**
 * Immutable data asset describing one session configuration. It carries the default Pawn Data
 * that the Game Mode resolves into a Pawn class, plus an instanced list of session-modifier
 * Actions executed by UDanzmannExperienceManagerComponent.
 */
UCLASS(BlueprintType, Const, MinimalAPI)
class UDanzmannExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
		/**
		 * @see more info in Object.h.
		 */
		virtual FPrimaryAssetId GetPrimaryAssetId() const override;

		/**
		 * Primary Asset Type for UDanzmannExperienceDefinition. Single source of truth for both
		 * GetPrimaryAssetId() and any caller building a short-name FPrimaryAssetId
		 * (e.g., URL-option ?Experience= parsing in ADanzmannGameMode).
		 */
		static const FPrimaryAssetType PrimaryAssetType;

		/**
		 * Default Pawn Data for this Experience.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Settings")
		TSoftObjectPtr<UDanzmannPawnData> DefaultPawnData = nullptr;

		/**
		 * Actions executed when this Experience activates and then reverted on deactivation. Order
		 * matters: activated top-to-bottom, deactivated bottom-to-top -- mirroring Lyra's convention so
		 * Actions can depend on earlier ones (e.g., AddComponents before a Component-aware widget Action).
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Instanced, Category = "Settings")
		TArray<TObjectPtr<UDanzmannExperienceAction>> Actions;
};
