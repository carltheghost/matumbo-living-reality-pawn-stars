#include "CinematicChessCharacterDNA.h"

namespace
{
	FString FactionToken(EChessFaction Faction)
	{
		return Faction == EChessFaction::Black ? TEXT("Black") : TEXT("White");
	}

	FString PieceToken(EChessPieceType PieceType)
	{
		switch (PieceType)
		{
		case EChessPieceType::King:   return TEXT("King");
		case EChessPieceType::Queen:  return TEXT("Queen");
		case EChessPieceType::Bishop: return TEXT("Bishop");
		case EChessPieceType::Knight: return TEXT("Knight");
		case EChessPieceType::Rook:   return TEXT("Rook");
		case EChessPieceType::Pawn:   return TEXT("Pawn");
		default:                      return TEXT("Unknown");
		}
	}
}

FString UCinematicChessDNALibrary::DefaultMaterialSetID(EChessFaction Faction)
{
	// Locked material law: black obsidian + gold filigree vs white ivory + gold.
	return Faction == EChessFaction::Black
		? TEXT("MAT_Black_ObsidianGoldFiligree")
		: TEXT("MAT_White_IvoryGold");
}

FString UCinematicChessDNALibrary::DefaultArmorSetID(EChessFaction Faction, EChessPieceType PieceType)
{
	const FString F = FactionToken(Faction);
	const FString P = PieceToken(PieceType);
	if (PieceType == EChessPieceType::Knight)
	{
		// Centaur architecture: warrior torso + horse body + armor sockets.
		return FString::Printf(TEXT("ARM_%s_Knight_CentaurHarness"), *F);
	}
	if (PieceType == EChessPieceType::King)
	{
		return FString::Printf(TEXT("ARM_%s_King_RoyalPlate"), *F);
	}
	if (PieceType == EChessPieceType::Queen)
	{
		return FString::Printf(TEXT("ARM_%s_Queen_RegalPlate"), *F);
	}
	return FString::Printf(TEXT("ARM_%s_%s_WarPlate"), *F, *P);
}

FString UCinematicChessDNALibrary::DefaultWeaponAttachmentID(EChessPieceType PieceType)
{
	switch (PieceType)
	{
	case EChessPieceType::King:   return TEXT("WPN_King_WarScepter");
	case EChessPieceType::Queen:  return TEXT("WPN_Queen_DuelBlades");
	case EChessPieceType::Bishop: return TEXT("WPN_Bishop_CrozierStaff");
	case EChessPieceType::Knight: return TEXT("WPN_Knight_Lance");
	case EChessPieceType::Rook:   return TEXT("WPN_Rook_MassiveCastleStaff");
	case EChessPieceType::Pawn:   return TEXT("WPN_Pawn_ShortSword");
	default:                      return TEXT("WPN_None");
	}
}

FString UCinematicChessDNALibrary::DefaultFaceProfileID(EChessFaction Faction, EChessPieceType PieceType)
{
	// Face law: Tumbo's approved male face on kings only, and NEVER in a
	// public face-bearing asset without his explicit approval. Queens carry
	// a separate feminine identity. All other roles are helm/armor.
	if (PieceType == EChessPieceType::King)
	{
		// PENDING_APPROVAL: do not bake into any shippable asset until Tumbo signs off.
		return TEXT("FACE_Tumbo_Approved_Male_PENDING_APPROVAL");
	}
	if (PieceType == EChessPieceType::Queen)
	{
		return TEXT("FACE_Feminine_Regal");
	}
	return TEXT("FACE_None_ArmoredHelm");
}

FCharacterDNA UCinematicChessDNALibrary::MakePieceDNA(EChessFaction Faction, EChessPieceType PieceType)
{
	FCharacterDNA DNA;
	const FString F = FactionToken(Faction);
	const FString P = PieceToken(PieceType);

	DNA.CharacterID = FString::Printf(TEXT("CHR_%s_%s"), *F, *P);
	DNA.Faction = Faction;
	DNA.PieceType = PieceType;
	DNA.FaceProfileID = DefaultFaceProfileID(Faction, PieceType);
	DNA.ArmorSetID = DefaultArmorSetID(Faction, PieceType);
	DNA.MaterialSetID = DefaultMaterialSetID(Faction);
	DNA.WeaponAttachmentID = DefaultWeaponAttachmentID(PieceType);
	DNA.bUsesMetaHumanFace = (PieceType == EChessPieceType::King || PieceType == EChessPieceType::Queen);
	DNA.bIsCentaurArchitecture = (PieceType == EChessPieceType::Knight);
	return DNA;
}

TArray<FCharacterDNA> UCinematicChessDNALibrary::MakeFullSet()
{
	TArray<FCharacterDNA> Set;
	Set.Reserve(12);
	const EChessPieceType Roles[6] =
	{
		EChessPieceType::King, EChessPieceType::Queen, EChessPieceType::Bishop,
		EChessPieceType::Knight, EChessPieceType::Rook, EChessPieceType::Pawn
	};
	for (EChessFaction Faction : { EChessFaction::White, EChessFaction::Black })
	{
		for (EChessPieceType Role : Roles)
		{
			Set.Add(MakePieceDNA(Faction, Role));
		}
	}
	return Set;
}
