# Elia — Phase 0 MetaHuman Pipeline Test (rework)

Target: Unreal Engine 5.5.4. Branch: `rebuild/cinematic-chess`.

## What this is

The Phase 0 validation rig for the MetaHuman face pipeline, implemented as
`AMetaHumanPipelineTestActor` (`Source/CinematicChess/Public|Private`).
It walks one imported MetaHuman test character through every stage that
matters before a face-bearing asset is allowed near the public build.

## The stage machine

```
NotStarted -> Import -> FaceVerify -> MaterialVerify -> HairVerify
  -> ArmorAttach -> LightingVerify -> AwaitingTumboApproval -> Approved
```

A failed stage sets the machine to `Failed` and blocks everything after it.
`RunFullPipelineTest()` walks the automated stages; the machine then PARKS
at `AwaitingTumboApproval` — it cannot reach `Approved` on its own.

## What each stage checks

- **Import**: a UE 5.5.4 MetaHuman test character is actually imported
  (no placeholder, no grey mannequin standing in for a face).
- **FaceVerify**: the face animation rig is bound to an approved
  `FaceProfileID`. Automated check is presence + binding.
- **MaterialVerify**: skin/eye/teeth/armor-trim use the faction material
  rules (obsidian + gold filigree vs ivory + gold). Any material flagged
  stylized/cartoon in its metadata fails here.
- **HairVerify**: groom asset bound and simulating; no helmet-hair cards
  on cinematic LOD.
- **ArmorAttach**: temporary Phase 0 armor mounts to the `SKT_Armor_*`
  sockets without clipping the face or breaking the neck seam.
- **LightingVerify**: the cinematic rig (key/rim/fill over the dark
  reflective board). The face must hold up under rim light — this is the
  stage where cartoon shading gets caught.

## The approval gate (Tumbo's rule, enforced)

`RecordTumboApproval(ApproverNote)` is the ONLY path to `Approved`. It
refuses to run outside the approval stage and refuses an empty note —
the record must say who reviewed what and when. `GetTestReport()` prints
the full stage ledger for his review.

No face-bearing public asset ships without `Approved`. Phase 0 validation
only — this actor never places a final face in the game.

## Photoreal bar

The bar is "real person, never cartoon." The automated stages catch
pipeline breakage; the LOOK is judged by Tumbo at the approval stage, with
the lighting check as the last automated line of defense.
