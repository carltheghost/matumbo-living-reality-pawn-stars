# Phase 1 — Cinematic Piece Forge

## Locked bar

UE5.5.4 + MetaHuman + Blender. Daniel Eckler boss-fight quality is the visual bar. Black army is black obsidian with gold filigree; white army is ivory with gold. Every role is a full-body armored warrior. Cartoon, placeholder, low-poly and primitive-built character sources are rejected by the forge.

## Why Phase 0's procedural forge is retired

Procedural primitives can establish scale and socket layout, but they cannot create the anatomy, cloth folds, armor plate language, face quality, barding, weapon engraving or silhouette specificity required here. Phase 1 therefore treats procedural geometry as **diagnostic/blockout only** and never as a shippable character source.

## Real production pipeline

1. **Sculpt / source pack** — Artist-authored or high-resolution sculpted body, armor, cloth, hair and prop meshes live on Tumbo's PC under `TUMBO_CHESS_SOURCE_ROOT`. MetaHuman-derived face/body assets stay local when licensing/privacy requires it.
2. **Character-law assembly** — `chess_forge.py` imports the exact modular parts for the selected role. It fails if signature parts are missing: crown/staff/cape/sword for King; feminine crown/crook/long hair for Queen; full four-legged centaur anatomy + sword/tower shield for Knight; mitre/crook/cape for Bishop; tower pauldrons/tower shield/massive castle staff with battlements, side turrets and gate for Rook; kettle helm + spear for Pawn.
3. **Rig gate** — Export skeleton must contain the standard `mixamorig:*` biped bones. Centaur knights add `mixamorig:Horse*` extension bones; all exported deform bones still use the `mixamorig:` namespace.
4. **Look-dev / texture pass** — PBR faction materials are applied to tagged sculpt meshes (`ARMOR`, `FILIGREE`, `CAPE`). The manifest points at authored or reviewed AI-assisted base-color/roughness/metallic/normal maps. Texture generation itself is external to the forge; the forge consumes reviewed maps and never invents a face texture.
5. **Private face review** — Male pieces read `face.jpg` only from `--face-image`. Until Tumbo approves publication, **all twelve** GLBs and PNGs are written to a local private review directory outside the repository. The script never copies `face.jpg` into Git.
6. **Actual preview render** — The preview is rendered from the assembled 3D asset in a dark reflective board scene with warm key, cool fill and blue rim. Concept art cannot masquerade as a forge result.
7. **GLB export + QC** — The same assembled asset is exported after geometry, character-law, material and rig gates pass.
8. **Approval publication** — Only after Tumbo eyeballs all 12 private previews does `publish_approved.ps1` permit outputs to be written into `chess/glb/` for a PR.

The forge enforces the storage boundary: approved output must be inside the repository, while the private review directory and private face input must resolve outside it. Publication is all-or-nothing for both factions, and gameplay GLBs contain only the selected assembled actor—not the preview board, camera or lights.

## Source-pack layout expected on Tumbo's PC

```
TUMBO_CHESS_SOURCE_ROOT/
  bodies/male_hero.fbx
  bodies/female_regal.fbx
  bodies/centaur_hero.fbx
  armor/{king_armor,queen_armor,bishop_armor,knight_centaur_barding,rook_siege_armor,pawn_armor}.fbx
  cloth/{king_cape,queen_cape,bishop_cape,rook_cape}.fbx
  hair/queen_long_hair.fbx
  props/king_royal_staff.fbx
  props/king_sword.fbx
  props/queen_crook_staff.fbx
  props/bishop_crook_staff.fbx
  props/knight_sword.fbx
  props/knight_tower_shield.fbx
  props/rook_tower_shield.fbx
  props/rook_massive_castle_staff.fbx
  props/pawn_spear.fbx
  textures/{black,white,shared}/...
```

Source object names must contain the required manifest tags. That makes visual-law failures machine-detectable instead of depending on optimism.

## PC commands

```powershell
$env:BLENDER_EXE='C:\Program Files\Blender Foundation\Blender 4.2\blender.exe'
$env:TUMBO_CHESS_SOURCE_ROOT='D:\maTumbo\cinematic-chess-source'
$env:TUMBO_FACE_IMAGE='D:\maTumbo\private\face.jpg'
.\Pipeline\Blender\run_black.ps1
.\Pipeline\Blender\run_white.ps1
```

These commands generate **private review** outputs only.

After Tumbo explicitly approves every preview:

```powershell
$env:TUMBO_FACE_PUBLICATION_APPROVED='YES'
.\Pipeline\Blender\publish_approved.ps1 -Faction all
```

## Acceptance gate

All twelve must pass before demo integration:
- preview is rendered from the assembled 3D asset, not separate concept art;
- signature silhouette law is present;
- no placeholder/proxy/primitive source objects;
- cinematic triangle/detail floor passes;
- obsidian/ivory + gold filigree material law passes;
- `mixamorig:*` rig passes; centaur equine extension passes;
- Tumbo approves each private preview;
- only then are `chess/glb/{black|white}-{piece}.glb` and `preview-{faction}-{piece}.png` allowed into the PR/demo path.
