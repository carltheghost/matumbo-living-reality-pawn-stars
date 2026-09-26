# Character DNA — Phase 1

Character DNA is the stable identity record for the twelve cinematic chess warriors. The forge and UE5 entity framework read the same identities; geometry is sculpted-hybrid rather than generated from primitives.

Canonical data: `Source/Schema/CinematicChessCharacterDNA.json` version 0.3.

## Non-negotiable visual law

- **King** — tall crown, long cape, royal staff, sword at hip, heroic masculine face.
- **Queen** — feminine always: elegant crown, long hair, cape, crook staff. Never Tumbo's face texture.
- **Knight** — true centaur: humanoid armored warrior torso in a horse-head helm on a complete horse body with barrel, four armored legs, hooves and tail. Sword + tower shield.
- **Bishop** — tall mitre, crook staff, cape, heroic masculine face.
- **Rook** — tower pauldrons, tower shield, cape, and staff crowned by a **massive castle** with battlements, side turrets and a visible gate. Heroic masculine face.
- **Pawn** — kettle helm with brim, spear, heroic masculine face.
- **All twelve** — full-body armor, gold filigree/trim, photoreal PBR only.

## Factions

Black resolves to obsidian + gold filigree. White resolves to ivory + gold. No third costume palette is introduced by individual roles.

## Face law

King, Bishop, Knight warrior torso, Rook and Pawn use the local approved Tumbo identity reference (`FACE_Tumbo_Male_01`) only through the privacy-gated MetaHuman pipeline. `face.jpg` is local input and must never be committed or copied into the repository without explicit Tumbo publication approval.

Queens use `FACE_Feminine_Regal_01`; they are deliberately distinct from the male identity and never consume `face.jpg`.

## Rig law

Every export is Mixamo-namespaced (`mixamorig:*`). Biped roles carry the standard Mixamo hierarchy. Centaur knights keep the humanoid Mixamo upper-body contract and extend it with `mixamorig:Horse*` bones for horse pelvis/spine/legs/tail. A knight with only two legs is invalid.

## Forge law

`Pipeline/Blender/chess_forge.py` consumes sculpted source meshes, reviewed PBR maps and already-authored skin weights. It rejects placeholder/proxy/lowpoly/primitive character objects, missing signature parts, low detail, bad materials and bad rigs. Private review previews are actual renders of the assembled asset, never concept-art stand-ins.
