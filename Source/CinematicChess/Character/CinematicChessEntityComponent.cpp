#include "CinematicChessEntityComponent.h"

UCinematicChessEntityComponent::UCinematicChessEntityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCinematicChessEntityComponent::BeginPlay()
{
	Super::BeginPlay();
	// Editor-placed pieces validate on spawn; runtime-spawned pieces call
	// InitializeFromDNA explicitly.
	bDNAValid = ValidateDNA();
	if (!bDNAValid)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[CinematicChess] %s has invalid CharacterDNA (ID='%s'). Assign DNA before play."),
			*GetOwner()->GetName(), *CharacterDNA.CharacterID);
	}
}

void UCinematicChessEntityComponent::InitializeFromDNA(const FCharacterDNA& InDNA)
{
	CharacterDNA = InDNA;
	bDNAValid = ValidateDNA();
	if (!bDNAValid)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CinematicChess] InitializeFromDNA rejected DNA (ID='%s')."), *InDNA.CharacterID);
	}
}

bool UCinematicChessEntityComponent::ValidateDNA()
{
	if (!HasValidCharacterID())
	{
		return false;
	}
	// DNA must name real asset sets; empty ids mean the factory was bypassed.
	if (CharacterDNA.ArmorSetID.IsEmpty() || CharacterDNA.MaterialSetID.IsEmpty())
	{
		return false;
	}
	// Centaur architecture is knight-only by design law.
	if (CharacterDNA.bIsCentaurArchitecture && CharacterDNA.PieceType != EChessPieceType::Knight)
	{
		return false;
	}
	// MetaHuman faces are king/queen-only by design law.
	if (CharacterDNA.bUsesMetaHumanFace &&
		CharacterDNA.PieceType != EChessPieceType::King &&
		CharacterDNA.PieceType != EChessPieceType::Queen)
	{
		return false;
	}
	return true;
}

bool UCinematicChessEntityComponent::HasValidCharacterID() const
{
	// Matches Elisa's asset naming rules: CHR_<Faction>_<Piece>.
	const FString& ID = CharacterDNA.CharacterID;
	if (!ID.StartsWith(TEXT("CHR_")))
	{
		return false;
	}
	return ID.Contains(TEXT("White")) || ID.Contains(TEXT("Black"));
}

FString UCinematicChessEntityComponent::GetWeaponSocketName() const
{
	const FString& W = CharacterDNA.WeaponAttachmentID;
	if (W.Contains(TEXT("Staff")))
	{
		return TEXT("Socket_Weapon_Staff");
	}
	if (W.Contains(TEXT("Scepter")))
	{
		return TEXT("Socket_Weapon_Scepter");
	}
	if (W.Contains(TEXT("Blades")) || W.Contains(TEXT("Sword")))
	{
		return TEXT("Socket_Weapon_Hand");
	}
	if (W.Contains(TEXT("Lance")))
	{
		return TEXT("Socket_Weapon_Lance");
	}
	return TEXT("Socket_Weapon_Hand");
}

bool UCinematicChessEntityComponent::IsFaceAssetApprovalPending() const
{
	// Any face profile still marked pending blocks public face-bearing builds.
	return CharacterDNA.bUsesMetaHumanFace &&
		CharacterDNA.FaceProfileID.Contains(TEXT("PENDING_APPROVAL"));
}
