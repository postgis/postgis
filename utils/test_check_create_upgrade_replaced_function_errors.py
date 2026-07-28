#!/usr/bin/env python3

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("check_create_upgrade_replaced_function_errors.sh")


class CreateUpgradeReplacedFunctionErrorsTest(unittest.TestCase):
    def test_missing_tmpdir_parent_falls_back_to_system_temp(self):
        shell = shutil.which("sh")
        if shell is None:
            self.skipTest("test requires a POSIX sh on PATH")

        with tempfile.TemporaryDirectory() as temporary_directory:
            missing_tmpdir = Path(temporary_directory) / "missing" / "tmp"
            environment = os.environ.copy()
            environment["TMPDIR"] = str(missing_tmpdir)
            environment["srcdir"] = str(SCRIPT.parent)

            result = subprocess.run(
                [shell, str(SCRIPT)],
                cwd=SCRIPT.parent,
                env=environment,
                text=True,
                capture_output=True,
            )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
