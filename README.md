# maTumbo Cinematic Chess Rebuild

Unreal Engine 5 + MetaHuman + Blender cinematic chess character rebuild.

## Vision

Rebuild the twelve chess pieces as cinematic warriors using a shared Cinematic Chess Entity Framework and Character DNA system.

## Locked Pipeline

- Unreal Engine 5
- MetaHuman facial pipeline
- Blender custom assets
- Standalone UE5 experience designed for future maTumbo Reality Lens integration

## Build Phases

0. Foundation
1. Character system
2. Unique identities
3. Living battlefield
4. Validation

## Rules

- Pull requests only
- No direct main/master pushes after foundation
- Tumbo controls merges

## Phase 0 — Character DNA & Schema Contracts (Eliana)

Foundation deliverables on this branch:

- `Source/Schema/CinematicChessCharacterDNA.json` — full Character DNA for all twelve pieces (six roles x two factions): factions, piece types, face/armor/material/weapon IDs, centaur architecture flags, MetaHuman face pipeline references.
- `Source/Schema/FactionMaterialRules.json` — material law: black obsidian + gold filigree vs white ivory + gold, dark reflective board.
- `Source/Schema/AnimationStateContract.json` — shared animation state machine all twelve pieces implement.
- `Source/Schema/AttachmentSocketContract.json` — socket contract for weapons, armor, capes, and the rook castle staff.
- `Docs/CharacterDNA.md` — Character DNA documentation: male likeness rules, queen identity rules, centaur knight rules.
- `Docs/CinematicChessEntityFramework.md` — entity framework: identity, physical, visual, animation, and cinematic layers.
- `Docs/MetaHumanPipelineTest.md` — UE5.5.4 + RTX 4060 MetaHuman validation checklist.

Quality bar: photoreal, never cartoon. No face-bearing public assets without Tumbo's approval.
