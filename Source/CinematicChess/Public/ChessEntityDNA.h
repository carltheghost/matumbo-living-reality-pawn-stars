#pragma once

#include "CoreMinimal.h"
// FCharacterDNA, EChessFaction, EChessPieceType are owned by Elisa's PR #2.
// This file EXTENDS that schema — it never redeclares it.
#include "CharacterDNA.h"
#include "ChessEntityTypes.h"
#include "ChessEntityDNA.generated.h"

// Phase 0 entity-layer spec. Wraps the PR #2 character DNA with everything
// the runtime entity needs: movement identity, armor/weapon binding, LOD
// policy, idle-motion set, and board-square binding.
//
// Design law honored here:
// - Knights: GallopCharge REQUIRES bIsCentaurArchitecture on the DNA.
// - Rooks: castle staff binds to StaffMount (WPN_Rook_MassiveCastleStaff).
// - Queens: feminine identity — SwayHover, no weapon socket.
// - Male human-headed pieces: bUsesMetaHumanFace may only be true after
//   Tumbo approves the face-bearing asset (see MetaHumanPipelineTestActor).
USTRUCT(BlueprintType)
struct FChessEntitySpec
{
    GENERATED_BODY()

    // Core identity — owned by the PR #2 schema.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FCharacterDNA CharacterDNA;

    // Locomotion identity. Defaults are resolved per piece type by
    // ResolveMovementArchetype() — only override for variants.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion")
    EChessMovementArchetype MovementArchetype = EChessMovementArchetype::MarchStep;

    // Armor pieces bound per slot. Values are asset IDs and must validate
    // against CinematicChessAssetRules (ARM_ prefix).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    TMap<EChessArmorSlot, FString> ArmorBindings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EChessWeaponSocket WeaponSocket = EChessWeaponSocket::None;

    // Weapon asset ID (WPN_ prefix). Empty when WeaponSocket is None.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FString WeaponAssetID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
    EChessEntityLOD DefaultLOD = EChessEntityLOD::Cinematic;

    // Board binding: file 'a'..'h' + rank 1..8 (e.g. "e4"). Empty = unplaced.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
    FString BoardSquare;

    // Idle motion set shared with the avatar rig: sway, blink, hover pulse.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion")
    FString IdleMotionSetID;

    // Fully resolved asset name, CHR_[Faction]_[Piece]_[Variant].
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Identity")
    FString ResolvedAssetName;
};

// Helpers that keep every entity consistent with the locked design.
namespace CinematicChessEntityDNA
{
    // Canonical movement identity per piece. Throws the design law into code:
    // a knight that is not centaur-configured cannot GallopCharge.
    inline EChessMovementArchetype ResolveMovementArchetype(EChessPieceType Piece)
    {
        switch (Piece)
        {
        case EChessPieceType::King:   return EChessMovementArchetype::RegalGlide;
        case EChessPieceType::Queen:  return EChessMovementArchetype::SwayHover;
        case EChessPieceType::Bishop: return EChessMovementArchetype::DiagonalDrift;
        case EChessPieceType::Knight: return EChessMovementArchetype::GallopCharge;
        case EChessPieceType::Rook:   return EChessMovementArchetype::SiegeAdvance;
        case EChessPieceType::Pawn:
        default:                      return EChessMovementArchetype::MarchStep;
        }
    }

    // Default armor loadout per piece. Queens skip the helm (feminine identity,
    // hair and face stay visible); knights add the horse-body barding via Crest.
    inline TMap<EChessArmorSlot, FString> DefaultArmorBindings(EChessPieceType Piece, EChessFaction Faction)
    {
        const FString FactionTag = (Faction == EChessFaction::White) ? TEXT("White") : TEXT("Black");
        TMap<EChessArmorSlot, FString> Out;
        Out.Add(EChessArmorSlot::Torso,     FString::Printf(TEXT("ARM_%s_Torso_FullPlate"), *FactionTag));
        Out.Add(EChessArmorSlot::Pauldrons, FString::Printf(TEXT("ARM_%s_Pauldrons_War"), *FactionTag));
        Out.Add(EChessArmorSlot::Gauntlets, FString::Printf(TEXT("ARM_%s_Gauntlets_War"), *FactionTag));
        Out.Add(EChessArmorSlot::Greaves,   FString::Printf(TEXT("ARM_%s_Greaves_War"), *FactionTag));
        Out.Add(EChessArmorSlot::Cape,      FString::Printf(TEXT("ARM_%s_Cape_Cinematic"), *FactionTag));
        if (Piece != EChessPieceType::Queen)
        {
            Out.Add(EChessArmorSlot::Helm, FString::Printf(TEXT("ARM_%s_Helm_War"), *FactionTag));
        }
        if (Piece == EChessPieceType::Knight)
        {
            Out.Add(EChessArmorSlot::Crest, FString::Printf(TEXT("ARM_%s_Barding_Centaur"), *FactionTag));
        }
        return Out;
    }

    // Weapon socket + asset per piece. Only rooks take the castle staff.
    inline void ResolveWeapon(EChessPieceType Piece, EChessWeaponSocket& OutSocket, FString& OutAssetID)
    {
        switch (Piece)
        {
        case EChessPieceType::Rook:
            OutSocket = EChessWeaponSocket::StaffMount;
            OutAssetID = TEXT("WPN_Rook_MassiveCastleStaff");
            break;
        case EChessPieceType::Knight:
            OutSocket = EChessWeaponSocket::RightHand;
            OutAssetID = TEXT("WPN_Knight_Lance_Obsidian");
            break;
        case EChessPieceType::Bishop:
            OutSocket = EChessWeaponSocket::RightHand;
            OutAssetID = TEXT("WPN_Bishop_Crosier_Gold");
            break;
        case EChessPieceType::Pawn:
            OutSocket = EChessWeaponSocket::RightHand;
            OutAssetID = TEXT("WPN_Pawn_Shortblade");
            break;
        case EChessPieceType::King:
            OutSocket = EChessWeaponSocket::RightHand;
            OutAssetID = TEXT("WPN_King_Scepter");
            break;
        case EChessPieceType::Queen:
        default:
            OutSocket = EChessWeaponSocket::None;
            OutAssetID.Empty();
            break;
        }
    }

    // Builds CHR_[Faction]_[Piece]_[Variant], e.g. CHR_Black_Knight_Centaur01.
    inline FString ResolveAssetName(const FCharacterDNA& DNA, const FString& Variant = TEXT("01"))
    {
        const TCHAR* FactionStr = (DNA.Faction == EChessFaction::White) ? TEXT("White") : TEXT("Black");
        const TCHAR* PieceStr = TEXT("Pawn");
        switch (DNA.PieceType)
        {
        case EChessPieceType::King:   PieceStr = TEXT("King");   break;
        case EChessPieceType::Queen:  PieceStr = TEXT("Queen");  break;
        case EChessPieceType::Bishop: PieceStr = TEXT("Bishop"); break;
        case EChessPieceType::Knight: PieceStr = TEXT("Knight"); break;
        case EChessPieceType::Rook:   PieceStr = TEXT("Rook");   break;
        default: break;
        }
        return FString::Printf(TEXT("CHR_%s_%s_%s"), FactionStr, PieceStr, *Variant);
    }
}
