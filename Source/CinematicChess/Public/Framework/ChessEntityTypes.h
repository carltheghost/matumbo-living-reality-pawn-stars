#pragma once

#include "CoreMinimal.h"
// NOTE: this header intentionally does NOT declare EChessFaction or
// EChessPieceType — those are canonical in Elisa's CharacterDNA.h (PR #2).
// It also does NOT declare FCharacterDNA. Everything here references hers.
#include "CharacterDNA.h"
#include "ChessEntityTypes.generated.h"

/**
 * FChessBoardCoordinate
 * Shared board math for the entity framework: algebraic squares ("e4"),
 * zero-based row/column, and the standard back-rank layouts.
 */
USTRUCT(BlueprintType)
struct FChessBoardCoordinate
{
    GENERATED_BODY()

    /** 0-7, rank index from White's side. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Row = 0;

    /** 0-7, file index a-h. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Col = 0;

    /** "e4" -> {Row, Col}. Returns false on malformed input. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess|Board")
    static bool FromAlgebraic(const FString& Algebraic, FChessBoardCoordinate& OutCoord)
    {
        FString Lower = Algebraic.ToLower().TrimStartAndEnd();
        if (Lower.Len() != 2)
        {
            return false;
        }
        const int32 File = Lower[0] - TEXT('a');
        const int32 Rank = Lower[1] - TEXT('1');
        if (File < 0 || File > 7 || Rank < 0 || Rank > 7)
        {
            return false;
        }
        OutCoord.Col = File;
        OutCoord.Row = Rank;
        return true;
    }

    /** {Row, Col} -> "e4". */
    UFUNCTION(BlueprintPure, Category = "Cinematic Chess|Board")
    FString ToAlgebraic() const
    {
        return FString::Printf(TEXT("%c%c"), TEXT('a') + Col, TEXT('1') + Row);
    }

    UFUNCTION(BlueprintPure, Category = "Cinematic Chess|Board")
    bool IsValid() const { return Row >= 0 && Row < 8 && Col >= 0 && Col < 8; }
};

/** How a capture resolves on screen. */
UENUM(BlueprintType)
enum class EChessCaptureStyle : uint8
{
    /** Loser plays the defeat cinematic and leaves the board. */
    Standard,
    /** Full combat beat between the two entities before resolution. */
    CinematicDuel
};
