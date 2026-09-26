# Phase 1 — Cinematic Piece Forge

## Locked bar

UE5.5.4 + MetaHuman + Blender. Daniel Eckler boss-fight quality is the visual bar. Black army is black obsidian with gold filigree; white army is ivory with gold. Every role is a full-body armored warrior. Cartoon, placeholder, low-poly and primitive-built character sources are rejected by the forge.

## Why Phase 0's procedural forge is retired

Procedural primitives can establish scale and socket layout, but they cannot create the anatomy, cloth folds, armor plate language, face quality, barding, weapon engraving or silhouette specificity required here. Phase 1 therefore treats procedural geometry as **diagnostic/blockout only** and never as a shippable character source.

## Real production pipeline

1. **Sculpt / source pack** — Artist-authored or high-resolution sculpted body, armor, cloth, hair and prop meshes live on Tumbo's PC under `TUMBO_CHESS_SOURCE_ROOT`. Tumbo's `face.jpg` is used upstream to author a private MetaHuman likeness; the forge consumes a baked/exported `METAHUMAN_TUMBO` head with authored skin maps, never the photograph as a diffuse texture. The Queen uses a separate feminine `METAHUMAN_QUEEN` head.
2. **Character-law assembly** — `chess_forge.py` imports the exact modular parts for the selected role. It fails if signature parts are missing: crown/staff/cape/sword for King; feminine crown/crook/long hair for Queen; full four-legged centaur anatomy + sword/tower shield for Knight; mitre/crook/cape for Bishop; tower pauldrons/tower shield/massive castle staff with battlements, side turrets and gate for Rook; kettle helm + spear for Pawn.
3. **Rig gate** — Each role names one `rigSource` donor. Modular FBX parts may arrive with duplicate armatures; the forge retargets compatible weighted meshes onto the donor before export. Biped roles require the standard Mixamo body hierarchy. The centaur keeps the Mixamo upper body and uses `mixamorig:Horse*` pelvis/spine/leg/tail extensions instead of fake human legs. Every exported deform bone must use the `mixamorig:` namespace; non-deforming authoring/control bones may remain.
4. **Look-dev / texture pass** — PBR faction materials are applied to tagged material slots (`ARMOR`, `FILIGREE`, `CAPE`) so skin, eyes and hair materials survive untouched. The manifest points at authored or reviewed AI-assisted base-color/roughness/metallic/normal maps. Texture generation itself is external to the forge; the forge consumes reviewed maps and never invents a face texture.
5. **Private likeness review** — Male pieces require `face.jpg` only as local provenance that the private MetaHuman head was authored from the approved reference. The forge never loads the photo into a shader. Until Tumbo approves publication, **all twelve** GLBs and PNGs are written to a local private review directory outside the repository. Neither `face.jpg` nor the private MetaHuman source pack belongs in Git.
6. **Actual preview render** — The preview is rendered from the assembled 3D asset at 2048×2048 in a dark reflective board scene with warm key, cool fill and blue rim. Framing is computed from the real assembly bounds so the wide centaur and tall castle staff cannot be silently cropped. Concept art cannot masquerade as a forge result.
7. **GLB export + QC** — The same assembled asset is exported after geometry, authored-UV, character-law, material and rig gates pass. Every piece also writes a private `qc-{faction}-{piece}.json` receipt with geometry counts plus SHA-256 hashes and byte sizes for the exact PNG/GLB pair.
8. **Twelve-piece review gate** — `run_phase1_review.ps1` executes both armies, requires all 12 PNGs + GLBs + QC receipts, writes a private `phase1-review-manifest.json`, and builds `phase1-review.html` so Tumbo can inspect the complete set in one local board.
9. **Exact-artifact approval** — After Tumbo eyeballs all twelve previews, `approve_review.ps1 -IApproveAll12` verifies every QC/hash pair and writes a private `approval.json` that names the exact reviewed GLB/PNG hashes.
10. **Hash-locked publication** — `publish_approved.ps1` never launches Blender. It promotes only the exact files named by `approval.json`, re-checks SHA-256 before and after copy, and refuses partial or changed review sets.

The forge itself writes private review output only. The private review directory, source pack, MetaHuman likeness assets, and face reference must stay outside the repository. The publication step is separate and hash-locked so the files committed to the PR are the same files Tumbo actually inspected. Gameplay GLBs contain only the selected assembled actor, never the preview board, camera, or lights.

The Queen has an explicit likeness-isolation gate: any `PRIVATE_FACE_PREVIEW`, `tumbo_face`, or `male_face` asset on a Queen hard-fails the build.

## Source-pack layout expected on Tumbo's PC

```
TUMBO_CHESS_SOURCE_ROOT/
  bodies/male_hero.fbx
  bodies/female_regal.fbx
  bodies/centaur_hero.fbx
  faces/tumbo_metahuman_head.fbx
  faces/queen_regal_metahuman_head.fbx
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

Source object names must contain the silhouette/identity tags from the manifest (`METAHUMAN_TUMBO`, `METAHUMAN_QUEEN`, `tower_pauldrons`, `horse_barrel`, and so on). Armor meshes must expose `ARMOR`, `FILIGREE`, and where applicable `CAPE` material-slot names. The forge uses those slots rather than replacing whole meshes, because deleting skin and hair materials in the name of automation would be a fairly spectacular own goal.

## PC commands

```powershell
$env:BLENDER_EXE='C:\Program Files\Blender Foundation\Blender 4.2\blender.exe'
$env:TUMBO_CHESS_SOURCE_ROOT='D:\maTumbo\cinematic-chess-source'
$env:TUMBO_FACE_IMAGE='D:\maTumbo\private\face.jpg'
.\Pipeline\Blender\preflight_phase1.ps1
.\Pipeline\Blender\run_black.ps1
.\Pipeline\Blender\run_white.ps1

# Preferred: both armies + completeness/hash/QC gate + local review board
.\Pipeline\Blender\run_phase1_review.ps1
```

These commands generate **private review** outputs only. The combined runner leaves `phase1-review.html` and `phase1-review-manifest.json` beside the private outputs; neither belongs in Git.

After Tumbo explicitly approves every preview in the local review board:

```powershell
.\Pipeline\Blender\approve_review.ps1 -IApproveAll12
$env:TUMBO_FACE_PUBLICATION_APPROVED='YES'
.\Pipeline\Blender\publish_approved.ps1 -Faction all
```

`approval.json` stays in the private review directory. The publish command does not regenerate anything; it copies only the previously reviewed hash-locked outputs into `chess/glb/` for the PR.

## Acceptance gate

All twelve must pass before demo integration:
- preview is rendered from the assembled 3D asset, not separate concept art;
- signature silhouette law is present;
- no placeholder/proxy/primitive source objects;
- cinematic triangle/detail floor and authored-UV gates pass;
- preview is 2048×2048 and adaptively frames the complete silhouette;
- every output pair has a matching PASS QC receipt and SHA-256 hash;
- baked MetaHuman identity gate passes; direct `face.jpg` projection is forbidden;
- one donor Mixamo rig absorbs compatible modular weighted assets;
- obsidian/ivory + gold filigree material law passes;
- `mixamorig:*` rig passes; centaur equine extension passes;
- Tumbo approves each private preview and records the exact 12-piece review set with `approve_review.ps1 -IApproveAll12`;
- publication re-verifies the approved SHA-256 hashes and never re-runs Blender;
- only then are `chess/glb/{black|white}-{piece}.glb` and `preview-{faction}-{piece}.png` allowed into the PR/demo path.
