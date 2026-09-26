#include "CinematicChessEntityComponent.h"
#include "AssetNamingRules.h"
#include "Components/SkeletalMeshComponent.h"

UCinematicChessEntityComponent::UCinematicChessEntityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;
}

void UCinematicChessEntityComponent::BeginPlay()
{
    Super::BeginPlay();

    // Auto-initialize when a spec was authored in the editor.
    if (!bInitialized && !EntitySpec.ResolvedAssetName.IsEmpty())
    {
        InitializeFromSpec(EntitySpec);
    }
}

bool UCinematicChessEntityComponent::InitializeFromSpec(const FChessEntitySpec& Spec)
{
    EntitySpec = Spec;

    // Design-law enforcement: a knight that is not centaur-configured
    // cannot take the GallopCharge archetype. Fail loudly, not silently.
    if (EntitySpec.CharacterDNA.PieceType == EChessPieceType::Knight
        && EntitySpec.MovementArchetype == EChessMovementArchetype::GallopCharge
        && !EntitySpec.CharacterDNA.bIsCentaurArchitecture)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CinematicChess] Knight '%s' requests GallopCharge without bIsCentaurArchitecture. Fix the DNA."),
            *EntitySpec.CharacterDNA.CharacterID);
        OnEntityInitialized.Broadcast(false);
        return false;
    }

    // Face gate: no MetaHuman face may go live without Tumbo's approval.
    // The approval is recorded by the MetaHuman pipeline test actor; here we
    // only require that the flag is not set silently on unapproved assets.
    if (EntitySpec.CharacterDNA.bUsesMetaHumanFace && EntitySpec.CharacterDNA.FaceProfileID.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CinematicChess] '%s' sets bUsesMetaHumanFace with an empty FaceProfileID."),
            *EntitySpec.CharacterDNA.CharacterID);
        OnEntityInitialized.Broadcast(false);
        return false;
    }

    EntitySpec.ResolvedAssetName =
        CinematicChessEntityDNA::ResolveAssetName(EntitySpec.CharacterDNA);

    FString NameError;
    if (!ValidateAssetNaming(NameError))
    {
        UE_LOG(LogTemp, Error, TEXT("[CinematicChess] Asset naming invalid: %s"), *NameError);
        OnEntityInitialized.Broadcast(false);
        return false;
    }

    const bool bOk = ApplyArmorBindings() && ApplyWeaponBinding() && ApplyMovementArchetype();
    bInitialized = bOk;
    OnEntityInitialized.Broadcast(bOk);
    return bOk;
}

bool UCinematicChessEntityComponent::BindToBoardSquare(const FString& Square)
{
    if (!IsValidBoardSquare(Square))
    {
        UE_LOG(LogTemp, Warning, TEXT("[CinematicChess] Bad square notation '%s' (want e.g. 'e4')."), *Square);
        return false;
    }
    EntitySpec.BoardSquare = Square;
    return true;
}

void UCinematicChessEntityComponent::PlayIdleMotion()
{
    // Idle motion set is data-driven (Animation Blueprint reads IdleMotionSetID).
    // The component only guarantees the set ID is present before play.
    if (EntitySpec.IdleMotionSetID.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CinematicChess] '%s' has no IdleMotionSetID; idle motion skipped."),
            *EntitySpec.ResolvedAssetName);
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("[CinematicChess] '%s' playing idle set '%s'."),
        *EntitySpec.ResolvedAssetName, *EntitySpec.IdleMotionSetID);
}

bool UCinematicChessEntityComponent::ValidateAssetNaming(FString& OutError) const
{
    using namespace CinematicChessAssetRules;

    if (!ValidateCharacterName(EntitySpec.ResolvedAssetName))
    {
        OutError = FString::Printf(TEXT("Character name '%s' must start with CHR_"),
            *EntitySpec.ResolvedAssetName);
        return false;
    }
    for (const auto& Pair : EntitySpec.ArmorBindings)
    {
        if (!ValidateArmorName(Pair.Value))
        {
            OutError = FString::Printf(TEXT("Armor ID '%s' must start with ARM_"), *Pair.Value);
            return false;
        }
    }
    if (!EntitySpec.WeaponAssetID.IsEmpty() && !ValidateWeaponName(EntitySpec.WeaponAssetID))
    {
        OutError = FString::Printf(TEXT("Weapon ID '%s' must start with WPN_"), *EntitySpec.WeaponAssetID);
        return false;
    }
    return true;
}

bool UCinematicChessEntityComponent::ApplyArmorBindings()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }
    USkeletalMeshComponent* Skel = Owner->FindComponentByClass<USkeletalMeshComponent>();
    for (const auto& Pair : EntitySpec.ArmorBindings)
    {
        const FString Socket = ArmorSocketName(Pair.Key);
        // Phase 0: verify the socket exists; the actual attach is authored
        // in the character Blueprint so artists keep control of transforms.
        if (Skel && !Skel->DoesSocketExist(*Socket))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[CinematicChess] '%s': skeleton missing armor socket '%s' for %s."),
                *EntitySpec.ResolvedAssetName, *Socket, *Pair.Value);
        }
    }
    return true;
}

bool UCinematicChessEntityComponent::ApplyWeaponBinding()
{
    if (EntitySpec.WeaponSocket == EChessWeaponSocket::None)
    {
        return true; // Queens and unarmed variants: nothing to bind.
    }
    if (EntitySpec.WeaponAssetID.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CinematicChess] '%s': weapon socket set but WeaponAssetID empty."),
            *EntitySpec.ResolvedAssetName);
        return false;
    }
    UE_LOG(LogTemp, Log, TEXT("[CinematicChess] '%s': weapon '%s' -> socket '%s'."),
        *EntitySpec.ResolvedAssetName, *EntitySpec.WeaponAssetID,
        *WeaponSocketName(EntitySpec.WeaponSocket));
    return true;
}

bool UCinematicChessEntityComponent::ApplyMovementArchetype()
{
    // The archetype drives which locomotion Animation Blueprint the piece uses.
    // Phase 0 records the choice; the ABP wiring lands with the sculpt pass.
    UE_LOG(LogTemp, Log, TEXT("[CinematicChess] '%s': movement archetype %d."),
        *EntitySpec.ResolvedAssetName, static_cast<int32>(EntitySpec.MovementArchetype));
    return true;
}

FString UCinematicChessEntityComponent::ArmorSocketName(EChessArmorSlot Slot)
{
    const TCHAR* Name = TEXT("Torso");
    switch (Slot)
    {
    case EChessArmorSlot::Helm:      Name = TEXT("Helm");      break;
    case EChessArmorSlot::Pauldrons: Name = TEXT("Pauldrons"); break;
    case EChessArmorSlot::Torso:     Name = TEXT("Torso");     break;
    case EChessArmorSlot::Gauntlets: Name = TEXT("Gauntlets"); break;
    case EChessArmorSlot::Greaves:   Name = TEXT("Greaves");   break;
    case EChessArmorSlot::Cape:      Name = TEXT("Cape");      break;
    case EChessArmorSlot::Crest:     Name = TEXT("Crest");     break;
    }
    return FString::Printf(TEXT("SKT_Armor_%s"), Name);
}

FString UCinematicChessEntityComponent::WeaponSocketName(EChessWeaponSocket Socket)
{
    const TCHAR* Name = TEXT("None");
    switch (Socket)
    {
    case EChessWeaponSocket::RightHand: Name = TEXT("RightHand"); break;
    case EChessWeaponSocket::LeftHand:  Name = TEXT("LeftHand");  break;
    case EChessWeaponSocket::BackMount: Name = TEXT("BackMount"); break;
    case EChessWeaponSocket::StaffMount: Name = TEXT("StaffMount"); break;
    default: break;
    }
    return FString::Printf(TEXT("SKT_Weapon_%s"), Name);
}

bool UCinematicChessEntityComponent::IsValidBoardSquare(const FString& Square)
{
    if (Square.Len() != 2)
    {
        return false;
    }
    const TCHAR File = FChar::ToLower(Square[0]);
    const TCHAR Rank = Square[1];
    return File >= TEXT('a') && File <= TEXT('h') && Rank >= TEXT('1') && Rank <= TEXT('8');
}
