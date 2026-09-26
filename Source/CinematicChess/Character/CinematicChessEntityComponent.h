#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
// FCharacterDNA comes from Elisa's merged schema:
// Source/CinematicChess/Public/CharacterDNA.h (same module Public include path).
#include "CharacterDNA.h"
#include "CinematicChessEntityComponent.generated.h"

/**
 * UCinematicChessEntityComponent — Phase 0 piece entity runtime (Elias).
 *
 * Attach to any ACharacter/AActor representing a chess warrior. It carries
 * the piece's FCharacterDNA (built by UCinematicChessDNALibrary), validates
 * it against the asset naming rules, and exposes the queries the animation,
 * material, and weapon systems need:
 *  - faction / role / centaur-architecture / MetaHuman-face flags
 *  - weapon socket name derived from the weapon attachment id
 *  - armor and material set ids for the material system
 *
 * Phase 0 scope: data carriage + validation only. No animation, no AI, no
 * board logic — those arrive in later phases.
 */
UCLASS(Blueprintable, ClassGroup = (CinematicChess), meta = (BlueprintSpawnableComponent))
class CINEMATICCHESS_API UCinematicChessEntityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCinematicChessEntityComponent();

	/** The DNA record driving this piece. Set in editor or via InitializeFromDNA. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Chess")
	FCharacterDNA CharacterDNA;

	/** True after InitializeFromDNA (or a valid editor-set DNA) passes validation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinematic Chess")
	bool bDNAValid = false;

	/** Assign DNA at runtime and validate it. Safe to call from BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Cinematic Chess")
	void InitializeFromDNA(const FCharacterDNA& InDNA);

	/** Re-validate the current CharacterDNA (useful after editor edits). */
	UFUNCTION(BlueprintCallable, Category = "Cinematic Chess")
	bool ValidateDNA();

	/** Socket the weapon attaches to, derived from the weapon id.
	 *  e.g. WPN_Rook_MassiveCastleStaff -> "Socket_Weapon_Staff". */
	UFUNCTION(BlueprintPure, Category = "Cinematic Chess")
	FString GetWeaponSocketName() const;

	UFUNCTION(BlueprintPure, Category = "Cinematic Chess")
	bool IsCentaur() const { return CharacterDNA.bIsCentaurArchitecture; }

	UFUNCTION(BlueprintPure, Category = "Cinematic Chess")
	bool UsesMetaHumanFace() const { return CharacterDNA.bUsesMetaHumanFace; }

	/** Face assets are gated: kings carry Tumbo's face profile, which must
	 *  never ship publicly without his explicit approval. */
	UFUNCTION(BlueprintPure, Category = "Cinematic Chess")
	bool IsFaceAssetApprovalPending() const;

protected:
	virtual void BeginPlay() override;

private:
	/** Naming-rule check: character ids must look like CHR_<Faction>_<Piece>. */
	bool HasValidCharacterID() const;
};
