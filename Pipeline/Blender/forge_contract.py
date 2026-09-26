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
    "king": {"crown","royal_staff","sword_hip"},
    "queen": {"elegant_crown","crook_staff"},
    "bishop": {"tall_mitre","crook_staff"},
    "knight": {"horse_head_helm","sword","tower_shield"},
    "rook": {"tower_shield","massive_castle_staff"},
    "pawn": {"kettle_helm","spear"},
}


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
        if not s.get("sources"): errors.append(f"{p}: no sculpted source files")
        if int(s.get("minTriangles",0)) < 80000: errors.append(f"{p}: cinematic triangle floor < 80k")
        rigid=set(s.get("rigidMounts",{}))
        missing_rigid=RIGID[p]-rigid
        if missing_rigid: errors.append(f"{p}: missing rigid mounts {sorted(missing_rigid)}")
    for faction in ("black","white"):
        fs=m.get("materials",{}).get(faction,{})
        if "gold" not in fs or "armor" not in fs: errors.append(f"{faction}: missing armor/gold material spec")
        for material_name, floor in (("armor",4096),("gold",4096),("cloth",2048)):
            ms=fs.get(material_name,{})
            if int(ms.get("minResolution",0)) < floor:
                errors.append(f"{faction}/{material_name}: texture floor below {floor}px")
            tex=ms.get("textures",{})
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
