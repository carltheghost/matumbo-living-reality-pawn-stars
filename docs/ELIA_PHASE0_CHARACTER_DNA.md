# Elia — Phase 0 Character DNA / Entity Framework (rework)

Target: Unreal Engine 5.5.4. Branch: `rebuild/cinematic-chess`.

## What this is

The entity layer of the cinematic chess framework. It sits ON TOP of Elisa's
PR #2 character DNA schema — it extends `FCharacterDNA`, never redefines it —
and turns a DNA record into a living board entity: movement identity, armor
and weapon binding, LOD policy, idle motion, and board-square placement.

## Files

- `Source/CinematicChess/Public/ChessEntityTypes.h` — shared enums: armor
  slots (`SKT_Armor_*` sockets), weapon hardpoints, movement archetypes,
  LOD policy, MetaHuman test stages.
- `Source/CinematicChess/Public/ChessEntityDNA.h` — `FChessEntitySpec`, the
  runtime spec that wraps `FCharacterDNA` with entity configuration, plus
  the `CinematicChessEntityDNA` helper namespace (movement resolution,
  default armor loadouts, weapon resolution, asset-name builder).
- `Source/CinematicChess/Public/CinematicChessEntityComponent.h` /
  `Source/CinematicChess/Private/CinematicChessEntityComponent.cpp` —
  `UCinematicChessEntityComponent`: applies a spec to an actor, validates
  asset naming against the PR #2 rules, binds armor/weapon sockets,
  enforces the design law in code.

## Design law, enforced in code (not just documented)

| Rule | Enforcement |
|---|---|
| Knights are centaurs | `GallopCharge` archetype requires `bIsCentaurArchitecture`; otherwise init fails with a log error |
| Rooks carry the castle staff | `ResolveWeapon()` binds `WPN_Rook_MassiveCastleStaff` to `StaffMount` |
| Queens are feminine, unarmed | No helm in default armor (hair/face visible), `SwayHover` motion, `WeaponSocket::None` |
| No cartoon, ever | Motion archetypes are glide/hover/drift/gallop — no bounce, no slide |
| Face approval gate | `bUsesMetaHumanFace` with an empty `FaceProfileID` fails init; shipping a face needs the MetaHuman test actor's `Approved` stage |

## Movement archetypes (Tumbo's motion language)

Pieces move the way his avatar moves — idle sway, glow pulse, glide-move:

- King `RegalGlide`, Queen `SwayHover`, Bishop `DiagonalDrift`,
  Knight `GallopCharge`, Rook `SiegeAdvance`, Pawn `MarchStep`.

## Asset naming

Built by `ResolveAssetName()`: `CHR_[Faction]_[Piece]_[Variant]`
(e.g. `CHR_Black_Knight_Centaur01`). Armor `ARM_*`, weapons `WPN_*`,
validated by `ValidateAssetNaming()` against `CinematicChessAssetRules`
from PR #2. Every violation fails loudly at init — never silently.

## Relationship to the other PRs

- PR #2 (Elisa): owns `FCharacterDNA`, `EChessFaction`, `EChessPieceType`,
  naming rules, material rules. This work includes and builds on them.
- PR #4 (Eli): his entity class should be rebased onto `FChessEntitySpec`
  here the same way this spec is based on #2's DNA — one schema each,
  no redefinitions.
