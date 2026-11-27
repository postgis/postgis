#!/usr/bin/env python3
"""Exercise multiline routine signatures in unpackaged conversion."""

import pathlib
import subprocess
import unittest

SCRIPT = pathlib.Path(__file__).with_name("create_unpackaged.pl")


class RoutineSignatures(unittest.TestCase):
    def test_nested_defaults_and_following_arguments(self):
        sql = """\
CREATE OR REPLACE PROCEDURE example(
 a integer DEFAULT greatest(1, 2),
 b text DEFAULT 'a,b'
) LANGUAGE plpgsql AS $$ BEGIN NULL; END $$;
CREATE OR REPLACE FUNCTION another(
 a integer DEFAULT greatest(3, 4),
 b text DEFAULT 'c,d'
) RETURNS integer LANGUAGE sql AS $$ SELECT 1 $$;
"""
        result = subprocess.run(
            ["perl", str(SCRIPT), "postgis"], input=sql, text=True,
            capture_output=True, check=True,
        ).stdout
        normalized = " ".join(result.split())
        self.assertIn("'example ( a integer , b text )'", normalized)
        self.assertIn("'another ( a integer , b text )'", normalized)
        self.assertIn("p.prokind", result)
        self.assertIn("WHEN 'p' THEN 'PROCEDURE'", result)
        self.assertIn("WHEN 'a' THEN 'AGGREGATE'", result)
        self.assertIn("ALTER %s %s OWNER TO %I", result)

    def test_incomplete_signature_fails(self):
        result = subprocess.run(
            ["perl", str(SCRIPT), "postgis"],
            input="CREATE PROCEDURE broken(\n a integer\n", text=True,
            capture_output=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Unexpected end of input", result.stderr)


if __name__ == "__main__":
    unittest.main()
