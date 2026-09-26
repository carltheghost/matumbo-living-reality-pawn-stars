#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChessEntityLifecycle.generated.h"

/** Where an entity is in its on-board life. */
UENUM(BlueprintType)
enum class EEntityLifecycleState : uint8
{
    /** Not on the board. No visuals, no tick cost. */
    Dormant,
    /** Cinematic entrance: materialize / rise / armor-up sequence. */
    Materializing,
    /** On its square, breathing idle. */
    Idle,
    /** Player has this piece selected. */
    Selected,
    /** Gliding to a new square. */
    Moving,
    /** Combat beat: striking or being struck. */
    Engaged,
    /** Defeated: death/capture cinematic, then off-board. */
    Captured,
    /** Fully removed from play. */
    Removed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnLifecycleStateChanged,
    EEntityLifecycleState, OldState,
    EEntityLifecycleState, NewState);

/**
 * UChessEntityLifecycleComponent
 * Owns the entity's state machine so ACinematicChessEntity stays focused on
 * visuals. Transitions are validated: illegal jumps (e.g. Dormant -> Engaged)
 * are rejected and logged instead of silently applied.
 */
UCLASS(Blueprintable, ClassGroup = (CinematicChess), meta = (BlueprintSpawnableComponent))
class CINEMATICCHESS_API UChessEntityLifecycleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UChessEntityLifecycleComponent();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Chess|Lifecycle")
    EEntityLifecycleState CurrentState = EEntityLifecycleState::Dormant;

    /** Seconds the materialize cinematic runs before reaching Idle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Chess|Lifecycle")
    float MaterializeDuration = 2.5f;

    /** Request a state change. Returns false when the transition is illegal. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess|Lifecycle")
    bool RequestState(EEntityLifecycleState NewState);

    UPROPERTY(BlueprintAssignable, Category = "Cinematic Chess|Lifecycle")
    FOnLifecycleStateChanged OnStateChanged;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool IsTransitionLegal(EEntityLifecycleState From, EEntityLifecycleState To) const;
    void EnterState(EEntityLifecycleState NewState);

    float StateTime = 0.0f;
};
