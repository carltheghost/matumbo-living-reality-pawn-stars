#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
// Canonical DNA schema — Elisa's PR #2 (merged). This module MUST NOT
// redeclare FCharacterDNA, EChessFaction, or EChessPieceType.
#include "CharacterDNA.h"
#include "Framework/ChessEntityLifecycle.h"
#include "CinematicChessEntity.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnChessEntityDNAApplied,
    ACinematicChessEntity*, Entity,
    const FCharacterDNA&, AppliedDNA);

/**
 * ACinematicChessEntity
 * Runtime incarnation of one cinematic chess piece. Identity comes entirely
 * from Elisa's canonical FCharacterDNA (PR #2); this class owns the body:
 * faction materials, piece architecture (centaur knights, castle-staff rooks),
 * MetaHuman face binding, weapon/armor attachment, and board presence.
 */
UCLASS(Blueprintable, BlueprintType)
class CINEMATICCHESS_API ACinematicChessEntity : public ACharacter
{
    GENERATED_BODY()

public:
    ACinematicChessEntity(const FObjectInitializer& ObjectInitializer);

    /** Canonical identity block. Drives every visual system below. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Chess|DNA")
    FCharacterDNA CharacterDNA;

    /** Lifecycle state machine (dormant -> materializing -> idle -> ...). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Chess|Lifecycle")
    TObjectPtr<UChessEntityLifecycleComponent> Lifecycle;

    /** Algebraic board square this entity currently occupies, e.g. "e4". Empty = off-board. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Chess|Board")
    FString BoardSquare;

    /** Full configuration pass from a DNA block. Safe to call at spawn time or in-editor. */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess|DNA")
    void InitializeFromDNA(const FCharacterDNA& InDNA);

    /**
     * Validates the DNA block against pipeline law:
     *  - required IDs present, asset names honor the naming rules
     *  - MetaHuman face use requires Tumbo's approval flag
     *  - centaur architecture only legal on knights
     */
    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess|DNA")
    bool ValidateDNA(FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "Cinematic Chess|Board")
    void SetBoardSquare(const FString& SquareAlgebraic);

    UFUNCTION(BlueprintPure, Category = "Cinematic Chess|DNA")
    EChessFaction GetFaction() const { return CharacterDNA.Faction; }

    UFUNCTION(BlueprintPure, Category = "Cinematic Chess|DNA")
    EChessPieceType GetPieceType() const { return CharacterDNA.PieceType; }

    UPROPERTY(BlueprintAssignable, Category = "Cinematic Chess|DNA")
    FOnChessEntityDNAApplied OnDNAApplied;

protected:
    virtual void BeginPlay() override;

    /** Obsidian + gold filigree (Black) vs ivory + gold (White), dark reflective board. */
    void ApplyFactionMaterials();

    /** Per-piece body plan: centaur knights, castle-staff rooks, feminine queens, armored forms. */
    void ConfigurePieceArchitecture();

    /** MetaHuman face binding. Face-bearing assets require Tumbo's approval. */
    void BindMetaHumanFace();

    /** WeaponAttachmentID -> socket attach (e.g. MassiveCastleStaff on rooks). */
    void AttachWeaponProp();

    /** ArmorSetID -> skeletal mesh / armor set swap. */
    void ApplyArmorSet();

private:
    /** Face profile ID approved by Tumbo for his likeness on male pieces. */
    static const FString ApprovedTumboFaceProfileID;
};
