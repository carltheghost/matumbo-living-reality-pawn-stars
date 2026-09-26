import importlib.util
import json
import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE = ROOT / "Pipeline" / "Blender" / "forge_contract.py"
MANIFEST = ROOT / "Pipeline" / "Blender" / "forge_manifest.json"
FORGE = ROOT / "Pipeline" / "Blender" / "chess_forge.py"
PUBLISH = ROOT / "Pipeline" / "Blender" / "publish_approved.ps1"
RUNNERS = [
    ROOT / "Pipeline" / "Blender" / "run_black.ps1",
    ROOT / "Pipeline" / "Blender" / "run_white.ps1",
]

spec = importlib.util.spec_from_file_location("forge_contract", MODULE)
forge_contract = importlib.util.module_from_spec(spec)
spec.loader.exec_module(forge_contract)

forge_spec = importlib.util.spec_from_file_location("chess_forge_contract", FORGE)
chess_forge = importlib.util.module_from_spec(forge_spec)
sys.modules[forge_spec.name] = chess_forge
forge_spec.loader.exec_module(chess_forge)

class Phase1ForgeContractTests(unittest.TestCase):
    def test_manifest_obeys_character_law(self):
        self.assertEqual(forge_contract.validate(MANIFEST), [])

    def test_all_six_roles_are_required(self):
        data=json.loads(MANIFEST.read_text(encoding="utf-8"))
        self.assertEqual(set(data["pieces"]), set(forge_contract.PIECES))

    def test_public_face_is_closed_by_default(self):
        data=json.loads(MANIFEST.read_text(encoding="utf-8"))
        self.assertFalse(data["privacy"]["maleFacePublicByDefault"])

    def test_manifest_rejects_source_root_escape(self):
        data=json.loads(MANIFEST.read_text(encoding="utf-8"))
        data["pieces"]["king"]["sources"][0]="../private/face.jpg"
        with tempfile.TemporaryDirectory() as tmp:
            manifest=pathlib.Path(tmp) / "manifest.json"
            manifest.write_text(json.dumps(data), encoding="utf-8")
            errors=forge_contract.validate(manifest)
        self.assertTrue(any("unsafe source path" in error for error in errors))

    def test_storage_boundary_logic(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=pathlib.Path(tmp).resolve()
            repo=root / "repo"
            output=repo / "chess" / "glb"
            private=root / "private-review"
            face=root / "private" / "face.jpg"
            chess_forge._validate_storage_boundaries(repo, output, private, face)
            with self.assertRaises(chess_forge.ForgeError):
                chess_forge._validate_storage_boundaries(repo, output, repo / "review", face)
            with self.assertRaises(chess_forge.ForgeError):
                chess_forge._validate_storage_boundaries(repo, output, private, repo / "face.jpg")
            with self.assertRaises(chess_forge.ForgeError):
                chess_forge._validate_storage_boundaries(repo, root / "public", private, face)

    def test_glb_export_excludes_preview_staging(self):
        source=FORGE.read_text(encoding="utf-8")
        self.assertIn("use_selection=True", source)
        self.assertIn("export_cameras=False", source)
        self.assertIn("export_lights=False", source)
        self.assertIn("_export_glb(target, imported, arm)", source)

    def test_private_face_binding_is_scoped_to_current_assembly(self):
        source=FORGE.read_text(encoding="utf-8")
        self.assertIn("_apply_private_face_preview(target, imported)", source)
        self.assertIn("for slot in obj.material_slots", source)
        self.assertNotIn('m for m in bpy.data.materials if m.name == "PRIVATE_FACE_PREVIEW"', source)

    def test_queen_explicitly_rejects_private_male_face_assets(self):
        source=FORGE.read_text(encoding="utf-8")
        self.assertIn("_validate_queen_face_policy(target, imported)", source)
        self.assertIn("queen character law forbids Tumbo/private male-face assets", source)
        self.assertIn("private_face_preview", source.lower())

    def test_deforming_meshes_require_export_rig_weights(self):
        source=FORGE.read_text(encoding="utf-8")
        self.assertIn("valid_group_indices", source)
        self.assertIn("vertices lack export-rig weights", source)
        self.assertIn("obj.matrix_world = world", source)

    def test_private_paths_and_all_twelve_publication_are_enforced(self):
        source=FORGE.read_text(encoding="utf-8")
        self.assertIn("Private review root must stay outside the repository", source)
        self.assertIn("Private face input must stay outside the repository", source)
        for runner in RUNNERS:
            self.assertIn("--repository-root", runner.read_text(encoding="utf-8"))
        publish=PUBLISH.read_text(encoding="utf-8")
        self.assertIn("[ValidateSet('all')]", publish)
        self.assertIn("--repository-root", publish)

if __name__ == "__main__":
    unittest.main()
