#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
// FCharacterDNA, EChessFaction, EChessPieceType come from Elisa's merged
// schema: Source/CinematicChess/Public/CharacterDNA.h (same module, Public
// include path). This library never redeclares them.
#include "CharacterDNA.h"
#include "CinematicChessCharacterDNA.generated.h"

/**
 * UCinematicChessDNALibrary — Phase 0 Character DNA factory (Elias).
 *
 * Builds the canonical FCharacterDNA for each of the twelve cinematic chess
 * warriors (six roles x two factions) from the locked design law:
 *  - Black army: obsidian armor + gold filigree, dark capes
 *  - White army: ivory armor + gold details, light capes
 *  - Knights: full centaur architecture (warrior torso, horse body, armor sockets)
 *  - Rooks: MassiveCastleStaff weapon attachment
 *  - Queens: separate feminine identity (never the male face profile)
 *  - Kings: MetaHuman face pipeline, Tumbo-approved male face profile.
 *    NO final face-bearing asset ships publicly without Tumbo's approval.
 *  - Board: dark reflective surface, cinematic lighting (see
 *    Content/CinematicChess/Materials/FactionMaterialRules.md)
 *
 * Photoreal bar: these DNA records describe realistic proportioned warriors.
 * Nothing cartoon, no stylized proportions.
 */
UCLASS()
class CINEMATICCHESS_API UCinematicChessDNALibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Material set for a faction: obsidian+gold filigree vs ivory+gold. */
	UFUNCTION(BlueprintPure, Category = "Cinematic Chess|DNA")
	static FString DefaultMaterialSetID(EChessFaction Faction);

	/** Armor set id for a faction + role, e.g. ARM_Black_Knight_CentaurHarness. */
	UFUNCTION(BlueprintPure, Category = "Cinematic Chess|DNA")
	static FString DefaultArmorSetID(EChessFaction Faction, EChessPieceType PieceType);

	/** Weapon attachment id for a role, e.g. WPN_Rook_MassiveCastleStaff. */
	UFUNCTION(BlueprintPure, Category = "Cinematic Chess|DNA")
	static FString DefaultWeaponAttachmentID(EChessPieceType PieceType);

	/** Face profile for a role. Kings use the Tumbo-approved male profile
	 *  (PENDING Tumbo approval before any public face-bearing asset);
	 *  queens use a separate feminine profile; other roles are helm/armor. */
	UFUNCTION(BlueprintPure, Category = "Cinematic Chess|DNA")
	static FString DefaultFaceProfileID(EChessFaction Faction, EChessPieceType PieceType);

	/** Full DNA record for one piece. */
	UFUNCTION(BlueprintPure, Category = "Cinematic Chess|DNA")
	static FCharacterDNA MakePieceDNA(EChessFaction Faction, EChessPieceType PieceType);

	/** All twelve warriors: White King/Queen/Bishop/Knight/Rook/Pawn then Black. */
	UFUNCTION(BlueprintPure, Category = "Cinematic Chess|DNA")
	static TArray<FCharacterDNA> MakeFullSet();
};
