#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
// Canonical DNA schema (Elisa's PR #2). The factory consumes it; it never redefines it.
#include "CharacterDNA.h"
#include "ChessEntityFactory.generated.h"

class ACinematicChessEntity;

/**
 * UCinematicChessEntityFactory
 * Spawns entities from DNA blocks: single pieces, or the full 32-piece
 * cinematic set placed on their regulation starting squares.
 */
UCLASS()
class CINEMATICCHESS_API UCinematicChessEntityFactory : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Spawn one entity from a DNA block at a world transform. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess|Factory")
    static ACinematicChessEntity* SpawnFromDNA(
        UObject* WorldContextObject,
        TSubclassOf<ACinematicChessEntity> EntityClass,
        const FCharacterDNA& DNA,
        const FTransform& SpawnTransform);

    /**
     * Spawn the full 32-piece set from 32 DNA blocks.
     * White occupies ranks 1-2, Black occupies ranks 7-8, standard layout.
     * Returns the spawned entities in DNA order.
     */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess|Factory")
    static TArray<ACinematicChessEntity*> SpawnFullSet(
        UObject* WorldContextObject,
        TSubclassOf<ACinematicChessEntity> EntityClass,
        const TArray<FCharacterDNA>& RosterDNA,
        float SquareSizeCm = 200.0f);

    /** Validates a 32-block roster: counts per faction/piece, legal squares, no duplicate IDs. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess|Factory")
    static bool ValidateRoster(const TArray<FCharacterDNA>& RosterDNA, FString& OutError);

private:
    /** Regulation starting square for a faction + piece index (0-15 per side). */
    static FString StartingSquareFor(EChessFaction Faction, EChessPieceType PieceType, int32 PieceIndex);

    static FVector SquareToWorld(const FString& Algebraic, float SquareSizeCm);
};
