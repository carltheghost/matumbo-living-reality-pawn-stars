#pragma once

#include "CoreMinimal.h"
#include "ChessEntityTypes.generated.h"

// Armor attachment slots shared by all six cinematic chess roles.
// Skeleton socket names must follow: SKT_Armor_<SlotName> (e.g. SKT_Armor_Pauldrons).
// Filigree trim is part of every slot's material set — never a separate mesh.
UENUM(BlueprintType)
enum class EChessArmorSlot : uint8
{
    Helm,
    Pauldrons,
    Torso,
    Gauntlets,
    Greaves,
    Cape,
    Crest
};

// Weapon hardpoints. The rook's castle staff binds to StaffMount;
// all other armed pieces use RightHand. Queens are unarmed by design.
UENUM(BlueprintType)
enum class EChessWeaponSocket : uint8
{
    None,
    RightHand,
    LeftHand,
    BackMount,
    StaffMount
};

// Locomotion identity per piece. This is the motion language Tumbo approved:
// pieces move the way his avatar moves — idle sway, glow pulse, glide-move.
// No wheeled, no sliding-chess-piece, no cartoon bounce. Ever.
UENUM(BlueprintType)
enum class EChessMovementArchetype : uint8
{
    RegalGlide,    // King:   slow, deliberate hover-glide
    SwayHover,     // Queen:  feminine hover with lateral sway
    DiagonalDrift, // Bishop: smooth diagonal slide, cloak trailing
    GallopCharge,  // Knight: centaur gallop (requires bIsCentaurArchitecture)
    SiegeAdvance,  // Rook:   heavy grounded advance, staff planted
    MarchStep      // Pawn:   infantry march
};

// Detail policy. Cinematic is the only LOD allowed in trailers and
// the Arena showcase; Gameplay/Distant are perf fallbacks, not art targets.
UENUM(BlueprintType)
enum class EChessEntityLOD : uint8
{
    Cinematic,
    Gameplay,
    Distant
};

// Phase 0 MetaHuman validation stages. The pipeline test actor walks these
// in order; a stage that fails blocks every later stage.
UENUM(BlueprintType)
enum class EMetaHumanTestStage : uint8
{
    NotStarted,
    Import,
    FaceVerify,
    MaterialVerify,
    HairVerify,
    ArmorAttach,
    LightingVerify,
    AwaitingTumboApproval,
    Approved,
    Failed
};
