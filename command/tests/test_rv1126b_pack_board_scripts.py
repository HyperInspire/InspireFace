"""Static contracts for the RV1126B resource-pack loader smoke-test tooling."""

from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
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
        self.assertIn("'rm', '-rf', $RemoteDirectory", text)
        self.assertIn("libInspireFace.so", text)
        self.assertIn("librknnrt.so", text)
        self.assertIn(".partial", text)
        self.assertNotIn("adb push $PackPath /userdata/", text)


@unittest.skipUnless(os.name == "posix", "Linux build-host behavior")
class BuildValidatorBehaviorTest(unittest.TestCase):
    def test_real_build_script_checks_attributes_and_minimum_version(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sdk = root / "sdk"
            (sdk / "include").mkdir(parents=True)
            (sdk / "lib").mkdir()
            for name in ("include/inspireface.h", "lib/libInspireFace.so", "lib/librknnrt.so"):
                (sdk / name).write_text("fixture")
            binaries = root / "bin"
            binaries.mkdir()
            scripts = {
                "arm-linux-gnueabihf-g++": '#!/bin/bash\nif [[ "$1" == -dumpmachine ]]; then echo arm-linux-gnueabihf; exit; fi\nwhile [[ $# -gt 0 ]]; do if [[ "$1" == -o ]]; then printf binary > "$2"; break; fi; shift; done\n',
                "arm-linux-gnueabihf-readelf": '#!/bin/bash\ncase "$1" in\n-h) printf "Class: ELF32\\nMachine: ARM\\nFlags: Version5 EABI, hard-float ABI\\n";;\n-A) echo "Tag_ABI_VFP_args: ${FAKE_VFP:-VFP registers}";;\n-d) printf "Shared library: [libInspireFace.so]\\nShared library: [librknnrt.so]\\n";;\nesac\n',
                "strings": '#!/bin/bash\necho "librknnrt version: $FAKE_VERSION"\n',
            }
            for name, body in scripts.items():
                path = binaries / name
                path.write_text(body)
                path.chmod(0o755)
            env = dict(os.environ, PATH=str(binaries) + os.pathsep + os.environ["PATH"], SDK_INSTALL_DIR=str(sdk))
            for version, accepted in (("2.3.0", False), ("2.3.1", False), ("2.3.2", True), ("2.4.0", True)):
                with self.subTest(version=version):
                    result = subprocess.run(["bash", str(BUILD_SCRIPT), str(root / "out")], env=dict(env, FAKE_VERSION=version), capture_output=True, text=True)
                    self.assertEqual(result.returncode == 0, accepted, result.stdout + result.stderr)
            result = subprocess.run(["bash", str(BUILD_SCRIPT), str(root / "out")], env=dict(env, FAKE_VERSION="2.3.2", FAKE_VFP="base registers"), capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)

    def test_cpp_rejects_wrong_archive_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "inspireface.h").write_text('''#include <cstdlib>
#include <cstring>
#include <cstdio>
using HFStatus = int;
constexpr int HERR_INVALID_PARAM = 1, HSUCCEED = 0, HF_RESOURCE_PACK_INFO_VERSION = 1;
constexpr int HF_LOG_NONE = 0;
static bool logging = true;
inline int HFSetLogLevel(int) { logging = false; return 0; }
struct HFResourcePackInfo { unsigned structSize, structVersion, archiveFileCount, modelCount; char tag[64], version[64], major[64], releaseDate[64]; };
inline int HFValidateResourcePack(const char*, HFResourcePackInfo* p) {
if (logging) std::puts("== Load resource pack ==");
p->archiveFileCount = std::getenv("BAD_COUNT") ? 11 : 12; p->modelCount = 11;
std::strcpy(p->tag, "Gundam_RV1126B");
std::strcpy(p->version, std::getenv("BAD_VERSION") ? "3.0" : "4.0");
std::strcpy(p->major, std::getenv("BAD_MAJOR") ? "t3" : "t4"); return 0;
}
''')
            binary = root / "validator"
            subprocess.run(["g++", "-std=c++14", "-I" + str(root), str(VALIDATOR_SOURCE), "-o", str(binary)], check=True, capture_output=True)
            for mutation in (None, "BAD_COUNT", "BAD_VERSION", "BAD_MAJOR"):
                with self.subTest(mutation=mutation):
                    env = dict(os.environ)
                    if mutation:
                        env[mutation] = "1"
                    result = subprocess.run([str(binary), "pack"], env=env, capture_output=True, text=True)
                    self.assertEqual(result.returncode == 0, mutation is None)
                    self.assertTrue(result.stdout.startswith("{"), "SDK logging must not prefix the JSON result")
                    self.assertEqual(json.loads(result.stdout)["status"], "success" if mutation is None else "failed")


@unittest.skipUnless(shutil.which("pwsh"), "PowerShell host required")
class BoardScriptBehaviorTest(unittest.TestCase):
    def setUp(self):
        from command.tests.test_rv1126b_pack_validation import RV1126BPackValidationTests
        self.fixture = RV1126BPackValidationTests()
        self.fixture.setUp()
        self.addCleanup(self.fixture.tearDown)
        (ROOT / "build").mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / "build")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for source in (self.fixture.pack, self.fixture.report):
            shutil.copy2(source, self.root / source.name)
        self.validator = self.root / "validator"
        self.validator.mkdir()
        for name in ("validate_pack_main", "libInspireFace.so", "librknnrt.so"):
            (self.validator / name).write_text("fixture")
        (self.validator / "librknnrt.so").write_bytes(b"binary\0librknnrt version: 2.3.2\0")
        self.fake = self.root / "fake_adb.py"
        self.fake.write_text('''import json, os, pathlib, shutil, sys
root = pathlib.Path(os.environ["FAKE_ROOT"])
args = sys.argv[1:]
with (root / "argv.jsonl").open("a") as log: log.write(json.dumps(args)+"\\n")
assert args[:2] == ["-s", "e3d7377f6fc6d325"], args
args = args[2:]
mode = os.environ.get("FAKE_MODE", "success")
remote = root / "remote"
if args == ["get-state"]:
    if mode == "offline": sys.exit(1)
    print("device")
elif args[:3] == ["shell", "readlink", "-f"]: print(args[3])
elif args[:3] == ["shell", "rm", "-rf"]:
    assert args == ["shell", "rm", "-rf", "/userdata/inspireface-rv1126b/pack-validation"]
    if remote.exists(): shutil.rmtree(remote)
elif args[:3] == ["shell", "mkdir", "-p"]: remote.mkdir(exist_ok=True)
elif args[0] == "push": shutil.copyfile(args[1], remote / pathlib.PurePosixPath(args[2]).name)
elif args[0] == "pull":
    source = remote / pathlib.PurePosixPath(args[1]).name
    shutil.copyfile(source, args[2])
    if mode == "hash" and source.name == "pack": pathlib.Path(args[2]).write_bytes(b"tampered")
elif args[:2] == ["shell", "chmod"]: pass
elif args[:2] == ["shell", "sh"]:
    assert args == ["shell", "sh", "/userdata/inspireface-rv1126b/pack-validation/run.sh"], args
    script = (remote / "run.sh").read_bytes()
    assert b"\\r" not in script and b"sha256sum" not in script and b"strings" not in script
    result = dict(status="success", sdk_status=0, archive_file_count=12, model_count=11, tag="Gundam_RV1126B", version="4.0", major="t4", release_date="", peak_rss_kb=100, error="")
    if mode == "metadata": result["archive_file_count"] = 11
    if mode == "version": result["version"] = "3.0"
    if mode == "major": result["major"] = "t3"
    (remote / "result.json").write_text(json.dumps(result))
    (remote / "exit-status.txt").write_text("" if mode == "empty_exit" else "0")
    (remote / "runtime.txt").write_text("" if mode == "runtime" else "librknnrt version: 2.3.2")
    (remote / "driver.txt").write_text("" if mode == "driver" else "0.9.8")
else: raise AssertionError(args)
''')

    def run_board(self, mode):
        (self.validator / "librknnrt.so").write_bytes(b"no version" if mode == "runtime" else b"binary\0librknnrt version: 2.3.2\0")
        results = self.root / "results"
        results.mkdir(exist_ok=True)
        (results / "board-result.json").write_text('{"status":"success","run_id":"stale"}')
        def quote(value):
            return "'" + str(value).replace("'", "''") + "'"
        script = "function global:adb { & python " + quote(self.fake) + " @args }; & " + quote(BOARD_SCRIPT)
        script += " -PackPath " + quote(self.root / "Gundam_RV1126B") + " -ValidatorDirectory " + quote(self.validator) + " -ResultDirectory " + quote(results)
        completed = subprocess.run(["pwsh", "-NoProfile", "-Command", script], env=dict(os.environ, FAKE_ROOT=str(self.root), FAKE_MODE=mode), capture_output=True, text=True, encoding="utf-8")
        return completed, results

    def test_fake_adb_runs_without_remote_hash_tool_or_shell_c(self):
        result, folder = self.run_board("success")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        record = json.loads((folder / "board-result.json").read_text(encoding="utf-8-sig"))
        self.assertEqual(record["status"], "success")
        self.assertNotEqual(record["run_id"], "stale")
        argv = [json.loads(line) for line in (self.root / "argv.jsonl").read_text().splitlines()]
        self.assertFalse(any("-c" in args for args in argv))
        pulls = [args for args in argv if args[2] == "pull" and args[3].endswith("/pack")]
        self.assertEqual(len(pulls), 1)
        self.assertIn(record["run_id"], pulls[0][4])

    def test_failures_never_reuse_old_success(self):
        for mode in ("offline", "hash", "metadata", "version", "major", "runtime", "driver", "empty_exit"):
            with self.subTest(mode=mode):
                result, folder = self.run_board(mode)
                self.assertNotEqual(result.returncode, 0)
                record = json.loads((folder / "board-result.json").read_text(encoding="utf-8-sig"))
                self.assertEqual(record["status"], "failed", result.stdout + result.stderr)
                self.assertNotEqual(record["run_id"], "stale")
                self.assertTrue(record["error"])


if __name__ == "__main__":
    unittest.main()
