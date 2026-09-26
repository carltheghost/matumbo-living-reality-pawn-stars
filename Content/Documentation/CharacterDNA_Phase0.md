# Cinematic Chess Character DNA — Phase 0

The Character DNA system is the single source of truth for what each of the
twelve warriors IS: faction, role, face pipeline, armor, materials, weapon,
and body architecture. Every downstream system (Blender export, MetaHuman
integration, material assignment, weapon sockets) reads DNA — nothing is
hard-coded per piece.

## Schema

Defined once in `Source/CinematicChess/Public/CharacterDNA.h` (Elisa, merged):

| Field | Type | Meaning |
|---|---|---|
| CharacterID | FString | `CHR_<Faction>_<Piece>`, e.g. `CHR_Black_Knight` |
| Faction | EChessFaction | White / Black |
| PieceType | EChessPieceType | King, Queen, Bishop, Knight, Rook, Pawn |
| FaceProfileID | FString | MetaHuman face source, or `FACE_None_ArmoredHelm` |
| ArmorSetID | FString | `ARM_<Faction>_<Piece>_<Set>` |
| MaterialSetID | FString | `MAT_Black_ObsidianGoldFiligree` or `MAT_White_IvoryGold` |
| WeaponAttachmentID | FString | `WPN_<Piece>_<Name>` |
| bUsesMetaHumanFace | bool | King/Queen only |
| bIsCentaurArchitecture | bool | Knight only |

Built canonically by `UCinematicChessDNALibrary::MakePieceDNA` /
`MakeFullSet` (`Source/CinematicChess/Character/CinematicChessCharacterDNA.*`).

## The twelve warriors

| Piece | Face | Armor | Weapon | Architecture |
|---|---|---|---|---|
| White King | MetaHuman, Tumbo-approved male (PENDING approval) | RoyalPlate | WarScepter | Humanoid warrior |
| Black King | MetaHuman, Tumbo-approved male (PENDING approval) | RoyalPlate | WarScepter | Humanoid warrior |
| White/Black Queen | MetaHuman, feminine regal (separate identity) | RegalPlate | DuelBlades | Humanoid warrior, feminine |
| White/Black Bishop | Armored helm | WarPlate | CrozierStaff | Humanoid warrior |
| White/Black Knight | Armored helm | CentaurHarness | Lance | **Centaur**: warrior torso, horse body, armor sockets |
| White/Black Rook | Armored helm | WarPlate | **MassiveCastleStaff** | Humanoid warrior |
| White/Black Pawn | Armored helm | WarPlate | ShortSword | Humanoid warrior, infantry |

## Design law (locked)

- Black army: obsidian armor + gold filigree, dark capes.
- White army: ivory armor + gold details, light capes.
- Board: dark reflective surface, cinematic lighting.
- Photoreal bar: realistic proportions on every piece. Nothing cartoon, no
  stylized dream renders. Anything cartoon goes back.
- Face-bearing assets: NOTHING public without Tumbo's explicit approval.
