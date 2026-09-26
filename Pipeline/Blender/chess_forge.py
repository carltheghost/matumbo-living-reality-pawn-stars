#!/usr/bin/env python3
"""maTumbo Cinematic Chess Phase 1 asset-driven Blender forge.

This forge is deliberately NOT a procedural-character generator. It assembles
artist/sculpt/MetaHuman-derived source meshes, validates the rig/material/body
contract, applies faction look-dev, renders an actual preview from the assembled
asset, and exports GLB only when the appropriate privacy gate is satisfied.

Run inside Blender:
    blender --background --python Pipeline/Blender/chess_forge.py -- \
      --manifest Pipeline/Blender/forge_manifest.json --faction black \
      --piece king --source-root D:/maTumboChessSource --output-root chess/glb \
      --private-review-root D:/maTumboChessReview --repository-root . \
      --face-image D:/private/face.jpg

The forge writes PRIVATE REVIEW outputs only. Tumbo's raw face reference is
never copied into the repository and may not be used as a flat diffuse texture.
Male pieces must contain a locally prepared MetaHuman likeness. After Tumbo
reviews all twelve outputs, a separate hash-locked promotion step copies those
exact reviewed GLBs/PNGs into the repository.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import pathlib
import sys
from dataclasses import dataclass
from typing import Any, Iterable

try:
    import bpy  # type: ignore
    from mathutils import Vector  # type: ignore
except Exception:  # pragma: no cover
    bpy = None
    Vector = None

PIECE_ORDER = ("king", "queen", "bishop", "knight", "rook", "pawn")
FACTIONS = ("black", "white")
MALE_PIECES = frozenset({"king", "bishop", "knight", "rook", "pawn"})
REQUIRED_MIXAMO_UPPER = (
    "mixamorig:Hips", "mixamorig:Spine", "mixamorig:Spine1",
    "mixamorig:Spine2", "mixamorig:Neck", "mixamorig:Head",
    "mixamorig:LeftShoulder", "mixamorig:LeftArm", "mixamorig:LeftForeArm",
    "mixamorig:LeftHand", "mixamorig:RightShoulder", "mixamorig:RightArm",
    "mixamorig:RightForeArm", "mixamorig:RightHand",
)
REQUIRED_MIXAMO_BIPED_LEGS = (
    "mixamorig:LeftUpLeg", "mixamorig:LeftLeg", "mixamorig:LeftFoot",
    "mixamorig:RightUpLeg", "mixamorig:RightLeg", "mixamorig:RightFoot",
)
REQUIRED_CENTAUR_BONES = (
    "mixamorig:HorsePelvis", "mixamorig:HorseSpine", "mixamorig:HorseTail01",
    "mixamorig:HorseFrontLegL", "mixamorig:HorseFrontLegR",
    "mixamorig:HorseHindLegL", "mixamorig:HorseHindLegR",
)

class ForgeError(RuntimeError):
    pass

@dataclass(frozen=True)
class BuildTarget:
    faction: str
    piece: str
    spec: dict[str, Any]
    source_root: pathlib.Path
    output_root: pathlib.Path
    private_review_root: pathlib.Path
    face_image: pathlib.Path | None

    @property
    def slug(self) -> str:
        return f"{self.faction}-{self.piece}"

    @property
    def is_male(self) -> bool:
        return self.piece in MALE_PIECES

    @property
    def private_mode(self) -> bool:
        # Public writes are forbidden here. Publication is a separate,
        # hash-locked promotion of the exact artifacts Tumbo reviewed.
        return True

    @property
    def preview_path(self) -> pathlib.Path:
        return self.private_review_root / f"preview-{self.slug}.png"

    @property
    def glb_path(self) -> pathlib.Path:
        return self.private_review_root / f"{self.slug}.glb"

    @property
    def qc_path(self) -> pathlib.Path:
        return self.private_review_root / f"qc-{self.slug}.json"


def _args(argv: list[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Cinematic chess hybrid asset forge")
    p.add_argument("--manifest", required=True)
    p.add_argument("--faction", choices=FACTIONS + ("all",), default="all")
    p.add_argument("--piece", choices=PIECE_ORDER + ("all",), default="all")
    p.add_argument("--source-root", required=True)
    p.add_argument("--output-root", required=True)
    p.add_argument("--private-review-root", required=True)
    p.add_argument("--repository-root", required=True,
                   help="Repository root used to enforce the private-output boundary")
    p.add_argument("--face-image")
    p.add_argument("--cycles-samples", type=int, default=256)
    return p.parse_args(argv)


def _load_manifest(path: pathlib.Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))

    # Runtime and CI must enforce the exact same canonical contract. Loading
    # forge_contract.py directly from this directory avoids relying on Blender's
    # working-directory/sys.path behavior.
    contract_path = pathlib.Path(__file__).resolve().with_name("forge_contract.py")
    spec = importlib.util.spec_from_file_location("phase1_forge_contract", contract_path)
    if spec is None or spec.loader is None:
        raise ForgeError(f"Unable to load forge contract: {contract_path}")
    contract = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(contract)
    errors = contract.validate_data(data)
    if errors:
        raise ForgeError("Manifest violates Phase 1 contract:\n- " + "\n- ".join(errors))
    return data


def _require_blender() -> None:
    if bpy is None:
        raise ForgeError("This command must run inside Blender Python (bpy unavailable).")
    version = tuple(bpy.app.version)
    if version < (4, 2, 0):
        raise ForgeError(f"Blender 4.2+ required, found {version}")


def _reset_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.armatures,
                       bpy.data.cameras, bpy.data.lights):
        for block in list(datablocks):
            if block.users == 0:
                datablocks.remove(block)
    # Material datablocks survive object deletion unless explicitly removed.
    # Leaving them around makes later pieces import suffixed materials and can
    # bind the private face image to a stale, unused material from piece one.
    for material in list(bpy.data.materials):
        if material.users == 0:
            bpy.data.materials.remove(material)


def _resolve_source(root: pathlib.Path, rel: str) -> pathlib.Path:
    root = root.resolve()
    p = (root / rel).resolve()
    if not p.is_relative_to(root):
        raise ForgeError(f"Source path escapes the approved source root: {rel}")
    if not p.exists():
        raise ForgeError(f"Required sculpt/source asset is missing: {p}")
    return p


def _validate_storage_boundaries(
    repository_root: pathlib.Path,
    output_root: pathlib.Path,
    private_root: pathlib.Path,
    face_image: pathlib.Path | None,
) -> None:
    if not output_root.is_relative_to(repository_root):
        raise ForgeError("Approved output root must stay inside the repository")
    if private_root.is_relative_to(repository_root):
        raise ForgeError("Private review root must stay outside the repository")
    if face_image is not None and face_image.is_relative_to(repository_root):
        raise ForgeError("Private face input must stay outside the repository")


def _import_asset(path: pathlib.Path) -> list[Any]:
    before = set(bpy.data.objects)
    ext = path.suffix.lower()
    if ext == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(path), automatic_bone_orientation=False)
    elif ext in {".glb", ".gltf"}:
        bpy.ops.import_scene.gltf(filepath=str(path))
    elif ext == ".obj":
        bpy.ops.wm.obj_import(filepath=str(path))
    else:
        raise ForgeError(f"Unsupported source type: {path}")
    return [o for o in bpy.data.objects if o not in before]


def _find_armature(objects: Iterable[Any]) -> Any:
    arms = [o for o in objects if o.type == "ARMATURE"]
    if len(arms) != 1:
        raise ForgeError(f"Expected exactly one imported armature, found {len(arms)}")
    return arms[0]


def _retarget_secondary_armatures(
    primary: Any, imported: list[Any], slug: str
) -> list[Any]:
    """Collapse modular FBX armatures onto the selected donor Mixamo rig."""
    secondary_arms = [o for o in imported if o.type == "ARMATURE"]
    primary_names = {b.name for b in primary.data.bones}
    for mesh in [o for o in imported if o.type == "MESH"]:
        weighted_names = {
            mesh.vertex_groups[assignment.group].name
            for vertex in mesh.data.vertices
            for assignment in vertex.groups
            if assignment.weight > 0
        }
        missing = sorted(
            name for name in weighted_names
            if name.startswith("mixamorig:") and name not in primary_names
        )
        if missing:
            raise ForgeError(
                f"{slug}: modular mesh {mesh.name} uses bones absent from donor rig: "
                f"{missing[:12]}"
            )
        world = mesh.matrix_world.copy()
        for mod in mesh.modifiers:
            if mod.type == "ARMATURE" and mod.object in secondary_arms:
                mod.object = primary
        if mesh.parent in secondary_arms:
            mesh.parent = primary
            mesh.matrix_world = world
    return [o for o in imported if o.type != "ARMATURE"]


def _import_source_pack(target: BuildTarget) -> tuple[list[Any], Any]:
    sources = list(target.spec.get("sources", []))
    rig_source = target.spec.get("rigSource")
    if not sources:
        raise ForgeError(f"{target.slug}: no source sculpts listed")
    if not rig_source or rig_source not in sources:
        raise ForgeError(f"{target.slug}: rigSource must name one entry from sources")

    donor_batch = _import_asset(_resolve_source(target.source_root, rig_source))
    donor = _find_armature(donor_batch)
    imported: list[Any] = list(donor_batch)
    for rel in sources:
        if rel == rig_source:
            continue
        batch = _import_asset(_resolve_source(target.source_root, rel))
        imported.extend(_retarget_secondary_armatures(donor, batch, target.slug))
    return imported, donor


def _validate_mixamo(armature: Any, piece: str) -> None:
    bones = list(armature.data.bones)
    names = {b.name for b in bones}
    required = list(REQUIRED_MIXAMO_UPPER)
    required.extend(
        REQUIRED_CENTAUR_BONES if piece == "knight" else REQUIRED_MIXAMO_BIPED_LEGS
    )
    missing = [name for name in required if name not in names]
    if missing:
        raise ForgeError(f"{piece}: Mixamo rig missing bones: {', '.join(missing)}")
    # GLB armatures can carry non-deform helpers too. The contract is stricter
    # than "deform bones only": every exported bone must stay in the
    # mixamorig:* namespace so no control/helper junk leaks into gameplay.
    bad = sorted(b.name for b in bones if not b.name.startswith("mixamorig:"))
    if bad:
        raise ForgeError(
            f"{piece}: every export bone must use mixamorig:* naming; "
            f"bad={bad[:8]}"
        )


def _material(name: str, base_rgba: tuple[float, float, float, float],
              metallic: float, roughness: float) -> Any:
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes.get("Principled BSDF")
    if bsdf is None:
        raise ForgeError(f"Material {name}: Principled BSDF unavailable")
    bsdf.inputs["Base Color"].default_value = base_rgba
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return mat


def _load_image(path: pathlib.Path, non_color: bool = False) -> Any:
    img = bpy.data.images.load(str(path), check_existing=True)
    if non_color:
        img.colorspace_settings.name = "Non-Color"
    return img


def _build_pbr_material(name: str, spec: dict[str, Any], root: pathlib.Path) -> Any:
    mat = _material(
        name,
        tuple(spec["baseColor"]),
        float(spec["metallic"]),
        float(spec["roughness"]),
    )
    min_res = int(spec.get("minResolution", 0))
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    if bsdf is None:
        raise ForgeError(f"Material {name}: Principled BSDF unavailable")
    for input_name, key in (
        ("IOR", "ior"),
        ("Coat Weight", "coatWeight"),
        ("Coat Roughness", "coatRoughness"),
        ("Subsurface Weight", "subsurfaceWeight"),
    ):
        if key in spec and input_name in bsdf.inputs:
            bsdf.inputs[input_name].default_value = float(spec[key])
    tex = spec.get("textures", {})
    for slot, input_name, non_color in (
        ("baseColor", "Base Color", False),
        ("roughness", "Roughness", True),
        ("metallic", "Metallic", True),
    ):
        rel = tex.get(slot)
        if not rel:
            continue
        p = _resolve_source(root, rel)
        node = nt.nodes.new("ShaderNodeTexImage")
        node.name = f"TEX_{slot.upper()}"
        node.image = _load_image(p, non_color=non_color)
        if min_res and min(node.image.size[0], node.image.size[1]) < min_res:
            raise ForgeError(
                f"{p}: texture resolution {tuple(node.image.size)} below {min_res}px floor"
            )
        if slot == "metallic":
            # Obsidian and ivory are dielectric. Multiplying the authored map
            # by the manifest scalar prevents a stray white metallic map from
            # turning them into generic painted metal. Gold remains 1.0.
            multiply = nt.nodes.new("ShaderNodeMath")
            multiply.operation = "MULTIPLY"
            multiply.inputs[1].default_value = float(spec["metallic"])
            nt.links.new(node.outputs["Color"], multiply.inputs[0])
            nt.links.new(multiply.outputs["Value"], bsdf.inputs[input_name])
        else:
            nt.links.new(node.outputs["Color"], bsdf.inputs[input_name])
    normal_rel = tex.get("normal")
    if normal_rel:
        p = _resolve_source(root, normal_rel)
        image = nt.nodes.new("ShaderNodeTexImage")
        image.name = "TEX_NORMAL"
        image.image = _load_image(p, non_color=True)
        if min_res and min(image.image.size[0], image.image.size[1]) < min_res:
            raise ForgeError(
                f"{p}: texture resolution {tuple(image.image.size)} below {min_res}px floor"
            )
        normal = nt.nodes.new("ShaderNodeNormalMap")
        nt.links.new(image.outputs["Color"], normal.inputs["Color"])
        nt.links.new(normal.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def _assign_material_by_tag(objects: Iterable[Any], tag: str, mat: Any) -> int:
    """Swap only tagged material regions, preserving skin and hair materials."""
    count = 0
    needle = tag.lower()
    for obj in objects:
        if obj.type != "MESH":
            continue
        for slot in obj.material_slots:
            current = slot.material
            if current is not None and needle in current.name.lower():
                slot.material = mat
                count += 1
    return count


def _apply_faction_look(target: BuildTarget, objects: list[Any], manifest: dict[str, Any]) -> None:
    mspec = manifest["materials"][target.faction]
    base = _build_pbr_material(
        f"MAT_{target.faction}_armor", mspec["armor"], target.source_root
    )
    gold = _build_pbr_material(
        "MAT_gold_filigree", mspec["gold"], target.source_root
    )
    cloth = _build_pbr_material(
        f"MAT_{target.faction}_cloth", mspec["cloth"], target.source_root
    )
    assigned = {
        "armor": _assign_material_by_tag(objects, "armor", base),
        "gold": _assign_material_by_tag(objects, "filigree", gold),
        "cloth": _assign_material_by_tag(objects, "cape", cloth),
    }
    if assigned["armor"] == 0 or assigned["gold"] == 0:
        raise ForgeError(
            f"{target.slug}: source materials must expose ARMOR and FILIGREE tagged slots"
        )
    required_tags = {str(tag).lower() for tag in target.spec.get("requiredTags", [])}
    if {"cape", "long_cape"} & required_tags and assigned["cloth"] == 0:
        raise ForgeError(
            f"{target.slug}: cape-bearing piece must expose a CAPE-tagged cloth material slot"
        )


def _attach_rigid_parts(
    target: BuildTarget, objects: Iterable[Any], armature: Any
) -> set[str]:
    attached: set[str] = set()
    bone_names = {b.name for b in armature.data.bones}
    for tag, bone in target.spec.get("rigidMounts", {}).items():
        matches = [
            o for o in objects
            if o.type == "MESH" and tag.lower() in o.name.lower()
        ]
        if not matches:
            raise ForgeError(
                f"{target.slug}: rigid mount tag {tag!r} has no mesh"
            )
        if bone not in bone_names:
            raise ForgeError(
                f"{target.slug}: rigid mount bone {bone!r} missing"
            )
        for obj in matches:
            world = obj.matrix_world.copy()
            obj.parent = armature
            obj.parent_type = "BONE"
            obj.parent_bone = bone
            # Bone parenting must not move an artist-authored prop away from
            # its reviewed assembly position.
            obj.matrix_world = world
            attached.add(obj.name)
    return attached


def _parent_weighted_to_armature(
    objects: Iterable[Any], armature: Any, rigid_names: set[str]
) -> None:
    deform_bones = {
        b.name for b in armature.data.bones if getattr(b, "use_deform", True)
    }
    for obj in objects:
        if obj.type != "MESH" or obj.name in rigid_names:
            continue
        # Body, armor, hair and cloth arrive artist-weighted. The forge does
        # not use automatic envelopes because that would destroy authored
        # deformation quality around shoulders, capes and centaur joins.
        valid_group_indices = {
            g.index for g in obj.vertex_groups if g.name in deform_bones
        }
        if not valid_group_indices:
            raise ForgeError(
                f"{obj.name}: no weights target a deform bone on the export rig"
            )
        unweighted = [
            v.index for v in obj.data.vertices
            if not any(
                assignment.group in valid_group_indices and assignment.weight > 0
                for assignment in v.groups
            )
        ]
        if unweighted:
            raise ForgeError(
                f"{obj.name}: {len(unweighted)} vertices lack export-rig weights; "
                f"first={unweighted[:8]}"
            )
        if obj.parent != armature:
            world = obj.matrix_world.copy()
            obj.parent = armature
            obj.matrix_world = world
        mod = next(
            (m for m in obj.modifiers if m.type == "ARMATURE"), None
        )
        if mod is None:
            mod = obj.modifiers.new(name="Armature", type="ARMATURE")
        mod.object = armature


def _validate_piece_law(target: BuildTarget, objects: Iterable[Any]) -> None:
    object_names = " ".join(o.name.lower() for o in objects)
    material_names = " ".join(
        slot.material.name.lower()
        for obj in objects
        if obj.type == "MESH"
        for slot in obj.material_slots
        if slot.material is not None
    )
    required = [s.lower() for s in target.spec.get("requiredTags", [])]
    material_region_tags = {"armor", "filigree"}
    missing = [
        tag for tag in required
        if (
            tag not in (object_names + " " + material_names)
            if tag in material_region_tags
            else tag not in object_names
        )
    ]
    if missing:
        raise ForgeError(
            f"{target.slug}: sculpt assembly missing character-law tags: {missing}"
        )
    forbidden = ("placeholder", "proxy", "lowpoly", "primitive")
    bad = [o.name for o in objects if any(x in o.name.lower() for x in forbidden)]
    if bad:
        raise ForgeError(
            f"{target.slug}: placeholder/primitive source objects are forbidden: {bad[:8]}"
        )



def _validate_queen_face_policy(target: BuildTarget, objects: Iterable[Any]) -> None:
    """Queens are always feminine and may never carry Tumbo/private-face assets."""
    if target.piece != "queen":
        return
    violations: list[str] = []
    for obj in objects:
        lower_name = obj.name.lower()
        if (
            "tumbo_face" in lower_name
            or "male_face" in lower_name
            or "metahuman_tumbo" in lower_name
        ):
            violations.append(obj.name)
        if obj.type != "MESH":
            continue
        for slot in obj.material_slots:
            mat = slot.material
            if mat is None:
                continue
            name = mat.name.lower()
            if (
                name == "private_face_preview"
                or name.startswith("private_face_preview.")
                or "tumbo_face" in name
                or "male_face" in name
                or "metahuman_tumbo" in name
            ):
                violations.append(f"{obj.name}:{mat.name}")
            if mat.use_nodes and mat.node_tree is not None:
                for node in mat.node_tree.nodes:
                    image = getattr(node, "image", None)
                    if image is None:
                        continue
                    image_ref = " ".join((
                        str(getattr(image, "name", "")),
                        str(getattr(image, "filepath", "") or ""),
                    )).lower()
                    if any(token in image_ref for token in (
                        "metahuman_tumbo", "tumbo_face", "male_face",
                        "private_face_preview",
                    )):
                        violations.append(
                            f"{obj.name}:{mat.name}:{getattr(image, 'name', '<image>')}"
                        )
    if violations:
        raise ForgeError(
            f"{target.slug}: queen character law forbids Tumbo/private male-face assets: "
            + ", ".join(violations[:8])
        )


def _geometry_stats(objects: Iterable[Any]) -> tuple[int, int]:
    verts = 0
    tris = 0
    for obj in objects:
        if obj.type != "MESH":
            continue
        verts += len(obj.data.vertices)
        for poly in obj.data.polygons:
            tris += max(1, len(poly.vertices) - 2)
    return verts, tris


def _validate_geometry(target: BuildTarget, objects: list[Any]) -> None:
    _, tris = _geometry_stats(objects)
    min_tris = int(target.spec.get("minTriangles", 80000))
    if tris < min_tris:
        raise ForgeError(
            f"{target.slug}: {tris:,} triangles < cinematic floor {min_tris:,}"
        )
    meshes = [o for o in objects if o.type == "MESH"]
    if len(meshes) < int(target.spec.get("minMeshObjects", 8)):
        raise ForgeError(f"{target.slug}: assembly has too few sculpted mesh parts")
    missing_uv = [o.name for o in meshes if len(o.data.uv_layers) == 0]
    if missing_uv:
        raise ForgeError(
            f"{target.slug}: cinematic source meshes require authored UVs; "
            f"missing on {missing_uv[:8]}"
        )
    zero_area = [
        o.name for o in meshes
        if min(float(abs(v)) for v in o.dimensions) <= 1e-5
    ]
    if zero_area:
        raise ForgeError(
            f"{target.slug}: degenerate mesh bounds detected: {zero_area[:8]}"
        )


def _validate_private_metahuman_identity(
    target: BuildTarget, objects: Iterable[Any]
) -> None:
    """Require a baked private MetaHuman likeness; never project face.jpg."""
    if not target.is_male:
        return
    if target.face_image is None or not target.face_image.exists():
        raise ForgeError(
            f"{target.slug}: local face.jpg is required as private likeness provenance"
        )
    object_names = " ".join(o.name.lower() for o in objects)
    if "metahuman_tumbo" not in object_names:
        raise ForgeError(
            f"{target.slug}: male assembly must include a baked METAHUMAN_TUMBO "
            "head generated from the approved private reference"
        )
    for obj in objects:
        if obj.type != "MESH":
            continue
        for slot in obj.material_slots:
            mat = slot.material
            if mat and "private_face_preview" in mat.name.lower():
                raise ForgeError(
                    f"{target.slug}: flat face-photo materials are forbidden; "
                    "use the baked private MetaHuman head and authored skin maps"
                )


def _validate_raw_face_reference_absent(
    target: BuildTarget, objects: Iterable[Any]
) -> None:
    """Never allow the raw face.jpg to become a texture dependency of a piece."""
    if target.face_image is None:
        return
    face_path = target.face_image.resolve()
    face_name = face_path.name.lower()
    violations: list[str] = []
    seen_images: set[int] = set()
    for obj in objects:
        if obj.type != "MESH":
            continue
        for slot in obj.material_slots:
            mat = slot.material
            if mat is None or not mat.use_nodes or mat.node_tree is None:
                continue
            for node in mat.node_tree.nodes:
                image = getattr(node, "image", None)
                if image is None or id(image) in seen_images:
                    continue
                seen_images.add(id(image))
                image_name = str(getattr(image, "name", "")).lower()
                raw_path = str(getattr(image, "filepath", "") or "")
                resolved = None
                if raw_path:
                    try:
                        resolved = pathlib.Path(bpy.path.abspath(raw_path)).resolve()
                    except Exception:
                        resolved = None
                if (
                    image_name == face_name
                    or pathlib.Path(raw_path).name.lower() == face_name
                    or resolved == face_path
                ):
                    violations.append(
                        f"{obj.name}:{mat.name}:{getattr(image, 'name', '<image>')}"
                    )
    if violations:
        raise ForgeError(
            f"{target.slug}: raw face.jpg texture dependency detected; "
            "only baked MetaHuman skin maps may enter review/export: "
            + ", ".join(violations[:8])
        )


def _world_bounds(objects: Iterable[Any]) -> tuple[Any, Any, Any]:
    points = []
    for obj in objects:
        if obj.type != "MESH":
            continue
        points.extend(obj.matrix_world @ Vector(corner) for corner in obj.bound_box)
    if not points:
        raise ForgeError("Cannot frame preview: assembly has no mesh bounds")
    minimum = Vector((
        min(p.x for p in points), min(p.y for p in points), min(p.z for p in points)
    ))
    maximum = Vector((
        max(p.x for p in points), max(p.y for p in points), max(p.z for p in points)
    ))
    return minimum, maximum, (minimum + maximum) * 0.5


def _setup_cinematic_scene(
    target: BuildTarget, objects: Iterable[Any], samples: int
) -> None:
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    try:
        prefs = bpy.context.preferences.addons["cycles"].preferences
        prefs.get_devices()
        prefs.compute_device_type = "OPTIX"
        scene.cycles.device = "GPU"
    except Exception:
        scene.cycles.device = "CPU"

    # Private review renders are deliberately large enough to inspect face,
    # filigree, cloth weave and armor micro-detail without mistaking a thumbnail
    # for a quality gate.
    scene.render.resolution_x = 2048
    scene.render.resolution_y = 2048
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.render.film_transparent = False
    scene.world.color = (0.004, 0.005, 0.008)

    minimum, maximum, center = _world_bounds(objects)
    size = maximum - minimum
    radius = max(float(size.x), float(size.y), float(size.z), 1.0)
    floor_z = float(minimum.z) - max(radius * 0.006, 0.004)
    board_size = max(14.0, radius * 3.1)

    bpy.ops.mesh.primitive_plane_add(
        size=board_size, location=(float(center.x), float(center.y), floor_z)
    )
    board = bpy.context.active_object
    board.name = "RENDER_BOARD"
    board.data.materials.append(
        _material("MAT_Board", (0.008, 0.009, 0.014, 1), 0.75, 0.18)
    )

    aim = Vector((
        float(center.x),
        float(center.y),
        float(minimum.z + size.z * (0.54 if target.piece == "knight" else 0.58)),
    ))

    def area(
        name: str,
        offset: tuple[float, float, float],
        energy: float,
        size_scale: float,
        color: tuple[float, float, float],
    ) -> None:
        data = bpy.data.lights.new(name, type="AREA")
        data.energy = energy
        data.shape = "DISK"
        data.size = max(radius * size_scale, 2.0)
        data.color = color
        obj = bpy.data.objects.new(name, data)
        bpy.context.collection.objects.link(obj)
        obj.location = aim + Vector(offset) * radius
        direction = aim - obj.location
        obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()

    area("Key", (0.82, -1.00, 1.08), 1700, 0.72, (1.0, 0.75, 0.48))
    area("Fill", (-0.92, -0.45, 0.72), 760, 0.90, (0.38, 0.48, 0.72))
    area("Rim", (0.0, 0.92, 0.92), 1450, 0.58, (0.25, 0.42, 1.0))

    cam_data = bpy.data.cameras.new("ForgeCamera")
    cam = bpy.data.objects.new("ForgeCamera", cam_data)
    bpy.context.collection.objects.link(cam)
    scene.camera = cam
    cam.data.lens = 72 if target.piece != "knight" else 66

    # Frame from real assembly bounds rather than magic coordinates. The old
    # fixed camera could crop the centaur or make a tall castle staff look tiny.
    half_fov = max(float(cam.data.angle) * 0.5, math.radians(12.0))
    distance = (radius * 0.72) / max(math.tan(half_fov), 0.15)
    distance *= 1.20 if target.piece == "knight" else 1.10
    view_dir = Vector((0.62, -1.0, 0.30 if target.piece == "knight" else 0.24))
    view_dir.normalize()
    cam.location = aim - view_dir * distance
    cam.rotation_euler = (aim - cam.location).to_track_quat("-Z", "Y").to_euler()


def _render_preview(target: BuildTarget) -> None:
    target.preview_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.context.scene.render.filepath = str(target.preview_path)
    bpy.ops.render.render(write_still=True)
    if (
        not target.preview_path.exists()
        or target.preview_path.stat().st_size < 100_000
    ):
        raise ForgeError(
            f"Preview render failed or suspiciously small: {target.preview_path}"
        )


def _export_glb(target: BuildTarget, objects: Iterable[Any], armature: Any) -> None:
    target.glb_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    export_objects = list(dict.fromkeys([*objects, armature]))
    for obj in export_objects:
        if obj.name in bpy.context.view_layer.objects:
            obj.select_set(True)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.export_scene.gltf(
        filepath=str(target.glb_path),
        export_format="GLB",
        # Export the assembled actor only. RENDER_BOARD, camera and lights are
        # preview staging and must never enter a gameplay GLB.
        use_selection=True,
        export_skins=True,
        export_animations=True,
        export_materials="EXPORT",
        export_apply=False,
        export_image_format="AUTO",
        export_cameras=False,
        export_lights=False,
    )
    if not target.glb_path.exists() or target.glb_path.stat().st_size < 1_000_000:
        raise ForgeError(
            f"GLB export failed or below cinematic size floor: {target.glb_path}"
        )


def _sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _write_qc_receipt(
    target: BuildTarget, objects: list[Any], armature: Any
) -> None:
    verts, tris = _geometry_stats(objects)
    meshes = [o for o in objects if o.type == "MESH"]
    payload = {
        "schemaVersion": 1,
        "status": "PASS",
        "piece": target.slug,
        "privacy": "private-review" if target.private_mode else "approved-output",
        "geometry": {
            "vertices": verts,
            "triangles": tris,
            "meshObjects": len(meshes),
            "uvMappedMeshes": sum(1 for o in meshes if len(o.data.uv_layers) > 0),
        },
        "rig": {
            "armature": armature.name,
            "boneCount": len(armature.data.bones),
            "namespace": "mixamorig:*",
        },
        "outputs": {
            "preview": {
                "file": target.preview_path.name,
                "bytes": target.preview_path.stat().st_size,
                "sha256": _sha256(target.preview_path),
            },
            "glb": {
                "file": target.glb_path.name,
                "bytes": target.glb_path.stat().st_size,
                "sha256": _sha256(target.glb_path),
            },
        },
    }
    target.qc_path.parent.mkdir(parents=True, exist_ok=True)
    target.qc_path.write_text(
        json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8"
    )


def _build_one(
    target: BuildTarget, manifest: dict[str, Any], samples: int
) -> None:
    _reset_scene()
    imported, arm = _import_source_pack(target)

    _validate_piece_law(target, imported)
    _validate_queen_face_policy(target, imported)
    _validate_raw_face_reference_absent(target, imported)
    _validate_geometry(target, imported)
    _validate_mixamo(arm, target.piece)
    rigid = _attach_rigid_parts(target, imported, arm)
    _parent_weighted_to_armature(imported, arm, rigid)
    _apply_faction_look(target, imported, manifest)
    _validate_private_metahuman_identity(target, imported)
    _setup_cinematic_scene(target, imported, samples)
    _render_preview(target)
    _export_glb(target, imported, arm)
    _write_qc_receipt(target, imported, arm)

    print(json.dumps({
        "status": "PASS",
        "piece": target.slug,
        "preview": str(target.preview_path),
        "glb": str(target.glb_path),
        "qc": str(target.qc_path),
        "publication": (
            "private-review" if target.private_mode else "approved-output"
        ),
    }))


def main() -> int:
    argv = (
        sys.argv[sys.argv.index("--") + 1:]
        if "--" in sys.argv
        else sys.argv[1:]
    )
    ns = _args(argv)
    _require_blender()
    manifest = _load_manifest(pathlib.Path(ns.manifest).resolve())
    source_root = pathlib.Path(ns.source_root).resolve()
    output_root = pathlib.Path(ns.output_root).resolve()
    private_root = pathlib.Path(ns.private_review_root).resolve()
    repository_root = pathlib.Path(ns.repository_root).resolve()
    face_image = (
        pathlib.Path(ns.face_image).resolve()
        if ns.face_image
        else None
    )
    _validate_storage_boundaries(
        repository_root, output_root, private_root, face_image
    )

    factions = FACTIONS if ns.faction == "all" else (ns.faction,)
    pieces = PIECE_ORDER if ns.piece == "all" else (ns.piece,)
    failures: list[str] = []

    for faction in factions:
        for piece in pieces:
            spec = manifest["pieces"][piece]
            target = BuildTarget(
                faction=faction,
                piece=piece,
                spec=spec,
                source_root=source_root,
                output_root=output_root,
                private_review_root=private_root,
                face_image=face_image,
            )
            try:
                _build_one(target, manifest, ns.cycles_samples)
            except Exception as exc:
                failures.append(f"{target.slug}: {exc}")
                print(json.dumps({
                    "status": "FAIL",
                    "piece": target.slug,
                    "error": str(exc),
                }), file=sys.stderr)

    if failures:
        raise ForgeError("Forge failed:\n" + "\n".join(failures))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
