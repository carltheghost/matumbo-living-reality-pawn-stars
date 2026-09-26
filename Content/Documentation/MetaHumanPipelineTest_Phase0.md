# MetaHuman Pipeline Test — Phase 0 (UE 5.5.4)

Goal: prove the MetaHuman facial pipeline works inside the Cinematic Chess
Entity Framework before any character sculpting begins. This is a validation
pass only — no final face-bearing assets are produced here.

## Test procedure

1. **Import.** In UE 5.5.4, import a MetaHuman test character via the
   MetaHuman plugin / Quixel Bridge. Target: the DNA face slot for
   `CHR_White_King` (test rig only, not Tumbo's likeness).
2. **Face animation.** Drive the test face with a short animation sequence;
   verify lip/jaw/eye motion, skin shading, and hair render correctly under
   cinematic lighting on the dark reflective board material.
3. **Armor attach.** Attach a temporary armor mesh to the test character;
   verify no clipping at the neck/face seam during animation.
4. **DNA binding.** Assign the test DNA through
   `UCinematicChessEntityComponent::InitializeFromDNA` and confirm
   `UsesMetaHumanFace()` returns true and `IsFaceAssetApprovalPending()`
   correctly gates the build.
5. **Record results.** Pass/fail per step, with viewport screenshots, in this
   document's test log below.

## Approval gate (hard rule)

Tumbo's face is the approved male face profile (`FACE_Tumbo_Approved_Male`).
**No final face-bearing public asset ships without his explicit approval.**
Test rigs stay internal. Queens use the separate feminine profile and are not
affected by this gate.

## Test log

| Date | Step | Result | Notes |
|---|---|---|---|
| — | — | — | _Not yet run. Fill in as the pipeline is validated._ |
