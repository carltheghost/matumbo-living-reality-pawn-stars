import importlib.util
import json
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
MODULE = ROOT / "Pipeline" / "Blender" / "forge_contract.py"
MANIFEST = ROOT / "Pipeline" / "Blender" / "forge_manifest.json"

spec = importlib.util.spec_from_file_location("forge_contract", MODULE)
forge_contract = importlib.util.module_from_spec(spec)
spec.loader.exec_module(forge_contract)

class Phase1ForgeContractTests(unittest.TestCase):
    def test_manifest_obeys_character_law(self):
        self.assertEqual(forge_contract.validate(MANIFEST), [])

    def test_all_six_roles_are_required(self):
        data=json.loads(MANIFEST.read_text(encoding="utf-8"))
        self.assertEqual(set(data["pieces"]), set(forge_contract.PIECES))

    def test_public_face_is_closed_by_default(self):
        data=json.loads(MANIFEST.read_text(encoding="utf-8"))
        self.assertFalse(data["privacy"]["maleFacePublicByDefault"])

if __name__ == "__main__":
    unittest.main()
