// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#include "Pawns/DanzmannPawnDataComponent.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/CameraAsset.h"
#include "Core/CameraAssetReference.h"
#include "DanzmannAbilitySystemComponent.h"
#include "DanzmannAbilitySystemGlobals.h"
#include "DanzmannEnhancedInputComponent.h"
#include "DanzmannLogDanzmannExperiences.h"
#include "DanzmannMoverComponent.h"
#include "DanzmannMovementProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Pawns/DanzmannPawnData.h"

FName UDanzmannPawnDataComponent::ComponentName(TEXT("DanzmannPawnDataComponent"));

UDanzmannPawnDataComponent::UDanzmannPawnDataComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UDanzmannPawnDataComponent::OnRegister()
{
	Super::OnRegister();

	if (APawn* OwnerPawn = GetPawn<APawn>(); IsValid(OwnerPawn))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::OnPawnControllerChanged);
	}
}

void UDanzmannPawnDataComponent::OnUnregister()
{
	if (APawn* OwnerPawn = GetPawn<APawn>(); IsValid(OwnerPawn))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::OnPawnControllerChanged);
	}

	Super::OnUnregister();
}

void UDanzmannPawnDataComponent::OnPawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	ApplyPawnData();
}

void UDanzmannPawnDataComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	FDoRepLifetimeParams Params;
	// Params.bIsPushBased = true; // Push Model
	Params.Condition = COND_None;

	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, Params);
}

void UDanzmannPawnDataComponent::ApplyPawnData()
{
	APawn* OwnerPawn = GetPawn<APawn>();
	
	if (!IsValid(OwnerPawn))
	{
		return;
	}
	
	if (!IsValid(PawnData))
	{
		return;
	}

	// Identity check against the last applied PawnData. UDanzmannPawnData is const, so pointer
	// identity implies the bundle set is unchanged. Skips both the revoke and the grant pass below
	// for the common re-apply case (server SetPawnData followed by possession-driven re-apply,
	// BeginPlay re-apply, etc.) and avoids burning ASC handles for no behavior change
	const bool bSameAsLastApplied = LastAppliedPawnData.Get() == PawnData;

	// Revoke previously-granted Gameplay Bundles first so re-application is idempotent and runtime swaps don't leak handles
	if (OwnerPawn->HasAuthority() && !bSameAsLastApplied)
	{
		RevokeGrantedGameplayBundles();
	}

	/*
	 * Skeletal mesh + AnimBP.
	 */
	
	if (USkeletalMeshComponent* SkeletalMeshComponent = OwnerPawn->FindComponentByClass<USkeletalMeshComponent>(); IsValid(SkeletalMeshComponent))
	{
		// TODO(async-load): Preload via UDanzmannAssetManager and resolve here without blocking.
		if (USkeletalMesh* LoadedSkeletalMesh = PawnData->MainSkeletalMesh.LoadSynchronous(); IsValid(LoadedSkeletalMesh))
		{
			SkeletalMeshComponent->SetSkeletalMeshAsset(LoadedSkeletalMesh);
		}

		// TODO(async-load): Preload via UDanzmannAssetManager and resolve here without blocking.
		if (UClass* LoadedAnimationBlueprint = PawnData->MainAnimationBlueprint.LoadSynchronous(); LoadedAnimationBlueprint != nullptr)
		{
			SkeletalMeshComponent->SetAnimInstanceClass(LoadedAnimationBlueprint);
		}

		SkeletalMeshComponent->SetRelativeLocationAndRotation(PawnData->MainSkeletalMeshRelativeLocation, PawnData->MainMeshRelativeRotation);
	}

	/*
	 * Capsule collider size.
	 */

	if (UCapsuleComponent* CapsuleComponent = OwnerPawn->FindComponentByClass<UCapsuleComponent>(); IsValid(CapsuleComponent))
	{
		CapsuleComponent->SetCapsuleSize(PawnData->CapsuleRadius, PawnData->CapsuleHalfHeight);
	}

	/*
	 * Movement Profile.
	 */
	
	if (UDanzmannMoverComponent* MoverComponent = OwnerPawn->FindComponentByClass<UDanzmannMoverComponent>(); IsValid(MoverComponent))
	{
		// TODO(async-load): Preload via UDanzmannAssetManager and resolve here without blocking.
		if (const UDanzmannMovementProfile* LoadedProfile = PawnData->MovementProfile.LoadSynchronous(); IsValid(LoadedProfile))
		{
			MoverComponent->SetMovementProfile(LoadedProfile);
		}
	}
	
	/*
	 * Input Profile.
	 *
	 * The Enhanced Input Component is created during possession (APawn::SetupPlayerInputComponent), so on
	 * the initial pre-possession passes (SetPawnData server-side, OnRep before controller is bound, BeginPlay
	 * on simulated proxies) OwnerPawn->InputComponent is null and the block silently skips. Possession-driven
	 * re-apply via OnPawnControllerChanged covers the locally-controlled path. When PawnData swaps at runtime
	 * on an already-possessed pawn, re-apply lands here.
	 */

	if (UInputComponent* InputComponent = OwnerPawn->InputComponent; InputComponent != nullptr)
	{
		if (UDanzmannEnhancedInputComponent* EnhancedInputComponent = Cast<UDanzmannEnhancedInputComponent>(InputComponent); IsValid(EnhancedInputComponent))
		{
			EnhancedInputComponent->ApplyInputProfile(GetInputProfile());
		}
		else
		{
			UE_LOG(LogDanzmannExperiences, Warning, TEXT("[%hs] Pawn \"%s\" has an InputComponent that is not a UDanzmannEnhancedInputComponent. Skipping Input Profile apply -- check DefaultInput.ini's DefaultInputComponentClass."), __FUNCTION__, *GetNameSafe(OwnerPawn));
		}
	}

	/*
	 * Camera Asset.
	 */

	if (OwnerPawn->IsLocallyControlled())
	{
		if (UGameplayCameraComponent* CameraComponent = OwnerPawn->FindComponentByClass<UGameplayCameraComponent>(); IsValid(CameraComponent))
		{
			// TODO(async-load): Preload via UDanzmannAssetManager and resolve here without blocking.
			if (UCameraAsset* LoadedCameraAsset = PawnData->CameraAsset.LoadSynchronous(); IsValid(LoadedCameraAsset))
			{
				const bool bCameraAssetChanged = CameraComponent->CameraReference.GetCameraAsset() != LoadedCameraAsset;
				if (bCameraAssetChanged)
				{
					CameraComponent->CameraReference.SetCameraAsset(LoadedCameraAsset);
				}

				if (bCameraAssetChanged || !CameraComponent->IsActive())
				{
					CameraComponent->ActivateCameraForPlayerController(GetController<APlayerController>());
				}
			}
		}
	}

	/*
	 * Gameplay Ability System.
	 */

	if (!PawnData->bUseAbilitySystemComponent)
	{
		return;
	}

	UDanzmannAbilitySystemComponent* DanzmannAbilitySystemComponent = Cast<UDanzmannAbilitySystemComponent>(UDanzmannAbilitySystemGlobals::GetAbilitySystemComponentFromPawnOrPlayerState(OwnerPawn));

	if (!IsValid(DanzmannAbilitySystemComponent))
	{
		UE_LOG(LogDanzmannExperiences, Warning, TEXT("[%hs] No Ability System Component found on Pawn or its Player State. Skipping Gameplay Bundle granting..."), __FUNCTION__);
		return;
	}

	// Initialize() is safe to call on both server and client
	if (!DanzmannAbilitySystemComponent->IsInitialized())
	{
		AActor* AbilitySystemComponentOwner = nullptr;

		if (PawnData->AbilitySystemComponentOwner == EDanzmannAbilitySystemComponentOwner::Pawn)
		{
			AbilitySystemComponentOwner = OwnerPawn;
		}
		else
		{
			AbilitySystemComponentOwner = OwnerPawn->GetPlayerState();
		}

		DanzmannAbilitySystemComponent->Initialize(AbilitySystemComponentOwner, OwnerPawn);
	}

	// Gameplay Bundle granting is server-authoritative. Skip the loop when the PawnData pointer is
	// unchanged -- the revoke above was skipped too, so the existing grants stay live.
	if (OwnerPawn->HasAuthority() && !bSameAsLastApplied)
	{
		for (const TObjectPtr<UDanzmannGameplayBundle>& GameplayBundle : PawnData->GameplayBundles)
		{
			if (IsValid(GameplayBundle))
			{
				DanzmannAbilitySystemComponent->GrantGameplayBundle(GameplayBundle);
				GrantedGameplayBundles.Add(GameplayBundle->GetPrimaryAssetId());
			}
		}
	}

	LastAppliedPawnData = PawnData;
}

const UDanzmannInputProfile* UDanzmannPawnDataComponent::GetInputProfile() const
{
	return IsValid(PawnData) ? PawnData->InputProfile : nullptr;
}

void UDanzmannPawnDataComponent::SetPawnData(const UDanzmannPawnData* NewPawnData)
{
	if (const AActor* Owner = GetOwner(); !IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	if (!ensureMsgf(IsValid(NewPawnData), TEXT("[%hs] NewPawnData is invalid. Use ClearPawnData() to tear down the active PawnData."), __FUNCTION__))
	{
		return;
	}

	if (PawnData == NewPawnData)
	{
		return;
	}

	const UDanzmannPawnData* OldPawnData = PawnData;
	PawnData = NewPawnData;

	ApplyPawnData();

	OnPawnDataUpdatedDelegate.Broadcast(OldPawnData, PawnData);
}

void UDanzmannPawnDataComponent::ClearPawnData()
{
	if (const AActor* Owner = GetOwner(); !IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	if (!IsValid(PawnData))
	{
		return;
	}

	const UDanzmannPawnData* OldPawnData = PawnData;

	// Authoritative gameplay teardown. The mesh / mover profile / camera intentionally stay applied
	// to avoid a visible pop -- the next SetPawnData() will replace them.
	RevokeGrantedGameplayBundles();
	
	// Reset the identity cache so a future SetPawnData() with the same asset re-grants
	// (otherwise the identity check would short-circuit the regrant we just need to do).
	LastAppliedPawnData.Reset();

	if (const APawn* OwnerPawn = GetPawn<APawn>(); IsValid(OwnerPawn))
	{
		if (UDanzmannEnhancedInputComponent* EnhancedInputComponent = Cast<UDanzmannEnhancedInputComponent>(OwnerPawn->InputComponent); IsValid(EnhancedInputComponent))
		{
			EnhancedInputComponent->ClearAppliedInputProfile();
		}
	}

	PawnData = nullptr;

	OnPawnDataUpdatedDelegate.Broadcast(OldPawnData, nullptr);
}

void UDanzmannPawnDataComponent::OnRep_PawnData(const UDanzmannPawnData* OldPawnData)
{
	if (IsValid(PawnData))
	{
		ApplyPawnData();
	}
	else
	{
		// Server cleared. Mirror the input-side teardown locally (Gameplay Bundle revoke is
		// authority-only -- ASC replication will surface the removal on its own)
		if (const APawn* OwnerPawn = GetPawn<APawn>(); IsValid(OwnerPawn))
		{
			if (UDanzmannEnhancedInputComponent* EnhancedInputComponent = Cast<UDanzmannEnhancedInputComponent>(OwnerPawn->InputComponent); IsValid(EnhancedInputComponent))
			{
				EnhancedInputComponent->ClearAppliedInputProfile();
			}
		}
	}

	OnPawnDataUpdatedDelegate.Broadcast(OldPawnData, PawnData);
}

void UDanzmannPawnDataComponent::RevokeGrantedGameplayBundles()
{
	UDanzmannAbilitySystemComponent* DanzmannAbilitySystemComponent = Cast<UDanzmannAbilitySystemComponent>(UDanzmannAbilitySystemGlobals::GetAbilitySystemComponentFromPawnOrPlayerState(GetPawn<APawn>()));
	
	if (!IsValid(DanzmannAbilitySystemComponent))
	{
		return;
	}
	
	for (const FPrimaryAssetId& GrantedGameplayBundle : GrantedGameplayBundles)
	{
		DanzmannAbilitySystemComponent->RevokeGameplayBundle(GrantedGameplayBundle);
	}
	
	GrantedGameplayBundles.Empty();
}
