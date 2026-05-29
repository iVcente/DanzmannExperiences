// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "Components/PawnComponent.h"
#include "CoreMinimal.h"
#include "DanzmannInputProfileSourceInterface.h"

#include "DanzmannPawnDataComponent.generated.h"

class UDanzmannInputProfile;
class UDanzmannPawnData;

/**
 * Delegate to notify Pawn Data has been updated.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDanzmannOnPawnDataUpdated, const UDanzmannPawnData* OldPawnData, const UDanzmannPawnData* NewPawnData);

/**
 * Replicated holder and applier for a UDanzmannPawnData onto the right system -- skeletal mesh + AnimBP, 
 * Mover Profile, Input Profile, Camera Asset, Gameplay Bundles the ASC, and so on. 
 *
 * Apply triggers: server SetPawnData(), OnRep() on clients, and explicit ApplyPawnData() for runtime swap.
 * Granted handles are tracked so re-application revokes the prior set before granting the new one.
 */
UCLASS(ClassGroup = "Danzmann", HideCategories = ("Activation", "AssetUserData", "ComponentTick", "ComponentReplication", "Cooking", "Navigation", "Replication"), Meta = (BlueprintSpawnableComponent))
class DANZMANNEXPERIENCES_API UDanzmannPawnDataComponent : public UPawnComponent, public IDanzmannInputProfileSourceInterface
{
	GENERATED_BODY()

	public:
		UDanzmannPawnDataComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

		/**
		 * Source the current Pawn Data's Input Profile for the owning Pawn to apply during
		 * SetupPlayerInputComponent().
		 * @see more info in DanzmannInputProfileSourceInterface.h.
		 */
		virtual const UDanzmannInputProfile* GetInputProfile() const override;
		
		/**
		 * @see more info in Object.h.
		 */
		virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		
		/**
		 * Idempotent apply pass. Pushes the current PawnData's fields onto the owning Pawn's Components,
		 * by default:
		 * - Skeletal mesh + AnimBP on main USkeletalMeshComponent;
		 * - Movement Profile on UDanzmannMoverComponent;
		 * - Input Profile sourced via GetInputProfile() and applied by the Pawn at SetupPlayerInputComponent() time, not here;
		 * - Camera Asset on UGameplayCameraComponent;
		 * - Gameplay Bundles granted to the owner's ASC. Skips granting silently when the ASC
		 *   has not yet had InitAbilityActorInfo() called. The owning Pawn should re-invoke
		 *   ApplyPawnData() after ASC initialization.
		 *
		 * Shared worker for both code paths -- server's SetPawnData() and client's OnRep_PawnData(). 
		 */
		UFUNCTION(BlueprintCallable)
		void ApplyPawnData();

		/**
		 * Server-only entrypoint. Stores NewPawnData, marks it dirty for replication, and runs ApplyPawnData()
		 * immediately so the server-side Pawn is configured before remote clients receive the OnRep().
		 * No-ops on remote clients and when the new PawnData matches the current one.
		 * @param NewPawnData New PawnData to apply. Must be non-null. Use ClearPawnData() to tear down the active PawnData explicitly.
		 */
		UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
		void SetPawnData(const UDanzmannPawnData* NewPawnData);

		/**
		 * Server-only teardown of the active PawnData. Revokes granted Gameplay Bundles, clears the
		 * applied Input Profile, nulls PawnData (replicates), and broadcasts OnPawnDataUpdatedDelegate
		 * with NewPawnData = nullptr so listeners can react.
		 *
		 * Does NOT reset visual state (mesh/AnimBP/movement profile/camera) -- clearing those would
		 * visually pop the Pawn. The next SetPawnData() will overwrite them anyway.
		 */
		UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
		void ClearPawnData();

		/**
		 * Get current Pawn Data.
		 * @return Current Pawn Data.
		 */
		UFUNCTION(BlueprintPure)
		const UDanzmannPawnData* GetPawnData() const
		{
			return PawnData;
		}
		
		/**
		 * Current Pawn Data.
		 */
		UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PawnData)
		TObjectPtr<const UDanzmannPawnData> PawnData = nullptr;

		/**
		 * Broadcast at the end of every successful ApplyPawnData() pass (server SetPawnData(), client OnRep(),
		 * or explicit re-apply).
		 */
		FDanzmannOnPawnDataUpdated OnPawnDataUpdatedDelegate;

		/**
		 * FName the Component is created under. Pass to FObjectInitializer::SetDefaultSubobjectClass<>()
		 * to swap in a subclass on derived Pawn classes.
		 */
		static FName ComponentName;

	protected:
		/**
		 * Subscribe to the owning Pawn's ReceiveControllerChangedDelegate so possession events
		 * trigger an ApplyPawnData() re-run. Without this hook the camera asset never lands on the
		 * listen-server's host Pawn -- ApplyPawnData() fires during SpawnDefaultPawnAtTransform() before
		 * Possess() runs, IsLocallyControlled() is false, and there's no later trigger. The same race
		 * exists on remote clients when PawnData replication beats controller possession.
		 * @see more info in ActorComponent.h.
		 */
		virtual void OnRegister() override;

		/**
		 * @see more info in ActorComponent.h.
		 */
		virtual void OnUnregister() override;

	private:
		/**
		 * OnRep() for when PawnData is updated.
		 * @param OldPawnData Old Pawn Data.
		 */
		UFUNCTION()
		void OnRep_PawnData(const UDanzmannPawnData* OldPawnData);

		/**
		 * Handler bound to APawn::ReceiveControllerChangedDelegate. Re-runs ApplyPawnData() so the
		 * camera block (gated on IsLocallyControlled()) finally fires once a Controller is bound.
		 * Other apply branches are idempotent and bail out cheaply via the bundle-set identity check.
		 */
		UFUNCTION()
		void OnPawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

		/**
		 * Revoke every ability tracked in GrantedGameplayBundles (authority only) and clear the list.
		 */
		void RevokeGrantedGameplayBundles();

		/**
		 * Gameplay Bundles granted by this Component, tracked by FPrimaryAssetId so they can be
		 * revoked.
		 */
		TArray<FPrimaryAssetId> GrantedGameplayBundles;

		/**
		 * Last PawnData pointer that successfully completed an ApplyPawnData() pass on this Component.
		 * Used to short-circuit the authority-only revoke+regrant cycle when ApplyPawnData() runs again
		 * with the same PawnData -- which happens routinely (server SetPawnData() followed by the
		 * possession-driven re-apply, BeginPlay re-apply, etc.). ClearPawnData() resets this so a subsequent
		 * SetPawnData() with the same asset re-grants the bundles it just revoked.
		 */
		TWeakObjectPtr<const UDanzmannPawnData> LastAppliedPawnData = nullptr;
};
