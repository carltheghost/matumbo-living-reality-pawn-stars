# Elisa Phase 0 Asset Pipeline

## Target

Unreal Engine 5.5.4 cinematic chess rebuild.

Pipeline: Blender -\> FBX/GLTF -\> Unreal Engine 5.5.4 -\> Character
Entity Framework

## Goals

-   Create consistent asset naming.
-   Prepare armor and hard-surface asset imports.
-   Keep MetaHuman integration points clean.

## Design Rules

Male human-headed pieces: - Approved identity pipeline.

Queens: - Separate feminine identity.

Knights: - Full centaur architecture: - warrior torso - horse body -
armor sockets

Rooks: - Castle-staff attachment system.

## Naming

Characters: CHR\_\[Faction\]*\[Piece\]*\[Variant\]

Armor: ARM\_\[Faction\]*\[Set\]*\[Part\]

Weapons: WPN\_\[Piece\]\_\[Name\]

Materials: MAT\_\[Faction\]\_\[Surface\]
