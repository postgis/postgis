#!/usr/bin/env python3

import os
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


CHECK_NEWS = Path(__file__).with_name("check_news.sh")

BASE_NEWS = """\
PostGIS 4.0.0rc1
2026/xx/xx

* Bug Fixes *

 - Existing unreleased fix

PostGIS 3.9.0
2020/01/01

* Bug Fixes *

 - Existing released fix
          with historical detail
 - Another released fix
"""


class NewsFixture:
    def __init__(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.repo = Path(self.temporary_directory.name)
        self.git("init", "--initial-branch=main")
        self.git("config", "user.name", "Fixture Committer")
        self.git("config", "user.email", "committer@example.com")
        self.write_news(BASE_NEWS)
        self.git("add", "NEWS")
        self.git("commit", "-m", "base NEWS")
        self.base_commit = self.git("rev-parse", "HEAD").stdout.strip()

    def close(self):
        self.temporary_directory.cleanup()

    def git(self, *args):
        return subprocess.run(
            ["git", *args],
            cwd=self.repo,
            check=True,
            text=True,
            capture_output=True,
        )

    def write_news(self, news):
        (self.repo / "NEWS").write_text(
            textwrap.dedent(news),
            encoding="utf-8",
        )

    def check_news(self, base_ref=None, environment_base_ref=None):
        command = [str(CHECK_NEWS)]
        if base_ref is not None:
            command.append(f"--base-ref={base_ref}")
        command.append(str(self.repo))
        environment = os.environ.copy()
        environment.pop("NEWS_CHECK_BASE_REF", None)
        if environment_base_ref is not None:
            environment["NEWS_CHECK_BASE_REF"] = environment_base_ref
        return subprocess.run(
            command,
            env=environment,
            text=True,
            capture_output=True,
        )


class NewsValidationTest(unittest.TestCase):
    def setUp(self):
        self.fixture = NewsFixture()

    def tearDown(self):
        self.fixture.close()

    def test_new_entry_in_current_unreleased_section_passes(self):
        self.fixture.write_news(
            BASE_NEWS.replace(
                " - Existing unreleased fix",
                " - New unreleased fix\n - Existing unreleased fix",
            )
        )
        result = self.fixture.check_news(self.fixture.base_commit)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_new_entry_in_released_section_fails(self):
        self.fixture.write_news(
            BASE_NEWS.replace(
                " - Existing released fix",
                " - New misplaced fix\n - Existing released fix",
            )
        )
        result = self.fixture.check_news(
            environment_base_ref=self.fixture.base_commit
        )
        self.assertNotEqual(0, result.returncode)
        self.assertIn(
            "PostGIS 3.9.0 has a new NEWS entry",
            result.stdout,
        )

    def test_new_two_space_entry_in_released_section_fails(self):
        self.fixture.write_news(
            BASE_NEWS.replace(
                " - Existing released fix",
                "  - New misplaced legacy-format fix\n"
                " - Existing released fix",
            )
        )
        result = self.fixture.check_news(self.fixture.base_commit)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("New misplaced legacy-format fix", result.stdout)

    def test_rewording_released_entry_heading_fails(self):
        self.fixture.write_news(
            BASE_NEWS.replace(
                " - Existing released fix",
                " - Corrected wording for released fix",
            )
        )
        result = self.fixture.check_news(self.fixture.base_commit)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("has a new NEWS entry", result.stdout)

    def test_rewording_released_entry_detail_passes(self):
        self.fixture.write_news(
            BASE_NEWS.replace(
                "          with historical detail",
                "          with corrected historical detail",
            )
        )
        result = self.fixture.check_news(self.fixture.base_commit)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_new_released_entry_cannot_hide_behind_deleted_entry(self):
        news = BASE_NEWS.replace(
            " - Existing released fix",
            " - New misplaced fix\n - Existing released fix",
        ).replace("\n - Another released fix", "")
        self.fixture.write_news(news)
        result = self.fixture.check_news(self.fixture.base_commit)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("New misplaced fix", result.stdout)

    def test_duplicate_released_entry_fails(self):
        self.fixture.write_news(
            BASE_NEWS.replace(
                " - Existing released fix",
                " - Existing released fix\n - Existing released fix",
            )
        )
        result = self.fixture.check_news(self.fixture.base_commit)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("Existing released fix", result.stdout)

    def test_release_promotion_can_finish_previous_unreleased_section(self):
        self.fixture.write_news(
            """\
            PostGIS 4.1.0dev
            2026/xx/xx

            * Bug Fixes *

            PostGIS 4.0.0rc1
            2020/02/01

            * Bug Fixes *

             - Final fix added while preparing the release
             - Existing unreleased fix

            PostGIS 3.9.0
            2020/01/01

            * Bug Fixes *

             - Existing released fix
            """
        )
        result = self.fixture.check_news(self.fixture.base_commit)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_new_unreleased_section_can_follow_released_target(self):
        self.fixture.write_news(
            BASE_NEWS.replace(
                "PostGIS 4.0.0rc1\n2026/xx/xx",
                "PostGIS 4.0.0rc1\n2020/02/01",
            )
        )
        self.fixture.git("add", "NEWS")
        self.fixture.git("commit", "-m", "release current section")
        target_commit = self.fixture.git("rev-parse", "HEAD").stdout.strip()
        self.fixture.write_news(
            """\
            PostGIS 4.1.0dev
            2026/xx/xx

            * Bug Fixes *

             - First fix after the release

            """
            + (self.fixture.repo / "NEWS").read_text(encoding="utf-8")
        )

        result = self.fixture.check_news(target_commit)
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_stale_branch_cannot_add_to_section_closed_on_target(self):
        self.fixture.git("checkout", "-b", "feature")
        self.fixture.git("checkout", "-b", "target", self.fixture.base_commit)
        self.fixture.write_news(
            BASE_NEWS.replace(
                "PostGIS 4.0.0rc1\n2026/xx/xx",
                "PostGIS 4.1.0dev\n2026/xx/xx\n\n"
                "* Bug Fixes *\n\n"
                "PostGIS 4.0.0rc1\n2026/07/25",
            )
        )
        self.fixture.git("add", "NEWS")
        self.fixture.git("commit", "-m", "close release")
        target_commit = self.fixture.git("rev-parse", "HEAD").stdout.strip()
        self.fixture.git("checkout", "feature")
        self.fixture.write_news(
            BASE_NEWS.replace(
                " - Existing unreleased fix",
                " - Late stale-branch fix\n - Existing unreleased fix",
            )
        )

        result = self.fixture.check_news(target_commit)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("Late stale-branch fix", result.stdout)

    def test_shallow_feature_checkout_uses_target_tip_directly(self):
        self.fixture.git("checkout", "-b", "feature")
        self.fixture.write_news(
            BASE_NEWS.replace(
                " - Existing unreleased fix",
                " - New feature fix\n - Existing unreleased fix",
            )
        )
        self.fixture.git("add", "NEWS")
        self.fixture.git("commit", "-m", "feature NEWS")

        with tempfile.TemporaryDirectory() as clone_directory:
            clone = Path(clone_directory)
            subprocess.run(
                [
                    "git",
                    "clone",
                    "--depth=1",
                    "--branch=feature",
                    f"file://{self.fixture.repo}",
                    str(clone),
                ],
                check=True,
                text=True,
                capture_output=True,
            )
            subprocess.run(
                [
                    "git",
                    "fetch",
                    "origin",
                    "refs/heads/main:refs/news-check/target",
                ],
                cwd=clone,
                check=True,
                text=True,
                capture_output=True,
            )
            self.assertEqual(
                "true",
                subprocess.run(
                    ["git", "rev-parse", "--is-shallow-repository"],
                    cwd=clone,
                    check=True,
                    text=True,
                    capture_output=True,
                ).stdout.strip(),
            )
            result = subprocess.run(
                [
                    str(CHECK_NEWS),
                    "--base-ref=refs/news-check/target",
                    str(clone),
                ],
                text=True,
                capture_output=True,
            )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)

    def test_missing_explicit_base_ref_fails(self):
        result = self.fixture.check_news("refs/heads/missing")
        self.assertNotEqual(0, result.returncode)
        self.assertIn("does not resolve to a commit", result.stdout)


if __name__ == "__main__":
    unittest.main()
