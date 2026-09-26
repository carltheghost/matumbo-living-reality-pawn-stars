# Phase 0 MetaHuman Pipeline Test

Engine: Unreal Engine 5.5.4. Dev GPU: RTX 4060 (Tumbo's machine).

## Goal

Prove the MetaHuman face pipeline works end-to-end on one piece BEFORE any
character production: import → rig compatibility → animation compatibility →
performance headroom on the RTX 4060.

## Test procedure

1. **Import.** Import the approved MetaHuman face asset for `FaceProfileID`
   into `Content/CinematicChess/Faces/`. Confirm the asset name honors the
   `CHR_` prefix rule (see `Source/CinematicChess/Public/AssetNamingRules.h`).
2. **Bind.** In the test map, spawn `ACinematicChessEntity` with
   `bUsesMetaHumanFace = true` and the test `FaceProfileID`. Call
   `InitializeFromDNA` and confirm `BindMetaHumanFace` logs the bind with no
   errors. The face must sit correctly on the head socket at cinematic camera
   distance — no seam, no z-fighting with the helm.
3. **Rig compatibility.** Confirm the MetaHuman face rig drives with the
   entity's `ACharacter` skeleton: run the idle-breathing animation and check
   the jaw/eyes track. Knights use the centaur skeleton variant — face binding
   must be re-verified on the centaur rig separately.
4. **Animation compatibility.** Play the materialize entrance and one combat
   beat. Face must not detach or lag behind the head bone.
5. **Performance.** With all 32 entities spawned via
   `UCinematicChessEntityFactory::SpawnFullSet` on the RTX 4060, record FPS
   during the opening cinematic. Target: locked 60 FPS at 1080p. If the face
   shader cost breaks the budget, note it here — do not silently drop faces.

## Approval gate

Per `Pipeline/MetaHuman/README.md`: any face-bearing asset using Tumbo's
likeness (`FACE_TUMBO_APPROVED`) requires Tumbo's explicit approval before it
ships in a build. The entity code treats that profile ID as approval-gated;
this test does NOT grant approval.

## Sign-off

- [ ] Import clean, naming rule passes
- [ ] Face binds with no errors on humanoid rig
- [ ] Face binds with no errors on centaur rig (knights)
- [ ] Idle + entrance + combat animations track the face
- [ ] 32-piece scene holds 60 FPS on RTX 4060
- [ ] Results reported to Tumbo; face approval still his call
