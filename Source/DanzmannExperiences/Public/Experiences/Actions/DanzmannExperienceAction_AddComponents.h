// Copyright (C) 2026 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponentManager.h"
#include "CoreMinimal.h"
#include "Experiences/Actions/DanzmannExperienceAction.h"

#include "DanzmannExperienceAction_AddComponents.generated.h"

class UActorComponent;

#pragma region DanzmannExperienceAddComponentEntry

	/**
	 * Mirrors Lyra's FGameFeatureComponentEntry. Part of a list within UDanzmannExperienceAction_AddComponents detailing
	 * which Component should be added to a given Actor class. 
	 * The server/client flags decide which net mode actually registers the request.
	 */
	USTRUCT(BlueprintType)
	struct FDanzmannExperienceAddComponentEntry
	{
		GENERATED_BODY()
		
		/**
		 * Component class to add to Actor.
		 */
		UPROPERTY(EditAnywhere)
		TSubclassOf<UActorComponent> ComponentClass;

		/**
		 * Actor class to add Component to.
		 */
		UPROPERTY(EditAnywhere)
		TSoftClassPtr<AActor> ActorClass;
		
		/**
		 * Whether Component should be added on client side.
		 */
		UPROPERTY(EditAnywhere)
		bool bClientComponent = true;

		/**
		 * Whether Component should be added on server side.
		 */
		UPROPERTY(EditAnywhere)
		bool bServerComponent = true;
		
		#if WITH_EDITORONLY_DATA
			/**
			 * Editor Display Name to improve struct readability in the Editor.
			 * @note VisibleAnywhere (or similar) is required so the TitleProperty specifier code path can find this property.
			 * EditCondition and EditConditionHides is just to keep this property hidden to the user. 
			 */
			UPROPERTY(VisibleAnywhere, Meta = (EditCondition = false, EditConditionHides))
			FString EditorDisplayName;
		#endif

		/**
		 * Format EditorDisplayName here.
		 * @see more info in Class.h.
		 */
		void PostSerialize(const FArchive& Ar);
	};

	/**
	 * Boilerplate so PostSerialize() is called.
	 */
	template<>
	struct TStructOpsTypeTraits<FDanzmannExperienceAddComponentEntry> : public TStructOpsTypeTraitsBase2<FDanzmannExperienceAddComponentEntry>
	{
		enum
		{
			WithPostSerialize = true,
		};
	};

#pragma endregion DanzmannExperienceAddComponentEntry

/**
 * Experience Action that attaches modular Components to ModularGameplay receivers (any Actor that
 * calls UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver() in PreInitializeComponents()).
 * Activation registers one component request per entry.
 * Deactivation drops the request handles, which causes the Component manager to remove the Components.
 */
UCLASS(MinimalAPI, DisplayName = "Add Components")
class UDanzmannExperienceAction_AddComponents final : public UDanzmannExperienceAction
{
	GENERATED_BODY()

	public:
		/**
		 * @see more info in DanzmannExperienceAction.h.
		 */
		virtual void OnExperienceActivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent) override;
		
		/**
		 * @see more info in DanzmannExperienceAction.h.
		 */
		virtual void OnExperienceDeactivated(UDanzmannExperienceManagerComponent* ExperienceManagerComponent) override;

	private:
		UPROPERTY(EditAnywhere, Meta = (TitleProperty = EditorDisplayName))
		TArray<FDanzmannExperienceAddComponentEntry> ComponentsToAdd;

		/**
		 * Lifetime-owned request handles. Dropping these releases the Component requests -- explicit remove calls are not needed.
		 */
		TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;
};
