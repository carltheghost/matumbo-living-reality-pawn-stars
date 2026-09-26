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
      --private-review-root D:/maTumboChessReview --face-image D:/private/face.jpg

Public/publish output is intentionally harder than private review output. Male
pieces can use Tumbo's local face source for PRIVATE REVIEW, but publishing them
requires the explicit --publish-approved flag. The face source itself is never
copied into the repository by this script.
"""
from __future__ import annotations

import argparse
import json
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
REQUIRED_MIXAMO_BONES = (
    "mixamorig:Hips", "mixamorig:Spine", "mixamorig:Spine1",
    "mixamorig:Spine2", "mixamorig:Neck", "mixamorig:Head",
    "mixamorig:LeftShoulder", "mixamorig:LeftArm", "mixamorig:LeftForeArm",
    "mixamorig:LeftHand", "mixamorig:RightShoulder", "mixamorig:RightArm",
    "mixamorig:RightForeArm", "mixamorig:RightHand", "mixamorig:LeftUpLeg",
    "mixamorig:LeftLeg", "mixamorig:LeftFoot", "mixamorig:RightUpLeg",
    "mixamorig:RightLeg", "mixamorig:RightFoot",
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
    publish_approved: bool

    @property
    def slug(self) -> str:
        return f"{self.faction}-{self.piece}"

    @property
    def is_male(self) -> bool:
        return self.piece in MALE_PIECES

    @property
    def private_mode(self) -> bool:
        # Every Phase 1 piece stays local until Tumbo eyeballs all twelve.
        return not self.publish_approved

    @property
    def preview_path(self) -> pathlib.Path:
        root = self.private_review_root if self.private_mode else self.output_root
        return root / f"preview-{self.slug}.png"

    @property
    def glb_path(self) -> pathlib.Path:
        root = self.private_review_root if self.private_mode else self.output_root
        return root / f"{self.slug}.glb"


def _args(argv: list[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Cinematic chess hybrid asset forge")
    p.add_argument("--manifest", required=True)
    p.add_argument("--faction", choices=FACTIONS + ("all",), default="all")
    p.add_argument("--piece", choices=PIECE_ORDER + ("all",), default="all")
    p.add_argument("--source-root", required=True)
    p.add_argument("--output-root", required=True)
    p.add_argument("--private-review-root", required=True)
    p.add_argument("--face-image")
    p.add_argument("--publish-approved", action="store_true",
                   help="Explicit Tumbo approval gate for face-bearing public artifacts")
    p.add_argument("--cycles-samples", type=int, default=192)
    return p.parse_args(argv)


def _load_manifest(path: pathlib.Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schemaVersion") != 1:
        raise ForgeError("forge manifest schemaVersion must be 1")
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


def _resolve_source(root: pathlib.Path, rel: str) -> pathlib.Path:
    p = (root / rel).resolve()
    if not p.exists():
        raise ForgeError(f"Required sculpt/source asset is missing: {p}")
    return p


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


def _validate_mixamo(armature: Any, piece: str) -> None:
    names = {b.name for b in armature.data.bones}
    missing = [n for n in REQUIRED_MIXAMO_BONES if n not in names]
    if missing:
        raise ForgeError(f"{piece}: Mixamo rig missing bones: {', '.join(missing)}")
    bad = sorted(n for n in names if not n.startswith("mixamorig:"))
    if bad:
        raise ForgeError(f"{piece}: every deform/export bone must use mixamorig:* naming; bad={bad[:8]}")
    if piece == "knight":
        equine = {
            "mixamorig:HorsePelvis", "mixamorig:HorseSpine", "mixamorig:HorseTail01",
            "mixamorig:HorseFrontLegL", "mixamorig:HorseFrontLegR",
            "mixamorig:HorseHindLegL", "mixamorig:HorseHindLegR",
        }
        miss = sorted(equine - names)
        if miss:
            raise ForgeError(f"knight: centaur rig missing equine extension bones: {', '.join(miss)}")


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
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
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
        nt.links.new(node.outputs["Color"], bsdf.inputs[input_name])
    normal_rel = tex.get("normal")
    if normal_rel:
        p = _resolve_source(root, normal_rel)
        image = nt.nodes.new("ShaderNodeTexImage")
        image.name = "TEX_NORMAL"
        image.image = _load_image(p, non_color=True)
        normal = nt.nodes.new("ShaderNodeNormalMap")
        nt.links.new(image.outputs["Color"], normal.inputs["Color"])
        nt.links.new(normal.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


def _assign_material_by_tag(objects: Iterable[Any], tag: str, mat: Any) -> int:
    count = 0
    tag = tag.lower()
    for obj in objects:
        if obj.type != "MESH" or tag not in obj.name.lower():
            continue
        obj.data.materials.clear()
        obj.data.materials.append(mat)
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
            f"{target.slug}: source meshes must expose ARMOR and FILIGREE tagged objects"
        )


def _parent_to_armature(objects: Iterable[Any], armature: Any) -> None:
    for obj in objects:
        if obj.type != "MESH" or obj.parent == armature:
            continue
        groups = {g.name for g in obj.vertex_groups}
        if not groups:
            raise ForgeError(f"{obj.name}: modular cinematic mesh is unweighted")
        obj.parent = armature
        mod = next((m for m in obj.modifiers if m.type == "ARMATURE"), None)
        if mod is None:
            mod = obj.modifiers.new(name="Armature", type="ARMATURE")
        mod.object = armature


def _validate_piece_law(target: BuildTarget, objects: Iterable[Any]) -> None:
    names = " ".join(o.name.lower() for o in objects)
    required = [s.lower() for s in target.spec.get("requiredTags", [])]
    missing = [tag for tag in required if tag not in names]
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
    mesh_count = len([o for o in objects if o.type == "MESH"])
    if mesh_count < int(target.spec.get("minMeshObjects", 8)):
        raise ForgeError(f"{target.slug}: assembly has too few sculpted mesh parts")


def _apply_private_face_preview(target: BuildTarget) -> None:
    if not target.is_male:
        return
    if target.face_image is None or not target.face_image.exists():
        raise ForgeError(
            f"{target.slug}: local face.jpg is required for private male-piece review"
        )
    face_mats = [
        m for m in bpy.data.materials if m.name == "PRIVATE_FACE_PREVIEW"
    ]
    if not face_mats:
        raise ForgeError(
            f"{target.slug}: source face must expose PRIVATE_FACE_PREVIEW material"
        )
    img = _load_image(target.face_image, non_color=False)
    mat = face_mats[0]
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    tex = nt.nodes.get("PRIVATE_FACE_IMAGE") or nt.nodes.new("ShaderNodeTexImage")
    tex.name = "PRIVATE_FACE_IMAGE"
    tex.image = img
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    # Source face.jpg is never copied by this script. Private review outputs
    # live outside the repository; approved publication is a separate command.


def _setup_cinematic_scene(target: BuildTarget, samples: int) -> None:
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = 1024
    scene.render.resolution_y = 1024
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.render.film_transparent = False
    scene.world.color = (0.004, 0.005, 0.008)

    bpy.ops.mesh.primitive_plane_add(size=14, location=(0, 0, 0))
    board = bpy.context.active_object
    board.name = "RENDER_BOARD"
    board.data.materials.append(
        _material("MAT_Board", (0.008, 0.009, 0.014, 1), 0.75, 0.18)
    )

    def area(
        name: str,
        loc: tuple[float, float, float],
        energy: float,
        size: float,
        color: tuple[float, float, float],
    ) -> None:
        data = bpy.data.lights.new(name, type="AREA")
        data.energy = energy
        data.shape = "DISK"
        data.size = size
        data.color = color
        obj = bpy.data.objects.new(name, data)
        bpy.context.collection.objects.link(obj)
        obj.location = loc
        direction = Vector((0, 0, 2.0)) - obj.location
        obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()

    area("Key", (4.5, -5.5, 7.5), 1500, 4.0, (1.0, 0.75, 0.48))
    area("Fill", (-4.0, -2.0, 5.0), 700, 5.0, (0.38, 0.48, 0.72))
    area("Rim", (0.0, 4.5, 6.0), 1300, 3.0, (0.25, 0.42, 1.0))

    cam_data = bpy.data.cameras.new("ForgeCamera")
    cam = bpy.data.objects.new("ForgeCamera", cam_data)
    bpy.context.collection.objects.link(cam)
    scene.camera = cam
    cam.location = (
        (5.8, -8.8, 4.7)
        if target.piece != "knight"
        else (7.3, -10.8, 5.0)
    )
    target_pt = Vector((0, 0, 2.5 if target.piece != "knight" else 2.3))
    cam.rotation_euler = (
        target_pt - cam.location
    ).to_track_quat("-Z", "Y").to_euler()
    cam.data.lens = 68


def _render_preview(target: BuildTarget) -> None:
    target.preview_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.context.scene.render.filepath = str(target.preview_path)
    bpy.ops.render.render(write_still=True)
    if (
        not target.preview_path.exists()
        or target.preview_path.stat().st_size < 50_000
    ):
        raise ForgeError(
            f"Preview render failed or suspiciously small: {target.preview_path}"
        )


def _export_glb(target: BuildTarget) -> None:
    target.glb_path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=str(target.glb_path),
        export_format="GLB",
        use_selection=False,
        export_skins=True,
        export_animations=True,
        export_materials="EXPORT",
        export_apply=False,
        export_image_format="AUTO",
    )
    if not target.glb_path.exists() or target.glb_path.stat().st_size < 1_000_000:
        raise ForgeError(
            f"GLB export failed or below cinematic size floor: {target.glb_path}"
        )


def _build_one(
    target: BuildTarget, manifest: dict[str, Any], samples: int
) -> None:
    _reset_scene()
    imported: list[Any] = []
    source_files = target.spec.get("sources", [])
    if not source_files:
        raise ForgeError(f"{target.slug}: no source sculpts listed")
    for rel in source_files:
        imported.extend(_import_asset(_resolve_source(target.source_root, rel)))

    _validate_piece_law(target, imported)
    _validate_geometry(target, imported)
    arm = _find_armature(imported)
    _validate_mixamo(arm, target.piece)
    _parent_to_armature(imported, arm)
    _apply_faction_look(target, imported, manifest)
    _apply_private_face_preview(target)
    _setup_cinematic_scene(target, samples)
    _render_preview(target)
    _export_glb(target)

    print(json.dumps({
        "status": "PASS",
        "piece": target.slug,
        "preview": str(target.preview_path),
        "glb": str(target.glb_path),
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
    face_image = (
        pathlib.Path(ns.face_image).resolve()
        if ns.face_image
        else None
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
                publish_approved=bool(ns.publish_approved),
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
