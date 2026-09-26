# Character DNA Schema — Entity Framework Binding

## Canonical schema (Elisa's PR #2, merged)

`FCharacterDNA` is defined ONCE, in `Source/CinematicChess/Public/CharacterDNA.h`.
Nothing in the entity framework redeclares it. The old competing struct from
this PR's first version (`Public/Framework/CharacterDNA.h`) has been deleted.

| Field | Type | Meaning |
|---|---|---|
| CharacterID | FString | Unique ID, must honor `CHR_` asset-naming prefix |
| Faction | EChessFaction | White / Black — drives material set |
| PieceType | EChessPieceType | King, Queen, Bishop, Knight, Rook, Pawn — drives body plan |
| FaceProfileID | FString | MetaHuman face asset; `FACE_TUMBO_APPROVED` = Tumbo's likeness (approval-gated) |
| ArmorSetID | FString | `ARM_` armor set swap |
| MaterialSetID | FString | `MAT_` override; empty = faction default |
| WeaponAttachmentID | FString | `WPN_` prop attach (rooks: `MassiveCastleStaff`) |
| bUsesMetaHumanFace | bool | Enables MetaHuman face binding |
| bIsCentaurArchitecture | bool | Knight-only body plan (validated) |

## How the entity framework consumes each field

- `ACinematicChessEntity::InitializeFromDNA` stores the block and runs the full
  visual configuration: `ApplyFactionMaterials` → `ConfigurePieceArchitecture`
  → `ApplyArmorSet` → `AttachWeaponProp` → `BindMetaHumanFace`.
- `ValidateDNA` enforces pipeline law: naming prefixes (via Elisa's
  `CinematicChessAssetRules`), centaur-only-on-knights, and the MetaHuman
  face-approval rule from `Pipeline/MetaHuman/README.md`.
- `UCinematicChessEntityFactory::SpawnFullSet` consumes 32 DNA blocks and
  places the regulation army (1K/1Q/2B/2N/2R/8P per faction) on its starting
  squares; `ValidateRoster` rejects bad rosters before anything spawns.
- `UChessEntityLifecycleComponent` owns the on-board state machine
  (Dormant → Materializing → Idle → Selected/Moving/Engaged → Captured → Removed)
  with illegal transitions rejected and logged.

## File map

- `Source/CinematicChess/Public/Framework/CinematicChessEntity.h` — the entity class
- `Source/CinematicChess/Private/Framework/CinematicChessEntity.cpp` — DNA-driven visuals
- `Source/CinematicChess/Public/Framework/ChessEntityLifecycle.h` — lifecycle component
- `Source/CinematicChess/Private/Framework/ChessEntityLifecycle.cpp` — state machine
- `Source/CinematicChess/Public/Framework/ChessEntityTypes.h` — board coordinates, capture styles
- `Source/CinematicChess/Public/Framework/ChessEntityFactory.h` — spawner
- `Source/CinematicChess/Private/Framework/ChessEntityFactory.cpp` — roster validation + placement
