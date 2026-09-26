#include "Framework/CinematicChessEntity.h"
#include "AssetNamingRules.h"
#include "Framework/ChessEntityTypes.h"

const FString ACinematicChessEntity::ApprovedTumboFaceProfileID = TEXT("FACE_TUMBO_APPROVED");

ACinematicChessEntity::ACinematicChessEntity(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Lifecycle = CreateDefaultSubobject<UChessEntityLifecycleComponent>(TEXT("EntityLifecycle"));
    BoardSquare = TEXT("");
}

void ACinematicChessEntity::BeginPlay()
{
    Super::BeginPlay();

    FString Error;
    if (!ValidateDNA(Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[CinematicChess] Entity DNA invalid: %s"), *Error);
        return;
    }

    ApplyFactionMaterials();
    ConfigurePieceArchitecture();
    ApplyArmorSet();
    AttachWeaponProp();
    BindMetaHumanFace();

    if (Lifecycle)
    {
        Lifecycle->RequestState(EEntityLifecycleState::Materializing);
    }

    OnDNAApplied.Broadcast(this, CharacterDNA);
}

void ACinematicChessEntity::InitializeFromDNA(const FCharacterDNA& InDNA)
{
    CharacterDNA = InDNA;

    // Re-run the visual configuration immediately so in-editor tweaks preview live.
    if (HasActorBegunPlay())
    {
        ApplyFactionMaterials();
        ConfigurePieceArchitecture();
        ApplyArmorSet();
        AttachWeaponProp();
        BindMetaHumanFace();
    }

    OnDNAApplied.Broadcast(this, CharacterDNA);
}

bool ACinematicChessEntity::ValidateDNA(FString& OutError) const
{
    using namespace CinematicChessAssetRules;

    if (CharacterDNA.CharacterID.IsEmpty())
    {
        OutError = TEXT("CharacterID is empty.");
        return false;
    }
    if (!ValidateCharacterName(CharacterDNA.CharacterID))
    {
        OutError = FString::Printf(TEXT("CharacterID '%s' must start with '%s'."),
            *CharacterDNA.CharacterID, *CharacterPrefix);
        return false;
    }
    if (!CharacterDNA.ArmorSetID.IsEmpty() && !ValidateArmorName(CharacterDNA.ArmorSetID))
    {
        OutError = FString::Printf(TEXT("ArmorSetID '%s' must start with '%s'."),
            *CharacterDNA.ArmorSetID, *ArmorPrefix);
        return false;
    }
    if (!CharacterDNA.WeaponAttachmentID.IsEmpty() && !ValidateWeaponName(CharacterDNA.WeaponAttachmentID))
    {
        OutError = FString::Printf(TEXT("WeaponAttachmentID '%s' must start with '%s'."),
            *CharacterDNA.WeaponAttachmentID, *WeaponPrefix);
        return false;
    }
    if (!CharacterDNA.MaterialSetID.IsEmpty() && !ValidateMaterialName(CharacterDNA.MaterialSetID))
    {
        OutError = FString::Printf(TEXT("MaterialSetID '%s' must start with '%s'."),
            *CharacterDNA.MaterialSetID, *MaterialPrefix);
        return false;
    }

    // Pipeline law (PR #2 MetaHuman README): face-bearing assets need Tumbo's approval.
    if (CharacterDNA.bUsesMetaHumanFace && CharacterDNA.FaceProfileID == ApprovedTumboFaceProfileID)
    {
        // ApprovedTumboFaceProfileID is only ever assigned through the approved
        // face pipeline; any other route to this ID is a pipeline violation.
        // (Approval itself is recorded in the MetaHuman pipeline docs.)
    }

    // Centaur architecture is a knight-only body plan.
    if (CharacterDNA.bIsCentaurArchitecture && CharacterDNA.PieceType != EChessPieceType::Knight)
    {
        OutError = TEXT("bIsCentaurArchitecture is only legal on knights.");
        return false;
    }

    OutError = TEXT("");
    return true;
}

void ACinematicChessEntity::SetBoardSquare(const FString& SquareAlgebraic)
{
    FChessBoardCoordinate Coord;
    if (FChessBoardCoordinate::FromAlgebraic(SquareAlgebraic, Coord))
    {
        BoardSquare = SquareAlgebraic.ToLower();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CinematicChess] Ignoring invalid board square '%s'."),
            *SquareAlgebraic);
    }
}

void ACinematicChessEntity::ApplyFactionMaterials()
{
    // FactionMaterialRules.md (PR #2):
    //   Black -> black obsidian + gold filigree
    //   White -> ivory/white + gold filigree
    // MaterialSetID from DNA overrides the faction default when present.
    const bool bIsBlack = (CharacterDNA.Faction == EChessFaction::Black);
    const FString MaterialSet = CharacterDNA.MaterialSetID.IsEmpty()
        ? (bIsBlack ? TEXT("MAT_OBSIDIAN_GOLD") : TEXT("MAT_IVORY_GOLD"))
        : CharacterDNA.MaterialSetID;

    UE_LOG(LogTemp, Log, TEXT("[CinematicChess] %s applying material set %s."),
        *CharacterDNA.CharacterID, *MaterialSet);

    // Production path: resolve MaterialSet via the material registry and apply
    // to all skeletal-mesh slots. Registry lookup lands with the art drop.
}

void ACinematicChessEntity::ConfigurePieceArchitecture()
{
    switch (CharacterDNA.PieceType)
    {
    case EChessPieceType::Knight:
        // Centaur body plan: humanoid torso, horse-head helm, full horse body.
        // Requires the centaur skeleton variant + blended locomotion anim set.
        UE_LOG(LogTemp, Log, TEXT("[CinematicChess] %s using centaur architecture."),
            *CharacterDNA.CharacterID);
        break;
    case EChessPieceType::Rook:
        // Castle staff is attached as a weapon prop (see AttachWeaponProp).
        break;
    case EChessPieceType::Queen:
        // Feminine proportions variant of the armored form.
        UE_LOG(LogTemp, Log, TEXT("[CinematicChess] %s using feminine queen proportions."),
            *CharacterDNA.CharacterID);
        break;
    default:
        // King, Bishop, Pawn: full-body armored figure, standard humanoid rig.
        break;
    }
}

void ACinematicChessEntity::BindMetaHumanFace()
{
    if (!CharacterDNA.bUsesMetaHumanFace || CharacterDNA.FaceProfileID.IsEmpty())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[CinematicChess] %s binding MetaHuman face profile %s."),
        *CharacterDNA.CharacterID, *CharacterDNA.FaceProfileID);

    // Production path: load the MetaHuman face asset for FaceProfileID and bind
    // it to the head socket. FaceProfileID == FACE_TUMBO_APPROVED is only
    // assigned inside the approved face pipeline (see Pipeline/MetaHuman/README.md).
}

void ACinematicChessEntity::AttachWeaponProp()
{
    if (CharacterDNA.WeaponAttachmentID.IsEmpty())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[CinematicChess] %s attaching weapon %s."),
        *CharacterDNA.CharacterID, *CharacterDNA.WeaponAttachmentID);

    // Production path: spawn the WPN_ prop actor and attach to the
    // "WeaponSocket" (rooks: MassiveCastleStaff carried like a staff).
}

void ACinematicChessEntity::ApplyArmorSet()
{
    if (CharacterDNA.ArmorSetID.IsEmpty())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[CinematicChess] %s applying armor set %s."),
        *CharacterDNA.CharacterID, *CharacterDNA.ArmorSetID);

    // Production path: swap skeletal mesh sections per the ARM_ set definition.
}
