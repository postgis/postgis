#!/usr/bin/env python3

import json
import os
import re
import subprocess
import tempfile
from pathlib import Path
import unittest


class FakePostgresTools:
    def __init__(self, repo_root):
        self.repo_root = repo_root
        self.current_version = self._read_current_version()
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        self.sharedir = self.root / "pg_sharedir"
        self.extension_dir = self.sharedir / "extension"
        self.contrib_dir = self.sharedir / "contrib"
        self.bin_dir = self.root / "bin"
        self.gmake_log = self.root / "gmake.log"
        self.psql_log = self.root / "psql.log"
        self.extension_dir.mkdir(parents=True)
        self.contrib_dir.mkdir(parents=True)
        self.bin_dir.mkdir(parents=True)
        self.path_map = {}
        self.fail_patterns = {}
        self._write_fake_commands()

    def _write_fake_commands(self):
        pg_config = """#!/usr/bin/env bash
set -eu

case "$1" in
  --version)
    echo "$CHECK_ALL_UPGRADES_PG_VERSION"
    ;;
  --sharedir)
    echo "$CHECK_ALL_UPGRADES_SHAREDIR"
    ;;
  *)
    echo "$CHECK_ALL_UPGRADES_SHAREDIR"
    ;;
esac
"""

        psql = """#!/usr/bin/env python3
import json
import os
import re
import sys

args = sys.argv[1:]
query = ""

for i, arg in enumerate(args):
    if arg in ("-c", "--command") and i + 1 < len(args):
        query = args[i + 1]
        break
if not query:
    query = " ".join(args).strip()
query = " ".join(query.split())

log = os.environ.get("CHECK_ALL_UPGRADES_PSQL_LOG")
if log:
    with open(log, "a", encoding="utf-8") as fp:
        fp.write(query + "\\n")

if re.search("select default_version", query, flags=re.I):
    print(os.environ["CHECK_ALL_UPGRADES_AUTO_VERSION"])
    sys.exit(0)

m = re.search(
    r"pg_extension_update_paths\\(\\s*'([^']+)'\\s*\\)\\s*.*\\s*source\\s*=\\s*'([^']+)'\\s*.*\\s*target\\s*=\\s*'([^']+)'",
    query,
    flags=re.I | re.S,
)
if not m:
    print(query)
    sys.exit(1)

extension, source, target = m.groups()
path_map = json.loads(os.environ.get("CHECK_ALL_UPGRADES_PSQL_PATH_MAP", "{}"))
path = path_map.get(f"{extension}|{source}|{target}")
if path is not None:
    print(path)
    sys.exit(0)

sys.exit(0)
"""

        gmake = """#!/usr/bin/env python3
import json
import os
import sys

argv = sys.argv[1:]
cmd = " ".join(argv)
runflags = os.environ.get("RUNTESTFLAGS", "")
full = " ".join([cmd, runflags]).strip()

log = os.environ.get("CHECK_ALL_UPGRADES_GMAKE_LOG")
if log:
    with open(log, "a", encoding="utf-8") as fp:
        json.dump({"argv": argv, "cmd": cmd}, fp)
        fp.write("\\n")

fail_patterns = os.environ.get("CHECK_ALL_UPGRADES_GMAKE_FAIL", "{}")
for fragment, code in json.loads(fail_patterns).items():
    if fragment in full:
        sys.exit(int(code))

sys.exit(0)
"""

        for name, body in {
            "pg_config": pg_config,
            "psql": psql,
            "gmake": gmake,
            "make": gmake,
        }.items():
            script = self.bin_dir / name
            script.write_text(body + "\n", encoding="utf-8")
            script.chmod(0o755)

    def _read_current_version(self):
        parts = []
        version_keys = (
            "POSTGIS_MAJOR_VERSION",
            "POSTGIS_MINOR_VERSION",
            "POSTGIS_MICRO_VERSION",
        )
        for line in (self.repo_root / "Version.config").read_text(encoding="utf-8").splitlines():
            key, _, value = line.partition("=")
            if key in version_keys:
                parts.append(value.strip())
        return ".".join(parts)

    def create_extension_files(self, extension, versions):
        for version in versions:
            (self.extension_dir / f"{extension}--{version}.sql").write_text("--", encoding="utf-8")

    def create_contrib_files(self, versions):
        for version in versions:
            (self.contrib_dir / f"postgis-{version}").write_text("--", encoding="utf-8")

    def set_path_map(self, mapping):
        self.path_map = mapping

    def set_fail_patterns(self, mapping):
        self.fail_patterns = mapping

    def env(self):
        env = os.environ.copy()
        env["PATH"] = f"{self.bin_dir}:{env['PATH']}"
        env["CHECK_ALL_UPGRADES_SHAREDIR"] = str(self.sharedir)
        env["CHECK_ALL_UPGRADES_PG_VERSION"] = "PostgreSQL 16.2 (Debian)"
        env["CHECK_ALL_UPGRADES_AUTO_VERSION"] = self.current_version
        env["CHECK_ALL_UPGRADES_PSQL_PATH_MAP"] = json.dumps(self.path_map)
        env["CHECK_ALL_UPGRADES_GMAKE_FAIL"] = json.dumps(self.fail_patterns)
        env["CHECK_ALL_UPGRADES_PSQL_LOG"] = str(self.psql_log)
        env["CHECK_ALL_UPGRADES_GMAKE_LOG"] = str(self.gmake_log)
        return env

    def run(self, to_version, args=(), extra_env=None):
        if self.gmake_log.exists():
            self.gmake_log.unlink()
        if self.psql_log.exists():
            self.psql_log.unlink()

        env = self.env()
        if extra_env:
            env.update(extra_env)

        return subprocess.run(
            ["bash", str(self.repo_root / "utils/check_all_upgrades.sh"), *args, to_version],
            cwd=self.repo_root,
            env=env,
            text=True,
            capture_output=True,
        )

    def make_calls(self):
        if not self.gmake_log.exists():
            return []
        return [json.loads(line) for line in self.gmake_log.read_text(encoding="utf-8").splitlines() if line.strip()]

    def close(self):
        self.temp_dir.cleanup()


class CheckAllUpgradesHarnessTest(unittest.TestCase):
    def setUp(self):
        self.repo_root = Path(__file__).resolve().parents[1]
        self.harness = FakePostgresTools(self.repo_root)

    def tearDown(self):
        self.harness.close()

    def test_default_exhaustive_runs_packaged_and_unpacked_paths(self):
        current_version = self.harness.current_version
        self.harness.create_extension_files("postgis", ["3.5.0", "3.6.0", current_version])
        self.harness.create_extension_files("postgis_topology", [current_version])
        self.harness.create_contrib_files(["3.6.0"])
        self.harness.set_path_map(
            {
                f"postgis|3.5.0|{current_version}": "up",
                f"postgis|3.6.0|{current_version}": "up",
                f"postgis|unpackaged|{current_version}": "up",
                f"postgis_topology|{current_version}|{current_version}": "up",
                f"postgis_topology|unpackaged|{current_version}": "up",
            }
        )

        result = self.harness.run(current_version)
        output = result.stdout + result.stderr

        self.assertEqual(0, result.returncode)
        self.assertIn(f"Testing postgis extension upgrade 3.5.0--{current_version}", output)
        self.assertIn(f"Testing postgis extension upgrade 3.6.0--{current_version}", output)
        self.assertIn("Testing postgis script soft upgrade", output)
        self.assertIn("Testing postgis script hard upgrade", output)

        calls = self.harness.make_calls()
        self.assertTrue(calls)

    def test_oldest_uses_semantic_ordering_for_3_9_vs_3_10(self):
        self.harness.create_extension_files("postgis", ["3.10.0", "3.9.0", "3.11.0"])
        self.harness.create_contrib_files(["3.10.0"])
        self.harness.set_path_map(
            {
                "postgis|3.10.0|3.11.0": "up",
                "postgis|3.9.0|3.11.0": "up",
                "postgis|unpackaged|3.11.0": "up",
            }
        )

        result = self.harness.run("3.11.0", args=("--oldest",))
        output = result.stdout + result.stderr

        self.assertEqual(0, result.returncode)
        self.assertIn("Testing postgis extension upgrade 3.9.0--3.11.0", output)
        self.assertNotIn("extension upgrade 3.10.0--3.11.0", output)

    def test_oldest_falls_back_when_first_source_has_no_update_path(self):
        self.harness.create_extension_files("postgis", ["3.10.0", "3.9.0", "3.11.0"])
        self.harness.create_contrib_files(["3.10.0"])
        self.harness.set_path_map(
            {
                "postgis|3.9.0|3.11.0": "",
                "postgis|3.10.0|3.11.0": "up",
                "postgis|unpackaged|3.11.0": "up",
            }
        )

        result = self.harness.run("3.11.0", args=("--oldest",))
        output = result.stdout + result.stderr

        self.assertEqual(0, result.returncode)
        self.assertIn("SKIP: postgis extension upgrade 3.9.0--3.11.0 (no upgrade path", output)
        self.assertIn("Testing postgis extension upgrade 3.10.0--3.11.0", output)

    def test_oldest_skipped_when_oldest_label_is_excluded_by_regex(self):
        self.harness.create_extension_files("postgis", ["3.10.0", "3.9.0", "3.11.0"])
        self.harness.create_contrib_files(["3.10.0"])
        self.harness.set_path_map(
            {
                "postgis|3.9.0|3.11.0": "up",
                "postgis|3.10.0|3.11.0": "up",
                "postgis|unpackaged|3.11.0": "up",
            }
        )

        result = self.harness.run(
            "3.11.0",
            args=("--oldest", "--skip", r"3\.9\.0--3\.11\.0"),
        )
        output = result.stdout + result.stderr

        self.assertEqual(0, result.returncode)
        self.assertIn("SKIP: postgis extension upgrade 3.9.0--3.11.0", output)
        self.assertIn("Testing postgis extension upgrade 3.10.0--3.11.0", output)

    def test_extension_filter_runs_only_requested_extension(self):
        self.harness.create_extension_files("postgis", ["3.8.0", "3.7.0"])
        self.harness.create_extension_files("postgis_topology", ["3.6.0", "3.7.0"])
        self.harness.create_contrib_files(["3.6.0"])
        self.harness.set_path_map(
            {
                "postgis_topology|3.6.0|3.7.0": "up",
                "postgis|3.8.0|3.7.0": "up",
                "postgis_topology|3.7.0|3.7.0": "up",
                "postgis|3.7.0|3.7.0": "up",
                "postgis_topology|unpackaged|3.7.0": "up",
                "postgis|unpackaged|3.7.0": "up",
            }
        )

        result = self.harness.run("3.7.0", args=("--extension", "postgis_topology"))
        output = result.stdout + result.stderr
        calls = self.harness.make_calls()

        self.assertEqual(0, result.returncode)
        self.assertNotIn("Testing postgis extension upgrade", output)
        self.assertIn("Testing postgis_topology extension upgrade 3.6.0--3.7.0", output)
        for call in calls:
            cmd = " ".join(call["argv"])
            self.assertIn("-C", cmd)
            self.assertIn("topology/test", cmd)

    def test_unpackaged_soft_and_hard_share_oldest_upgrade_source(self):
        current_version = self.harness.current_version
        self.harness.create_extension_files("postgis", ["3.6.0", current_version])
        self.harness.create_contrib_files(["3.5.0", "3.6.0"])
        self.harness.set_path_map(
            {
                f"postgis|3.6.0|{current_version}": "up",
                f"postgis|unpackaged|{current_version}": "up",
            }
        )

        result = self.harness.run(current_version, args=("--oldest",))
        output = result.stdout + result.stderr
        calls = self.harness.make_calls()

        self.assertEqual(0, result.returncode)
        self.assertIn("Testing postgis script soft upgrade", output)
        self.assertIn("Testing postgis script hard upgrade", output)

        self.assertTrue(calls)
        soft = re.search(r"Testing postgis script soft upgrade unpackaged(\S+)--:auto", output)
        hard = re.search(r"Testing postgis script hard upgrade unpackaged(\S+)--:auto", output)
        self.assertIsNotNone(soft)
        self.assertIsNotNone(hard)
        self.assertEqual(soft.group(1), hard.group(1))

    def test_make_child_failure_keeps_nonzero_and_stops_with_dash_s(self):
        self.harness.create_extension_files("postgis", ["3.5.0", "3.6.0", "3.7.0"])
        self.harness.create_contrib_files(["3.5.0"])
        self.harness.set_path_map(
            {
                "postgis|3.5.0|3.7.0": "up",
                "postgis|3.6.0|3.7.0": "up",
                "postgis|unpackaged|3.7.0": "up",
            }
        )
        self.harness.set_fail_patterns({"3.5.0--3.7.0": 1})

        result = self.harness.run("3.7.0", args=("-s",))
        output = result.stdout + result.stderr
        calls = self.harness.make_calls()

        self.assertNotEqual(0, result.returncode)
        self.assertIn("FAIL: postgis extension upgrade 3.5.0--3.7.0", output)
        self.assertNotIn("Testing postgis extension upgrade 3.6.0--3.7.0", output)
        self.assertLessEqual(len(calls), 1)


if __name__ == "__main__":
    unittest.main()
