#!/usr/bin/env python3

import argparse
import subprocess
import sys
import unittest
from pathlib import Path


EXPECTED = (
    "plain|plain\n"
    "one|O''Brien\n"
    "leading|''start\n"
    "adjacent|a''''b\n"
    "trailing|end''\n"
    "whitespace|  keep  spaces" + "  \n"
)


class GardenTestEscapingTest(unittest.TestCase):
    def test_escapesinglequotes_preserves_all_characters(self):
        root = Path(__file__).resolve().parents[1]
        stylesheet = root / "doc" / "xsl" / "test_gardentest_escaping.xsl"
        result = subprocess.run(
            [self.xsltproc, "--nonet", str(stylesheet), "-"],
            input="<test/>\n",
            text=True,
            capture_output=True,
            check=True,
        )
        self.assertEqual(EXPECTED, result.stdout)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--xsltproc", required=True)
    args, unittest_args = parser.parse_known_args()
    GardenTestEscapingTest.xsltproc = args.xsltproc
    unittest.main(argv=[sys.argv[0], *unittest_args])
