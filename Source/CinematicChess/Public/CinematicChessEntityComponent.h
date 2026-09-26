#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChessEntityDNA.h"
#include "CinematicChessEntityComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChessEntityInitialized, bool, bSuccess);

/**
 * Runtime entity component for one cinematic chess piece.
 *
 * Owns the FChessEntitySpec (Elia, Phase 0 rework) and applies it to the
 * owning actor: resolves the asset name, binds armor/weapon to skeleton
 * sockets, assigns the movement archetype, and starts the idle motion set.
 *
 * It reads FCharacterDNA from Elisa's PR #2 — it never redefines it.
 */
UCLASS(ClassGroup = (CinematicChess), meta = (BlueprintSpawnableComponent))
class CINEMATICCHESS_API UCinematicChessEntityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCinematicChessEntityComponent();

    /** Full entity spec. Set in editor or via InitializeFromSpec. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Chess")
    FChessEntitySpec EntitySpec;

    /** Fires after initialization. False = spec invalid, see log. */
    UPROPERTY(BlueprintAssignable, Category = "Cinematic Chess")
    FOnChessEntityInitialized OnEntityInitialized;

    /** Applies a spec to this entity and initializes it. Returns false on invalid spec. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess")
    bool InitializeFromSpec(const FChessEntitySpec& Spec);

    /** Binds the piece to a board square ("e4"). Returns false on bad notation. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess")
    bool BindToBoardSquare(const FString& Square);

    /** Starts the idle motion set: sway, blink, hover pulse. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess")
    void PlayIdleMotion();

    /** Validates every asset ID against CinematicChessAssetRules. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess")
    bool ValidateAssetNaming(FString& OutError) const;

protected:
    virtual void BeginPlay() override;

private:
    bool bInitialized = false;

    bool ApplyArmorBindings();
    bool ApplyWeaponBinding();
    bool ApplyMovementArchetype();

    /** Socket name for an armor slot: SKT_Armor_<SlotName>. */
    static FString ArmorSocketName(EChessArmorSlot Slot);
    /** Socket name for a weapon hardpoint: SKT_Weapon_<SocketName>. */
    static FString WeaponSocketName(EChessWeaponSocket Socket);

    static bool IsValidBoardSquare(const FString& Square);
};
