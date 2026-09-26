#!/usr/bin/env python3
"""Static contract validator for the Phase 1 forge manifest.
Runs in normal Python; no Blender dependency."""
from __future__ import annotations
import argparse, json, pathlib

PIECES = ("king","queen","bishop","knight","rook","pawn")
LAW = {
    "king": {"crown","cape","royal_staff","sword_hip"},
    "queen": {"feminine","elegant_crown","cape","crook_staff","long_hair"},
    "knight": {"centaur","horse_head_helm","horse_barrel","four_legs","hooves","tail","sword","tower_shield"},
    "bishop": {"tall_mitre","crook_staff","cape"},
    "rook": {"tower_pauldrons","tower_shield","cape","massive_castle_staff","battlements","side_turrets","gate"},
    "pawn": {"kettle_helm","brim","spear"},
}
RIGID = {
    "king": {"crown","royal_staff","sword_hip","metahuman_tumbo"},
    "queen": {"elegant_crown","crook_staff","metahuman_queen"},
    "bishop": {"tall_mitre","crook_staff","metahuman_tumbo"},
    "knight": {"horse_head_helm","sword","tower_shield","metahuman_tumbo"},
    "rook": {"tower_shield","massive_castle_staff","metahuman_tumbo"},
    "pawn": {"kettle_helm","spear","metahuman_tumbo"},
}
IDENTITY = {
    "king": "metahuman_tumbo",
    "queen": "metahuman_queen",
    "bishop": "metahuman_tumbo",
    "knight": "metahuman_tumbo",
    "rook": "metahuman_tumbo",
    "pawn": "metahuman_tumbo",
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


def validate(path: pathlib.Path) -> list[str]:
    m=json.loads(path.read_text(encoding="utf-8")); errors=[]
    if m.get("schemaVersion") != 1: errors.append("schemaVersion must be 1")
    if set(m.get("pieces",{})) != set(PIECES): errors.append("pieces must be exactly six canonical roles")
    for p in PIECES:
        s=m.get("pieces",{}).get(p,{})
        tags=set(s.get("requiredTags",[]))
        miss=LAW[p]-tags
        if miss: errors.append(f"{p}: missing law tags {sorted(miss)}")
        if s.get("sourceMode") != "sculpted-hybrid": errors.append(f"{p}: sourceMode must be sculpted-hybrid")
        sources=s.get("sources",[])
        if not sources: errors.append(f"{p}: no sculpted source files")
        rig_source=s.get("rigSource")
        if not rig_source or rig_source not in sources:
            errors.append(f"{p}: rigSource must name one sculpt source")
        elif not _safe_relative(rig_source):
            errors.append(f"{p}: unsafe rigSource {rig_source!r}")
        if IDENTITY[p] not in tags:
            errors.append(f"{p}: missing private MetaHuman identity tag {IDENTITY[p]!r}")
        for source in sources:
            if not _safe_relative(source):
                errors.append(f"{p}: unsafe source path {source!r}")
            if pathlib.PurePosixPath(str(source).replace("\\", "/")).name.lower() == "face.jpg":
                errors.append(f"{p}: raw face.jpg may never be a forge source")
        source_text=" ".join(str(x).lower().replace("\\", "/") for x in sources)
        if p == "queen" and any(token in source_text for token in ("tumbo_metahuman", "tumbo_face", "male_face")):
            errors.append("queen: Tumbo/male likeness source is forbidden")
        if p != "queen" and "faces/tumbo_metahuman_head.fbx" not in source_text:
            errors.append(f"{p}: missing Tumbo MetaHuman head source")
        if p == "queen" and "faces/queen_regal_metahuman_head.fbx" not in source_text:
            errors.append("queen: missing dedicated feminine MetaHuman head source")
        if int(s.get("minTriangles",0)) < 80000: errors.append(f"{p}: cinematic triangle floor < 80k")
        rigid=set(s.get("rigidMounts",{}))
        missing_rigid=RIGID[p]-rigid
        if missing_rigid: errors.append(f"{p}: missing rigid mounts {sorted(missing_rigid)}")
    expected_surface={"black":"obsidian","white":"ivory"}
    for faction in ("black","white"):
        fs=m.get("materials",{}).get(faction,{})
        if "gold" not in fs or "armor" not in fs: errors.append(f"{faction}: missing armor/gold material spec")
        armor=fs.get("armor",{})
        if armor.get("surfaceClass") != expected_surface[faction]:
            errors.append(f"{faction}: armor surfaceClass must be {expected_surface[faction]}")
        if float(armor.get("metallic",1.0)) > 0.05:
            errors.append(f"{faction}: {expected_surface[faction]} armor must be dielectric, not metallic")
        if fs.get("gold",{}).get("surfaceClass") != "gold":
            errors.append(f"{faction}: filigree surfaceClass must be gold")
        for material_name, floor in (("armor",4096),("gold",4096),("cloth",2048)):
            ms=fs.get(material_name,{})
            if int(ms.get("minResolution",0)) < floor:
                errors.append(f"{faction}/{material_name}: texture floor below {floor}px")
            tex=ms.get("textures",{})
            for slot, source in tex.items():
                if not _safe_relative(source):
                    errors.append(f"{faction}/{material_name}/{slot}: unsafe texture path {source!r}")
                if pathlib.PurePosixPath(str(source).replace("\\", "/")).name.lower() == "face.jpg":
                    errors.append(f"{faction}/{material_name}/{slot}: raw face.jpg texture is forbidden")
            if material_name in {"armor","gold"} and not {"baseColor","roughness","metallic","normal"}.issubset(tex):
                errors.append(f"{faction}/{material_name}: incomplete PBR texture set")
    if m.get("privacy",{}).get("maleFacePublicByDefault", True): errors.append("maleFacePublicByDefault must be false")
    return errors

def main():
    ap=argparse.ArgumentParser();ap.add_argument("manifest");ns=ap.parse_args()
    e=validate(pathlib.Path(ns.manifest))
    if e:
        print("FAIL"); [print("-",x) for x in e]; raise SystemExit(1)
    print("PASS: Phase 1 cinematic forge manifest contract")
if __name__=="__main__": main()
