# Character DNA

Character DNA is the stable identity record for each of the twelve cinematic chess pieces. The Cinematic Chess Entity Framework reads a piece's DNA at spawn time and resolves its face, armor, materials, weapon, and architecture flags. DNA is data, not geometry — the same DNA drives the Blender authoring side and the UE5.5.4 runtime side.

Schema: `Source/Schema/CinematicChessCharacterDNA.json` (version 0.2). Twelve entries: `CHR_{Black|White}_{King|Queen|Bishop|Knight|Rook|Pawn}_01`.

## DNA fields

- `id` — stable character ID, follows the asset naming rules (`CHR_[Faction]_[Piece]_[Variant]`).
- `pieceType` / `faction` — King, Queen, Bishop, Knight, Rook, Pawn; Black or White.
- `faceProfileID` — which face identity this piece wears; resolved through the MetaHuman facial pipeline.
- `usesMetaHumanFace` — true for all twelve; every face is a MetaHuman face, never sculpted cartoon geometry.
- `armorSetID` — armor set, `ARM_[Faction]_[Set]` (e.g. `ARM_Black_SovereignPlate`).
- `materialSetID` — material set, `MAT_[Faction]_[Surface]` (e.g. `MAT_Black_ObsidianGold`).
- `weaponAttachmentID` — weapon, `WPN_[Piece]_[Name]`, bound to `Socket_Weapon_R` (rooks: `Socket_CastleStaff`).
- `isCentaurArchitecture` — true only for knights; when true, `centaurSpec` defines torso, helm, body, and locomotion.
- `scale` — board-relative silhouette scale (pawn 0.95 → king 1.15, knight 1.2 with horse body).

## Male human likeness rules

Male human-headed pieces (king, bishop, rook, pawn, and the knight's warrior torso) use Tumbo's approved face reference `FACE_Tumbo_Male_01` through the MetaHuman facial pipeline. Rules:

1. The face reference is the single approved likeness — no invented male faces.
2. The MetaHuman head sits under the helm at all times (`Socket_Helm`); helms never replace the head.
3. **No face-bearing asset ships publicly without Tumbo's explicit approval.** This includes screenshots, videos, and demo builds.

## Queen identity rules

Queens carry a separate feminine MetaHuman identity, `FACE_Feminine_Regal_01`:

1. Queens never use the male face reference.
2. Feminine armor silhouette: regal plate, narrower pauldrons, war-fan blades (`WPN_Queen_WarFanBlades`).
3. Same faction material law as the rest of the army — obsidian + gold filigree (black) or ivory + gold (white).

## Centaur knight rules

Knights are full centaurs, flagged `isCentaurArchitecture: true`:

1. Humanoid warrior torso wearing the male face reference under a horse-head helm (`HorseHeadHelm_Obsidian` / `HorseHeadHelm_Ivory`).
2. Full horse body with armored barding — not a horse head on a human body.
3. Quadruped locomotion: dedicated blend space (`QuadrupedIdle`, `QuadrupedGallop`); the knight rears on capture strike.
4. Barding sockets `Socket_Barding_Front` / `Socket_Barding_Hind_Equine` carry the armor plates.

## Photoreal bar

Every DNA entry must resolve to photoreal PBR assets. Anything that reads cartoon — toon shading, stylized proportions, dream-render geometry — fails the DNA and goes back.
