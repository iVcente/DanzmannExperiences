// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DanzmannAbilitySystemComponentOwner.h"
#include "Engine/DataAsset.h"

#include "DanzmannPawnData.generated.h"

class APawn;
class UAnimInstance;
class UCameraAsset;
class UDanzmannGameplayBundle;
class UDanzmannInputProfile;
class UDanzmannMovementProfile;
class USkeletalMesh;

/**
 * Immutable data asset describing one playable/spawnable Pawn variant. Designed so a thin C++ Pawn
 * class can be reused across many in-data character variants -- nearly all per-character configuration
 * (visual, input, abilities, movement, camera) lives here and is applied by UDanzmannPawnDataComponent.
 *
 * Game-side projects extend this class to add project-specific fields (e.g., default weapon, AI hints).
 */
UCLASS(BlueprintType, Const, MinimalAPI)
class UDanzmannPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

	public:
		/**
		 * @see more info in Object.h.
		 */
		DANZMANNEXPERIENCES_API virtual FPrimaryAssetId GetPrimaryAssetId() const override;

		/**
		 * Primary Asset Type for UDanzmannPawnData. Single source of truth for both
		 * GetPrimaryAssetId() and any caller building a short-name FPrimaryAssetId for Pawn Data.
		 */
		static const FPrimaryAssetType PrimaryAssetType;

		/**
		 * Pawn class spawned when this Pawn Data is the active selection.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Core")
		TSubclassOf<APawn> PawnClass = nullptr;
		
		/**
		 * Movement Profile pushed onto the Pawn's UDanzmannMoverComponent.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Core")
		TSoftObjectPtr<UDanzmannMovementProfile> MovementProfile = nullptr;

		/**
		 * Input Profile used for SetupPlayerInputComponent(). Carries both Input`Actions and Input Mapping Contexts.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Core")
		TObjectPtr<UDanzmannInputProfile> InputProfile = nullptr;

		/**
		 * Camera Asset applied to the Pawn's UGameplayCameraComponent. Pawns without a
		 * Gameplay Camera Component (e.g., AI Pawns) silently skip this field during apply.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Core")
		TSoftObjectPtr<UCameraAsset> CameraAsset = nullptr;
		
		/**
		 * Whether this Pawn is part of the Gameplay Ability System.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Core|GAS", Meta = (InlineEditConditionToggle))
		bool bUseAbilitySystemComponent = true;

		/**
		 * Owner type for this Pawn's Ability System Component.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Core|GAS", Meta = (EditCondition = bUseAbilitySystemComponent))
		EDanzmannAbilitySystemComponentOwner AbilitySystemComponentOwner = EDanzmannAbilitySystemComponentOwner::Pawn;
		
		/**
		 * Gameplay Bundles to grant to Ability System Component.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Core|GAS", Meta = (EditCondition = bUseAbilitySystemComponent))
		TArray<TObjectPtr<UDanzmannGameplayBundle>> GameplayBundles;
		
		/**
		 * Half-height of the Pawn's root capsule collider.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision")
		float CapsuleHalfHeight = 88.0f;

		/**
		 * Radius of the Pawn's root capsule collider.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Collision")
		float CapsuleRadius = 34.0f;

		/**
		 * Skeletal mesh applied to the Pawn's MainSkeletalMesh Component.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Visuals")
		TSoftObjectPtr<USkeletalMesh> MainSkeletalMesh = nullptr;

		/**
		 * Animation Blueprint class applied to the Pawn's MainSkeletalMesh Component.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Visuals")
		TSoftClassPtr<UAnimInstance> MainAnimationBlueprint = nullptr;

		/**
		 * Relative location of the main skeletal mesh against the root Component.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Visuals")
		FVector MainSkeletalMeshRelativeLocation = FVector(0.0f, 0.0f, -88.0f);

		/**
		 * Relative rotation of the main skeletal mesh.
		 */
		UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Visuals")
		FRotator MainMeshRelativeRotation = FRotator(0.0f, -90.0f, 0.0f);
};
