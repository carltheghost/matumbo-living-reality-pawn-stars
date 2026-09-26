# Phase 0 Architecture — Piece Entity Framework

Phase 0 builds the foundation everything else stands on: the data schema, the
runtime entity, and the pipelines that feed them. No characters are sculpted
in Phase 0; the framework must exist first so sculpting has something true
to plug into.

## Layers

```
Blender 4.x  --FBX-->  UE 5.5.4 Content/
                                |
MetaHuman plugin --> face rigs -+
                                |
Character DNA (data)            v
  FCharacterDNA ................ UCinematicChessEntityComponent (runtime)
  Public/CharacterDNA.h          Character/CinematicChessEntityComponent.*
  (Elisa, merged)                validates DNA, exposes faction/role/
                                 sockets/material queries
                                |
                                v
                      ACharacter / AActor pieces
                      (Phase 1: Character system)
```

## File map (this rework + merged #2)

| Path | Owner | Contents |
|---|---|---|
| `Source/CinematicChess/Public/CharacterDNA.h` | Elisa (merged) | `FCharacterDNA` USTRUCT, `EChessFaction`, `EChessPieceType` |
| `Source/CinematicChess/Public/AssetNamingRules.h` | Elisa (merged) | `CHR_`/`ARM_`/`WPN_`/`MAT_` prefix validators |
| `Source/CinematicChess/Character/CinematicChessCharacterDNA.h/.cpp` | Elias | `UCinematicChessDNALibrary`: canonical DNA factory for all 12 warriors |
| `Source/CinematicChess/Character/CinematicChessEntityComponent.h/.cpp` | Elias | Runtime component: carries DNA, validates it, exposes sockets/flags |
| `Content/CinematicChess/Data/CharacterDNA_Default.json` | Elisa (merged) | Default DNA data asset |
| `Content/Documentation/*_Phase0.md` | Elias | DNA spec, MetaHuman test plan, this architecture doc |
| `Pipeline/Blender/CinematicChess_Export_Checklist.md` | Elisa (merged) | Blender -> FBX export rules for UE 5.5.4 |
| `Pipeline/MetaHuman/README.md` | Elisa (merged) | MetaHuman pipeline notes + approval gate |

## Rules the code enforces

- `FCharacterDNA` is declared exactly once (Elisa's header). Nothing
  redeclares it — a duplicate struct would break the build.
- Character IDs must match `CHR_<Faction>_<Piece>` (validated at runtime).
- Centaur architecture flag is knight-only; MetaHuman face flag is
  king/queen-only. Violations fail validation loudly, not silently.
- Face assets carrying `PENDING_APPROVAL` are blocked from public builds by
  `IsFaceAssetApprovalPending()`.

## What Phase 0 does NOT do

No sculpting, no animation Blueprints, no AI, no board/game logic, no
renders presented as finished pieces. The first honest visual checkpoint
after Phase 0 is real UE5 viewport screenshots of DNA-driven test rigs —
never AI dream renders.
