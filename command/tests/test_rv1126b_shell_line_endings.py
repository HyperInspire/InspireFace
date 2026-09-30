import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class RV1126BShellLineEndingTests(unittest.TestCase):
    def test_board_build_scripts_use_lf_line_endings(self):
        scripts = [ROOT / "command" / "build_cross_rv1126b_armhf.sh"]
        scripts.extend(sorted((ROOT / "command").glob("rv1126b*/**/*.sh")))

        offenders = [
            script.relative_to(ROOT).as_posix()
            for script in scripts
            if b"\r\n" in script.read_bytes()
        ]

        self.assertEqual([], offenders, "CRLF breaks these scripts under Linux bash")


if __name__ == "__main__":
    unittest.main()
