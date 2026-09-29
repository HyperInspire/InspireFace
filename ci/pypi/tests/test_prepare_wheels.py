import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest
import zipfile


def load_module(name):
    path = Path(__file__).resolve().parents[1] / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


prepare = load_module("prepare_wheels")
source = load_module("check_source_run")
VERSION = "1.2.4.post1"


class WheelCollectionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.artifacts = self.root / "artifacts"
        self.output = self.root / "dist"
        for name in prepare.PLATFORMS:
            self.make_wheel(name)

    def make_wheel(self, artifact, *, suffix="", version=VERSION, metadata_version=VERSION,
                   purelib=False, native=True):
        platform, library = prepare.PLATFORMS[artifact]
        folder = self.artifacts / (artifact + suffix)
        folder.mkdir(parents=True, exist_ok=True)
        path = folder / f"inspireface-{version}-py3-none-{platform}.whl"
        info = f"inspireface-{version}.dist-info/"
        with zipfile.ZipFile(path, "w") as wheel:
            wheel.writestr(info + "METADATA", f"Name: inspireface\nVersion: {metadata_version}\n")
            wheel.writestr(info + "WHEEL", f"Root-Is-Purelib: false\nTag: py3-none-{platform}\n")
            if native:
                prefix = f"inspireface-{version}.data/purelib/" if purelib else ""
                wheel.writestr(prefix + "inspireface/modules/core/libs/" + library, b"native-library")
        return path

    def collect(self, legacy=None):
        return prepare.collect_wheels(self.artifacts, self.output, VERSION, legacy)

    def test_collects_four_distinct_platforms(self):
        self.make_wheel("macos-arm64-wheels", purelib=True)
        selected = self.collect()
        self.assertEqual(len(selected), 4)
        for path in selected:
            self.assertEqual(path.read_bytes(), (self.output / path.name).read_bytes())

    def test_legacy_matrix_selects_only_explicit_python_version(self):
        for artifact in ("macos-arm64-wheels", "macos-x86_64-wheels"):
            shutil.rmtree(self.artifacts / artifact)
            for version in ("3.8", "3.9", "3.10", "3.11", "3.12"):
                self.make_wheel(artifact, suffix=f"-py{version}", purelib=True)
        selected = self.collect("3.12")
        self.assertEqual(len(selected), 4)
        for path in selected:
            if path.parent.name.startswith("macos-"):
                self.assertTrue(path.parent.name.endswith("-py3.12"))

    def test_rejects_missing_platform_before_copying(self):
        shutil.rmtree(self.artifacts / "macos-x86_64-wheels")
        with self.assertRaisesRegex(ValueError, "exactly one artifact"):
            self.collect()
        self.assertFalse(self.output.exists())

    def test_rejects_duplicate_artifact_candidates(self):
        self.make_wheel("macos-arm64-wheels", suffix="-py3.12")
        with self.assertRaisesRegex(ValueError, "exactly one artifact"):
            self.collect("3.12")

    def test_rejects_multiple_wheels_in_one_artifact(self):
        self.make_wheel("macos-arm64-wheels", version="1.2.4.post2")
        with self.assertRaisesRegex(ValueError, "Expected one wheel"):
            self.collect()

    def test_rejects_wrong_release(self):
        with self.assertRaisesRegex(ValueError, "Expected inspireface-1.2.4.post2"):
            prepare.collect_wheels(self.artifacts, self.output, "1.2.4.post2")

    def test_rejects_metadata_version_mismatch(self):
        self.make_wheel("macos-arm64-wheels", metadata_version="1.2.3")
        with self.assertRaisesRegex(ValueError, "metadata does not match"):
            self.collect()

    def test_rejects_corrupt_zip_central_directory(self):
        path = self.make_wheel("macos-arm64-wheels")
        path.write_bytes(path.read_bytes().replace(b"PK\x01\x02", b"BAD!", 1))
        with self.assertRaisesRegex(ValueError, "Bad magic number for central directory"):
            self.collect()
        self.assertFalse(self.output.exists())

    def test_rejects_crc_corruption(self):
        path = self.make_wheel("macos-arm64-wheels")
        path.write_bytes(path.read_bytes().replace(b"native-library", b"broken-library"))
        with self.assertRaisesRegex(ValueError, "ZIP CRC check failed"):
            self.collect()

    def test_rejects_missing_native_library(self):
        self.make_wheel("macos-arm64-wheels", native=False)
        with self.assertRaisesRegex(ValueError, "native library"):
            self.collect()

    def test_does_not_overwrite_existing_release_files(self):
        self.output.mkdir()
        existing = self.output / "existing.whl"
        existing.write_bytes(b"keep")
        with self.assertRaisesRegex(ValueError, "must be empty"):
            self.collect()
        self.assertEqual(existing.read_bytes(), b"keep")


class SourceRunTests(unittest.TestCase):
    def test_allows_failed_upload_from_completed_build(self):
        source.validate_source_run(self.run_data(), "HyperInspire/InspireFace")

    def run_data(self):
        return {"path": ".github/workflows/build_wheels.yaml", "status": "completed",
                "conclusion": "failure", "event": "push",
                "head_repository": {"full_name": "HyperInspire/InspireFace"}}

    def test_rejects_unrelated_or_unfinished_runs(self):
        for key, value in [("path", ".github/workflows/build.yaml"),
                           ("status", "in_progress"), ("event", "pull_request"),
                           ("head_repository", {"full_name": "other/InspireFace"})]:
            with self.subTest(key=key):
                run = self.run_data()
                run[key] = value
                with self.assertRaises(ValueError):
                    source.validate_source_run(run, "HyperInspire/InspireFace")


if __name__ == "__main__":
    unittest.main()
