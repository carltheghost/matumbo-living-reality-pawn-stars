#!/usr/bin/env python3
"""Canonical static contract for the Phase 1 cinematic chess forge.

This module is intentionally Blender-free.  CI validates the manifest with it,
and chess_forge.py loads the same validator at runtime so the two enforcement
layers cannot quietly drift apart.
"""
from __future__ import annotations

import argparse
import json
import pathlib
from typing import Any

PIECES = ("king", "queen", "bishop", "knight", "rook", "pawn")
COMMON_LAW = {"armor", "filigree"}

LAW = {
    "king": {"tall_crown", "long_cape", "royal_staff", "sword_hip"},
    "queen": {"feminine", "elegant_crown", "cape", "crook_staff", "long_hair"},
    "knight": {
        "centaur", "humanoid_torso", "horse_head_helm", "horse_barrel",
        "four_legs", "armored_legs", "hooves", "tail", "sword", "tower_shield",
    },
    "bishop": {"tall_mitre", "crook_staff", "cape"},
    "rook": {
        "tower_pauldrons", "tower_shield", "cape", "massive_castle_staff",
        "battlements", "side_turrets", "gate",
    },
    "pawn": {"kettle_helm", "brim", "spear"},
}

RIGID = {
    "king": {"tall_crown", "royal_staff", "sword_hip", "metahuman_tumbo"},
    "queen": {"elegant_crown", "crook_staff", "metahuman_queen"},
    "bishop": {"tall_mitre", "crook_staff", "metahuman_tumbo"},
    "knight": {"horse_head_helm", "sword", "tower_shield", "metahuman_tumbo"},
    "rook": {"tower_shield", "massive_castle_staff", "metahuman_tumbo"},
    "pawn": {"kettle_helm", "spear", "metahuman_tumbo"},
}

IDENTITY = {
    "king": "metahuman_tumbo",
    "queen": "metahuman_queen",
    "bishop": "metahuman_tumbo",
    "knight": "metahuman_tumbo",
    "rook": "metahuman_tumbo",
    "pawn": "metahuman_tumbo",
}

MIN_TRIANGLES = {
    "king": 120_000,
    "queen": 120_000,
    "bishop": 110_000,
    "knight": 180_000,
    "rook": 140_000,
    "pawn": 90_000,
}

MIN_MESH_OBJECTS = {
    "king": 12,
    "queen": 12,
    "bishop": 11,
    "knight": 18,
    "rook": 15,
    "pawn": 9,
}

FACTION_SURFACES = {
    "black": {"armor": "obsidian", "gold": "gold"},
    "white": {"armor": "ivory", "gold": "gold"},
}

REQUIRED_TEXTURE_SLOTS = {
    "armor": {"baseColor", "roughness", "metallic", "normal"},
    "gold": {"baseColor", "roughness", "metallic", "normal"},
    "cloth": {"baseColor", "roughness", "normal"},
}

MIN_TEXTURE_RESOLUTION = {
    "armor": 4096,
    "gold": 4096,
    "cloth": 2048,
}


def _safe_relative(path: object) -> bool:
    text = str(path).replace("\\", "/")
    posix = pathlib.PurePosixPath(text)
    windows = pathlib.PureWindowsPath(str(path))
    return (
        bool(text)
        and not posix.is_absolute()
        and not windows.is_absolute()
        and ".." not in posix.parts
    )


def validate_data(m: dict[str, Any]) -> list[str]:
    """Validate the immutable Phase 1 design/production contract."""
    errors: list[str] = []

    if m.get("schemaVersion") != 1:
        errors.append("schemaVersion must be 1")

    quality = str(m.get("qualityBar", "")).lower()
    for phrase in ("photoreal", "no placeholder", "lowpoly", "primitive"):
        if phrase not in quality:
            errors.append(f"qualityBar must explicitly preserve {phrase!r}")

    pieces = m.get("pieces", {})
    if set(pieces) != set(PIECES):
        errors.append("pieces must be exactly six canonical roles")

    for piece in PIECES:
        spec = pieces.get(piece, {})
        tags = set(spec.get("requiredTags", []))
        missing_law = (LAW[piece] | COMMON_LAW) - tags
        if missing_law:
            errors.append(f"{piece}: missing law tags {sorted(missing_law)}")

        if spec.get("sourceMode") != "sculpted-hybrid":
            errors.append(f"{piece}: sourceMode must be sculpted-hybrid")

        sources = spec.get("sources", [])
        if not sources:
            errors.append(f"{piece}: no sculpted source files")
        for source in sources:
            if not _safe_relative(source):
                errors.append(f"{piece}: unsafe source path {source!r}")
            if pathlib.PurePosixPath(str(source).replace("\\", "/")).name.lower() == "face.jpg":
                errors.append(f"{piece}: raw face.jpg may never be a forge source")

        source_text = " ".join(str(x).lower().replace("\\", "/") for x in sources)
        if piece == "queen":
            if any(token in source_text for token in ("tumbo_metahuman", "tumbo_face", "male_face")):
                errors.append("queen: Tumbo/male likeness source is forbidden")
            if "faces/queen_regal_metahuman_head.fbx" not in source_text:
                errors.append("queen: missing dedicated feminine MetaHuman head source")
        elif "faces/tumbo_metahuman_head.fbx" not in source_text:
            errors.append(f"{piece}: missing Tumbo MetaHuman head source")

        rig_source = spec.get("rigSource")
        if not rig_source or rig_source not in sources:
            errors.append(f"{piece}: rigSource must name one sculpt source")
        elif not _safe_relative(rig_source):
            errors.append(f"{piece}: unsafe rigSource {rig_source!r}")

        if IDENTITY[piece] not in tags:
            errors.append(
                f"{piece}: missing private MetaHuman identity tag {IDENTITY[piece]!r}"
            )

        if int(spec.get("minTriangles", 0)) < MIN_TRIANGLES[piece]:
            errors.append(
                f"{piece}: cinematic triangle floor below {MIN_TRIANGLES[piece]:,}"
            )
        if int(spec.get("minMeshObjects", 0)) < MIN_MESH_OBJECTS[piece]:
            errors.append(
                f"{piece}: cinematic mesh-object floor below {MIN_MESH_OBJECTS[piece]}"
            )

        rigid = set(spec.get("rigidMounts", {}))
        missing_rigid = RIGID[piece] - rigid
        if missing_rigid:
            errors.append(f"{piece}: missing rigid mounts {sorted(missing_rigid)}")

    materials = m.get("materials", {})
    for faction, identities in FACTION_SURFACES.items():
        fs = materials.get(faction, {})
        for material_name in ("armor", "gold", "cloth"):
            spec = fs.get(material_name, {})
            if not spec:
                errors.append(f"{faction}: missing {material_name} material spec")
                continue

            floor = MIN_TEXTURE_RESOLUTION[material_name]
            if int(spec.get("minResolution", 0)) < floor:
                errors.append(
                    f"{faction}/{material_name}: texture floor below {floor}px"
                )

            textures = spec.get("textures", {})
            missing_slots = REQUIRED_TEXTURE_SLOTS[material_name] - set(textures)
            if missing_slots:
                errors.append(
                    f"{faction}/{material_name}: incomplete PBR texture set; "
                    f"missing {sorted(missing_slots)}"
                )
            for slot, source in textures.items():
                if not _safe_relative(source):
                    errors.append(
                        f"{faction}/{material_name}/{slot}: unsafe texture path {source!r}"
                    )
                if pathlib.PurePosixPath(str(source).replace("\\", "/")).name.lower() == "face.jpg":
                    errors.append(
                        f"{faction}/{material_name}/{slot}: raw face.jpg texture is forbidden"
                    )

        armor = fs.get("armor", {})
        expected_surface = "obsidian" if faction == "black" else "ivory"
        if float(armor.get("metallic", 1.0)) > 0.05:
            errors.append(
                f"{faction}/armor: {expected_surface} must remain dielectric, not metallic"
            )

        for material_name, identity in identities.items():
            actual = str(fs.get(material_name, {}).get("surfaceClass", "")).lower()
            if actual != identity:
                errors.append(
                    f"{faction}/{material_name}: surfaceClass must be {identity!r}"
                )

    privacy = m.get("privacy", {})
    if privacy.get("maleFacePublicByDefault", True):
        errors.append("maleFacePublicByDefault must be false")

    face_rule = str(m.get("texturePolicy", {}).get("faceRule", "")).lower()
    if "direct projection" not in face_rule or "forbidden" not in face_rule:
        errors.append("texturePolicy.faceRule must forbid direct face-photo projection")

    return errors


def validate(path: pathlib.Path) -> list[str]:
    return validate_data(json.loads(path.read_text(encoding="utf-8")))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest")
    ns = parser.parse_args()
    errors = validate(pathlib.Path(ns.manifest))
    if errors:
        print("FAIL")
        for error in errors:
            print("-", error)
        raise SystemExit(1)
    print("PASS: Phase 1 cinematic forge manifest contract")


if __name__ == "__main__":
    main()
