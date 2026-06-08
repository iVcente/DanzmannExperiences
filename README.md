# DanzmannExperiences

Data-driven session and Pawn configuration. An **Experience** describes what a session loads; **Pawn Data** describes a spawnable Pawn variant. A thin set of `GameMode`, `GameState`, and`WorldSettings` base classes wire the two together so a map (or a `?Experience=` URL option) fully determines which Pawn is spawned and how it is configured -- visuals, input, movement, camera, and abilities -- without per-class C++.

The design mirrors Lyra's Experience + PawnData split, trimmed down to a single runtime module and built on `ModularGameplay` for component injection.

> Depends on the sibling Danzmann plugins `DanzmannAbilities`, `DanzmannInput`, and `DanzmannMovement`. It's also required to set the project's Asset Manager so `ExperienceDefinition` and `PawnData` Primary Asset Types are scanned (see [Configure the Asset Manager](#1-configure-the-asset-manager)).

## Concepts

### Experiences (session scope -- owned by the Game State)

- `UDanzmannExperienceDefinition` (`UPrimaryDataAsset`): one immutable session configuration. Carries a `DefaultPawnData` the Game Mode resolves into a Pawn class, plus an array of `Actions`. Actions run top-to-bottom on activation and bottom-to-top on deactivation, so later Actions may depend on earlier ones. `PrimaryAssetType` is the single source of truth for ID construction (e.g., `?Experience=` parsing);
- `UDanzmannExperienceManagerComponent` (`UGameStateComponent`): loads, replicates, and announces the active Experience.
  - `LoadExperience(ExperienceId)`: server-only. Resolves the ID, stores `LoadedExperience` (replicated via `OnRep_LoadedExperience`), then notifies and activates Actions. Clients run the same broadcast off the OnRep;
  - One-shot, priority-tiered notification: `OnExperienceLoaded_HighPriority` -> `OnExperienceLoaded_MediumPriority` -> `OnExperienceLoaded_LowPriority`. Each tier is `Broadcast()` then `Clear()` is called. A consumer registering after load fires immediately via the `bExperienceLoaded` fast-path. Delegates are rvalue-only -- pass inline or `MoveTemp()`;
  - Notifications fires before `ActivateActions()`. Listeners can see `GetLoadedExperience()`, but must not assume Action-mutated state (e.g., Components added by `AddComponents`) exists yet -- defer that to the next tick or a ModularGameplay init-state callback;
  - `DeactivateActions()` runs in reverse on `EndPlay()`, so everything unwinds with the Game State's world.
- `UDanzmannExperienceAction` (`UObject`): a lighter version of `UGameFeatureAction`. Override `OnExperienceActivated()`/`OnExperienceDeactivated()` -- each is called once per session by the manager.
  - `UDanzmannExperienceAction_AddComponents` (`UDanzmannExperienceAction`): attaches modular Components to ModularGameplay receivers. Each `FDanzmannExperienceAddComponentEntry` pairs a `ComponentClass` with a target `ActorClass` plus `bClientComponent`/`bServerComponent` net-mode flags. Activation registers one Component request per entry; deactivation drops the `FComponentRequestHandle`s, and the Component manager removes the Components -- no explicit remove calls. Target Actors must call `UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver()` in `PreInitializeComponents()` (the `ADanzmannGameMode` and `ADanzmannGameState` already do);
  - `UDanzmannExperienceAction_SplitscreenSetup` (`UDanzmannExperienceAction`): force-disables splitscreen while active. Uses a static vote counter (only the 0 -> 1 transition disables, only 1 -> 0 re-enables) with per-manager `TWeakObjectPtr` bookkeeping so overlapping worlds (multi-client PIE, future Experience swaps) unwind correctly.

### Pawn scope

- `UDanzmannPawnData` (`UPrimaryDataAsset`): one spawnable Pawn variant. Lets a thin C++ Pawn class be reused across many in-data characters. Game-side projects subclass this to add project-specific fields (e.g., a default weapon). Default fields:
  - `PawnClass`: the class spawned when this Pawn Data is selected;
  - `MovementProfile`: pushed onto the Pawn's `UDanzmannMoverComponent` (from `DanzmannMovement`);
  - `InputProfile`: `UDanzmannInputProfile` (from `DanzmannInput`); sourced by the Component but applied by the Pawn at `SetupPlayerInputComponent()` time;
  - `CameraAsset`: applied to the Pawn's `UGameplayCameraComponent`. Pawns without one (e.g., AI) silently skip it;
  - GAS: `bUseAbilitySystemComponent`, `AbilitySystemComponentOwner` (`Pawn` or `PlayerState`), and `GameplayBundles` granted to the ASC;
  - Visuals: `MainSkeletalMesh` and `MainAnimationBlueprint`.
- `UDanzmannPawnDataComponent` (`UPawnComponent`, implements `IDanzmannInputProfileSourceInterface`): replicated holder and idempotent applier of a `UDanzmannPawnData`.
  - `SetPawnData(NewPawnData)`: server-only entry point. Stores + replicates the data and applies it immediately. No-ops on clients and on no-op swaps;
  - `ClearPawnData()`: server-only teardown -- revokes granted Gameplay Bundles and clears the applied Input Profile, but deliberately leaves mesh, AnimBP, movement, camera in place to avoid a visual pop (the next `SetPawnData()` overwrites them);
  - `ApplyPawnData()`: the shared worker for `SetPawnData()`, `OnRep_PawnData()`, and explicit runtime swaps. Pushes mesh + AnimBP, Movement Profile, Camera Asset (gated on `IsLocallyControlled()`), and grants Gameplay Bundles to the ASC. A `LastAppliedPawnData` identity check short-circuits the authority-only revoke/re-grant cycle on redundant re-applies. ASC granting is skipped silently until `InitAbilityActorInfo()` has run;
  - Subscribes to `APawn::ReceiveControllerChangedDelegate` in `OnRegister()` so possession re-triggers the apply pass -- without it the locally-controlled-only camera step would never land on a listen server's host Pawn (apply runs during spawn, before `Possess()`);
  - `OnPawnDataUpdatedDelegate` broadcasts after every successful apply.
- `EDanzmannAbilitySystemComponentOwner`: `Pawn` or `PlayerState` -- selects where the ASC's actor info points.

### Game framework base classes

- `ADanzmannGameMode`: resolves the active Experience (prioritizing `?Experience=` URL option over `ADanzmannWorldSettings::DefaultExperience`) in `InitGame()`, pushes the ID into the Game State's manager in `InitGameState()`, and spawns each player's Pawn from `PawnData->PawnClass` (overriding `GetDefaultPawnClassForController()` and `SpawnDefaultPawnAtTransform()`). Override `GetPawnDataForController()` to pick per-Controller/per-team variants;
- `ADanzmannGameState`: hosts the `UDanzmannExperienceManagerComponent`;
- `ADanzmannWorldSettings`: lets each map declare its `DefaultExperience`.

## Usage

### 1. Configure the Asset Manager

`ExperienceDefinition` and `PawnData` are resolved by ID (e.g. `?Experience=` parsing and `DefaultPawnData` lookups), so the Asset Manager must scan both Primary Asset Types or the Game Mode finds nothing to spawn. Add them under `[/Script/Engine.AssetManagerSettings]` in `DefaultGame.ini` (or set them via **Project Settings -> Asset Manager -> Primary Asset Types to Scan**). Point `Directories` at wherever your project keeps the assets:

```ini
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="PawnData",AssetBaseClass="/Script/DanzmannExperiences.DanzmannPawnData",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path="/Game/MyProject/PawnData")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=Unknown))
+PrimaryAssetTypesToScan=(PrimaryAssetType="ExperienceDefinition",AssetBaseClass="/Script/DanzmannExperiences.DanzmannExperienceDefinition",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path="/Game/MyProject/Experiences")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=Unknown))
```

Subclassing `UDanzmannPawnData` or `UDanzmannExperienceDefinition` in your project? Either point `AssetBaseClass` at the subclass or add an extra entry for it.

### 2. Author the data assets

- An **Experience Definition** with a `DefaultPawnData` and any Actions (e.g., an `Add Components` Action injecting gameplay Components onto your Pawn, PlayerState, and Game State);
- A **Pawn Data** with a `PawnClass`, skeletal mesh, AnimBP, Input/Movement Profiles, Camera Asset, and Gameplay Bundles.

### 3. Set the map up

Set the projects's **World Settings** class to `ADanzmannWorldSettings` (or a subclass) and assign `DefaultExperience` on a map. Set the project/map **Game Mode** to a `ADanzmannGameMode` subclass and **Game State** to a `ADanzmannGameState` subclass. No Pawn class is set on the Game Mode -- it comes from the Experience's Pawn Data.

### 4. React to the loaded Experience

```cpp
// From any system that needs the active Experience (e.g., a UI or spawning subsystem).
if (ADanzmannGameState* GameState = World->GetGameState<ADanzmannGameState>())
{
    UDanzmannExperienceManagerComponent* Manager = GameState->GetExperienceManagerComponent();

    // Fires immediately if already loaded, otherwise when LoadExperience() completes.
    Manager->OnExperienceLoaded_HighPriority(
        FDanzmannOnExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::HandleExperienceLoaded));
}

void UMySubsystem::HandleExperienceLoaded(const UDanzmannExperienceDefinition* Experience)
{
    // Experience is visible here, but Action-mutated state may not exist yet.
}
```

### 5. Swap Pawn Data at runtime (server)

```cpp
// Server authority. Re-applies visuals/movement/camera/abilities and replicates to clients.
if (UDanzmannPawnDataComponent* PawnDataComponent = Pawn->FindComponentByClass<UDanzmannPawnDataComponent>())
{
    PawnDataComponent->SetPawnData(NewPawnData);
}
```

## Lifecycle at a glance

```
ADanzmannGameMode::InitGame()        -> resolve Experience ID (URL > WorldSettings)
ADanzmannGameMode::InitGameState()   -> ExperienceManagerComponent->LoadExperience(ID)
  └─ server load (sync)              -> set LoadedExperience (replicates)
       └─ NotifyExperienceLoaded()   -> High -> Medium -> Low (one-shot)
            └─ ActivateActions()      -> Action::OnExperienceActivated() top-to-bottom
ADanzmannGameMode::SpawnDefaultPawn  -> spawn PawnData->PawnClass, SetPawnData()
  └─ ApplyPawnData()                 -> skeletal mesh / AnimBP / Mover / Input / Camera / Gameplay Bundles
       └─ possession re-applies camera (IsLocallyControlled gate)
EndPlay()                            -> DeactivateActions() bottom-to-top
```

---

Part of the Danzmann plugin suite. See also [`DanzmannInput`](https://github.com/iVcente/DanzmannInput), [`DanzmannMovement`](https://github.com/iVcente/DanzmannMovement), and [`DanzmannAbilities`](https://github.com/iVcente/DanzmannAbilities).
