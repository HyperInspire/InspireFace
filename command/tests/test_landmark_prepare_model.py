import importlib.util
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "rv1126b_landmark" / "prepare_model.py"


class LandmarkDatasetTest(unittest.TestCase):
    def test_quantization_paths_are_made_absolute_from_dataset_directory(self):
        spec = importlib.util.spec_from_file_location("prepare_model", MODULE_PATH)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            source.mkdir()
            images = source / "images"
            images.mkdir()
            (images / "a.png").touch()
            (images / "b.png").touch()
            dataset = source / "dataset.txt"
            dataset.write_text("images/a.png\nimages/b.png\n")
            output = root / "absolute.txt"
            module.make_absolute_dataset(dataset, output)
            self.assertEqual(output.read_text().splitlines(), [
                str((source / "images/a.png").resolve()),
                str((source / "images/b.png").resolve()),
            ])


if __name__ == "__main__":
    unittest.main()
