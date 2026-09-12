"""Static contracts for the RV1126B resource-pack loader smoke-test tooling."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / "command" / "rv1126b_pack"
VALIDATOR_SOURCE = PACK / "validate_pack_main.cpp"
BUILD_SCRIPT = PACK / "build_validator.sh"
BOARD_SCRIPT = PACK / "run_board_pack_validation.ps1"


class RV1126BPackBoardScriptTest(unittest.TestCase):
    def test_validator_uses_only_the_public_pack_validation_api_and_emits_json(self):
        text = VALIDATOR_SOURCE.read_text(encoding="utf-8")
        self.assertIn('#include "inspireface.h"', text)
        self.assertIn("HFValidateResourcePack", text)
        self.assertIn("HF_RESOURCE_PACK_INFO_VERSION", text)
        self.assertIn('"Gundam_RV1126B"', text)
        self.assertIn("modelCount != 11", text)
        self.assertIn(r'\"status\"', text)
        self.assertIn("peak_rss_kb", text)
        self.assertIn("PrintJsonString", text)
        self.assertNotIn("InspireArchive", text)

    def test_cross_build_enforces_armhf_abi_and_reuses_rv1126b_sdk(self):
        text = BUILD_SCRIPT.read_text(encoding="utf-8")
        self.assertIn("set -euo pipefail", text)
        self.assertIn("build_cross_rv1126b_armhf.sh", text)
        self.assertIn("SDK_INSTALL_DIR", text)
        self.assertIn("RKNN_RUNTIME_DIR", text)
        self.assertIn("arm-linux-gnueabihf", text)
        self.assertIn("hard-float ABI", text)
        self.assertIn("readelf", text)
        self.assertIn("file", text)
        self.assertIn("libInspireFace.so", text)
        self.assertIn("librknnrt.so", text)
        self.assertIn("librknnrt version:", text)
        self.assertNotIn("grep -Eq", text)

    def test_board_script_uses_fixed_serial_validated_directory_and_host_gate(self):
        text = BOARD_SCRIPT.read_text(encoding="utf-8")
        self.assertIn("e3d7377f6fc6d325", text)
        self.assertIn("-s $Serial", text)
        self.assertIn("/userdata/inspireface-rv1126b/pack-validation", text)
        self.assertIn("command.rv1126b_pack.validate_pack", text)
        self.assertIn("[System.IO.Path]::GetRelativePath", text)
        self.assertIn("Resolve-Path -LiteralPath $PackPath", text)
        self.assertIn("'readlink', '-f'", text)
        self.assertIn("rm -rf $RemoteDirectory", text)
        self.assertIn("libInspireFace.so", text)
        self.assertIn("librknnrt.so", text)
        self.assertIn(".partial", text)
        self.assertNotIn("adb push $PackPath /userdata/", text)


if __name__ == "__main__":
    unittest.main()
