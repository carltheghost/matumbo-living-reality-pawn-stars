#include "Framework/ChessEntityLifecycle.h"

UChessEntityLifecycleComponent::UChessEntityLifecycleComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UChessEntityLifecycleComponent::BeginPlay()
{
    Super::BeginPlay();
    StateTime = 0.0f;
}

void UChessEntityLifecycleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    StateTime += DeltaTime;

    // The materialize cinematic resolves itself into Idle; everything else is
    // driven by explicit RequestState calls from game logic.
    if (CurrentState == EEntityLifecycleState::Materializing && StateTime >= MaterializeDuration)
    {
        RequestState(EEntityLifecycleState::Idle);
    }
}

bool UChessEntityLifecycleComponent::RequestState(EEntityLifecycleState NewState)
{
    if (NewState == CurrentState)
    {
        return true;
    }
    if (!IsTransitionLegal(CurrentState, NewState))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CinematicChess] Illegal lifecycle transition %d -> %d rejected."),
            static_cast<int32>(CurrentState), static_cast<int32>(NewState));
        return false;
    }
    EnterState(NewState);
    return true;
}

bool UChessEntityLifecycleComponent::IsTransitionLegal(
    EEntityLifecycleState From, EEntityLifecycleState To) const
{
    switch (From)
    {
    case EEntityLifecycleState::Dormant:
        return To == EEntityLifecycleState::Materializing || To == EEntityLifecycleState::Removed;
    case EEntityLifecycleState::Materializing:
        return To == EEntityLifecycleState::Idle || To == EEntityLifecycleState::Removed;
    case EEntityLifecycleState::Idle:
        return To == EEntityLifecycleState::Selected || To == EEntityLifecycleState::Moving
            || To == EEntityLifecycleState::Engaged || To == EEntityLifecycleState::Removed;
    case EEntityLifecycleState::Selected:
        return To == EEntityLifecycleState::Idle || To == EEntityLifecycleState::Moving
            || To == EEntityLifecycleState::Removed;
    case EEntityLifecycleState::Moving:
        return To == EEntityLifecycleState::Idle || To == EEntityLifecycleState::Engaged
            || To == EEntityLifecycleState::Removed;
    case EEntityLifecycleState::Engaged:
        return To == EEntityLifecycleState::Idle || To == EEntityLifecycleState::Captured
            || To == EEntityLifecycleState::Removed;
    case EEntityLifecycleState::Captured:
        return To == EEntityLifecycleState::Removed;
    case EEntityLifecycleState::Removed:
        return false;
    default:
        return false;
    }
}

void UChessEntityLifecycleComponent::EnterState(EEntityLifecycleState NewState)
{
    const EEntityLifecycleState OldState = CurrentState;
    CurrentState = NewState;
    StateTime = 0.0f;
    OnStateChanged.Broadcast(OldState, NewState);
}
