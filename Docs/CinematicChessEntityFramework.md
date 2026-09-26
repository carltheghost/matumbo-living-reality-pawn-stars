# Cinematic Chess Entity Framework

The shared entity architecture every one of the twelve cinematic chess pieces is built on. One framework, twelve identities — the differences between pieces live in Character DNA (`Source/Schema/CinematicChessCharacterDNA.json`), never in forked code.

## Layers

1. **Identity layer** — resolves the piece's Character DNA at spawn: faction, piece type, face profile, armor/material/weapon IDs, architecture flags. Stable across sessions; DNA version is checked on load.
2. **Physical layer** — collision capsule, board-square snapping, silhouette scale from DNA (`scale` 0.95–1.2). Knights use the centaur capsule (longer, quadruped footprint).
3. **Visual layer** — skeletal mesh + MetaHuman face + armor sets + faction materials, assembled from DNA IDs. Material law comes from `Source/Schema/FactionMaterialRules.json`: black obsidian + gold filigree vs white ivory + gold, dark reflective board.
4. **Animation layer** — implements `Source/Schema/AnimationStateContract.json`: `IdleBreathing` (breath, blink, glow pulse), `MoveGlide` (the signature glide-move), `CaptureStrike` → `CombatPose`, `Defeated`. Knights extend with the quadruped blend space.
5. **Cinematic layer** — camera framing hooks, capture slow-motion trigger, check/checkmate declarations (king raises scepter-blade). Drives the boss-fight feel of the finished game.

## Attachment system

Weapons, armor plates, helms, and capes bind to the stable socket contract (`Source/Schema/AttachmentSocketContract.json`): `Socket_Weapon_R/L`, `Socket_CastleStaff` (rook signature), `Socket_Cape`, `Socket_Helm`, `Socket_Pauldron_L/R`, plus centaur barding sockets on knights. Sockets are authored in Blender, preserved through FBX export, bound in UE5.5.4. Socket names are frozen after Phase 1 — downstream assets bind by name.

## Asset naming

Characters `CHR_[Faction]_[Piece]_[Variant]`, armor `ARM_[Faction]_[Set]`, weapons `WPN_[Piece]_[Name]`, materials `MAT_[Faction]_[Surface]`. The framework validates every referenced ID at spawn and logs a hard error on any DNA ID with no matching asset — a piece never spawns half-dressed.

## Non-goals for Phase 0

No gameplay rules, no AI, no networking. Phase 0 is foundation: DNA, schemas, sockets, materials, and the framework contract. Gameplay arrives in later phases.
