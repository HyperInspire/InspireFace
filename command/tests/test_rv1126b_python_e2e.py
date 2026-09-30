import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "command" / "rv1126b_python_e2e" / "python_e2e_runner.py"


class Rv1126bPythonE2ETests(unittest.TestCase):
    def test_load_native_sets_explicit_library_before_import(self):
        spec = importlib.util.spec_from_file_location("rv1126b_python_e2e_runner", RUNNER)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        sentinel = object()
        package = types.ModuleType("native_pkg")
        package.native = sentinel

        with tempfile.TemporaryDirectory() as directory:
            library = Path(directory) / "libInspireFace.so"
            library.touch()
            with mock.patch.dict(sys.modules, {"native_pkg": package}), mock.patch.dict(
                os.environ, {}, clear=True
            ):
                self.assertIs(module.load_native(library), sentinel)
                self.assertEqual(
                    os.environ["INSPIREFACE_LIBRARY_PATH"], str(library.resolve())
                )


if __name__ == "__main__":
    unittest.main()
