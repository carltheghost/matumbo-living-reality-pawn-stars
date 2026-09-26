# MetaHuman Pipeline Test

Phase 0 validation checklist for the MetaHuman facial pipeline on the locked target: **Unreal Engine 5.5.4** running on Tumbo's PC (**RTX 4060**). This is a validation pass, not final character production.

## Import

- [ ] Import a UE5.5.4 MetaHuman test character via Quixel Bridge.
- [ ] Confirm the MetaHuman asset opens in the UE5.5.4 editor with no blueprint errors.
- [ ] Verify the face animates: run the facial animation test sequence, confirm lip/jaw/eye movement.

## Face identity

- [ ] Apply the male face reference (`FACE_Tumbo_Male_01`) to the test character; confirm likeness reads correctly.
- [ ] Apply the feminine regal identity (`FACE_Feminine_Regal_01`); confirm it reads feminine and distinct.
- [ ] Confirm neither identity reads cartoon at cinematic camera distance — photoreal bar applies to the test too.

## Materials, hair, lighting

- [ ] Verify skin, eye, and teeth materials render correctly under the project's cinematic three-point lighting with cool rim.
- [ ] Verify hair/groom renders without artifacts on the RTX 4060 at the target frame rate.
- [ ] Verify the obsidian + gold filigree and ivory + gold material sets read correctly on armor test geo.

## Armor attachment

- [ ] Attach temporary armor to `Socket_Helm`, `Socket_Pauldron_L/R`, `Socket_Cape` — confirm sockets bind and armor follows animation.
- [ ] Attach a test weapon to `Socket_Weapon_R` and the castle staff proxy to `Socket_CastleStaff`.

## Performance (RTX 4060)

- [ ] Record frame time with one fully assembled piece in the dark reflective board scene.
- [ ] Record frame time with all twelve pieces in the scene.
- [ ] If frame time misses target, note the bottleneck (groom, shadows, reflections) — do not silently lower quality.

## Sign-off rule

No face-bearing asset — test or final — is used publicly without Tumbo's explicit approval. Check every box, attach the numbers, then report.
