#!/usr/bin/env python3

import os
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path

from check_contributor_credits import (
    CreditValidationError,
    normalized_name,
    split_news_people,
    validate,
)


CREDITS_TEMPLATE = """\
<chapter xmlns="http://docbook.org/ns/docbook"
         xmlns:xlink="http://www.w3.org/1999/xlink"
         xml:id="postgis_credits">
  <section xml:id="psc"><variablelist>{terms}</variablelist></section>
  <section xml:id="credits_core_present"><variablelist/></section>
  <section xml:id="credits_core_past"><variablelist/></section>
  <section xml:id="credits_other_contributors">
    <simplelist>{members}</simplelist>
  </section>
</chapter>
"""


class ContributorCreditFixture:
    def __init__(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.repo = Path(self.temporary_directory.name)
        (self.repo / "doc").mkdir()
        self.git("init", "--initial-branch=main")
        self.git("config", "gc.auto", "0")
        self.git("config", "maintenance.auto", "false")
        self.git("config", "user.name", "Fixture Committer")
        self.git("config", "user.email", "committer@example.com")

    def close(self):
        self.temporary_directory.cleanup()

    def git(self, *args, env=None):
        subprocess.run(
            ["git", *args],
            cwd=self.repo,
            env=env,
            check=True,
            text=True,
            capture_output=True,
        )

    def write_credits(self, *names):
        members = "".join(f"<member>{name}</member>" for name in names)
        (self.repo / "doc" / "credits.xml").write_text(
            CREDITS_TEMPLATE.format(terms="", members=members),
            encoding="utf-8",
        )

    def write_news(self, current_credit="Bob News"):
        (self.repo / "NEWS").write_text(
            textwrap.dedent(
                f"""\
                PostGIS 9.9.0dev
                2099/xx/xx

                * Enhancements *

                 - Add a fixture ({current_credit})

                PostGIS 9.8.0
                2025/01/01

                 - Historical syntax note (--without-fixture)
                """
            ),
            encoding="utf-8",
        )

    def initial_commit(
        self,
        *credits,
        mailmap=True,
        message="fixture",
        mailmap_entries="",
        news_credit="Bob News",
    ):
        self.write_credits(*credits)
        self.write_news(news_credit)
        (self.repo / ".mailmap").write_text(
            (
                "Alice Example <alice@example.com> "
                "Alice Alias <alias@example.com>\n"
                + mailmap_entries
                if mailmap
                else ""
            ),
            encoding="utf-8",
        )
        self.git("add", ".")
        env = os.environ.copy()
        env.update(
            {
                "GIT_AUTHOR_NAME": "Alice Alias",
                "GIT_AUTHOR_EMAIL": "alias@example.com",
                "GIT_COMMITTER_NAME": "Fixture Committer",
                "GIT_COMMITTER_EMAIL": "committer@example.com",
            }
        )
        self.git("commit", "-m", message, env=env)


class ContributorCreditValidationTest(unittest.TestCase):
    def setUp(self):
        self.fixture = ContributorCreditFixture()

    def tearDown(self):
        self.fixture.close()

    def test_mailmapped_author_and_unreleased_news_credit_pass(self):
        self.fixture.initial_commit("Alice Example", "Bob News")
        result = validate(self.fixture.repo)
        self.assertEqual([], result.missing)
        self.assertEqual(1, result.git_authors)
        self.assertEqual(1, result.news_contributors)

    def test_missing_git_author_fails(self):
        self.fixture.initial_commit("Bob News")
        result = validate(self.fixture.repo)
        self.assertEqual(["Alice Example"], [item.name for item in result.missing])

    def test_subdirectory_is_rejected_as_worktree_root(self):
        self.fixture.initial_commit("Alice Example", "Bob News")
        with self.assertRaisesRegex(CreditValidationError, "not the Git worktree root"):
            validate(self.fixture.repo / "doc")

    def test_missing_mailmap_alias_fails(self):
        self.fixture.initial_commit("Alice Example", "Bob News", mailmap=False)
        result = validate(self.fixture.repo)
        self.assertEqual(["Alice Alias"], [item.name for item in result.missing])

    def test_missing_unreleased_news_contributor_fails(self):
        self.fixture.initial_commit("Alice Example")
        result = validate(self.fixture.repo)
        self.assertEqual(["Bob News"], [item.name for item in result.missing])

    def test_missing_handle_only_news_contributor_fails(self):
        self.fixture.initial_commit("Alice Example", news_credit="public_handle")
        result = validate(self.fixture.repo)
        self.assertEqual(["public_handle"], [item.name for item in result.missing])

    def test_partial_name_github_credit_is_an_explicit_alias(self):
        self.fixture.initial_commit(
            (
                '<link xlink:href="https://github.com/alice-handle">'
                "@alice-handle</link> (GitHub user: Alice Example)"
            ),
            news_credit="alice-handle",
        )
        result = validate(self.fixture.repo)
        self.assertEqual([], result.missing)

    def test_linked_github_user_is_an_explicit_handle_credit(self):
        self.fixture.initial_commit(
            "Alice Example",
            (
                '<link xlink:href="https://github.com/public-handle">'
                "@public-handle</link> (GitHub user)"
            ),
            news_credit="public-handle",
        )
        result = validate(self.fixture.repo)
        self.assertEqual([], result.missing)

    def test_github_mention_must_link_to_its_profile(self):
        self.fixture.initial_commit(
            "Alice Example",
            (
                '<link xlink:href="https://github.com/someone-else">'
                "@public-handle</link> (GitHub user)"
            ),
            news_credit="public-handle",
        )
        with self.assertRaisesRegex(
            CreditValidationError,
            "GitHub mention @public-handle must link to",
        ):
            validate(self.fixture.repo)

    def test_bare_linked_github_handle_is_rejected(self):
        self.fixture.initial_commit(
            "Alice Example",
            (
                '<link xlink:href="https://github.com/public-handle">'
                "@public-handle</link>"
            ),
            news_credit="public-handle",
        )
        with self.assertRaisesRegex(
            CreditValidationError,
            "must use '@handle \\(GitHub user\\)'",
        ):
            validate(self.fixture.repo)

    def test_name_first_github_credit_is_rejected(self):
        self.fixture.initial_commit(
            (
                'Alice Example (<link xlink:href="https://github.com/alice-handle">'
                "@alice-handle</link>)"
            ),
            news_credit="alice-handle",
        )
        with self.assertRaisesRegex(
            CreditValidationError,
            "must use '@handle \\(GitHub user\\)'",
        ):
            validate(self.fixture.repo)

    def test_longer_name_does_not_credit_a_different_identity(self):
        self.fixture.initial_commit("Alice Example Jr", "Bob News")
        result = validate(self.fixture.repo)
        self.assertEqual(["Alice Example"], [item.name for item in result.missing])

    def test_missing_coauthor_is_reported(self):
        self.fixture.initial_commit(
            "Alice Example",
            "Bob News",
            message="fixture\n\nCo-authored-by: Carol Helper <carol@example.com>",
        )
        result = validate(self.fixture.repo)
        self.assertEqual(["Carol Helper"], [item.name for item in result.missing])

    def test_mailmapped_coauthor_alias_passes(self):
        self.fixture.initial_commit(
            "Alice Example",
            "Bob News",
            "Carol Helper",
            message="fixture\n\nCo-authored-by: Carol Alias <carol-alias@example.com>",
            mailmap_entries=(
                "Carol Helper <carol@example.com> "
                "Carol Alias <carol-alias@example.com>\n"
            ),
        )
        result = validate(self.fixture.repo)
        self.assertEqual([], result.missing)
        self.assertEqual(1, result.git_coauthors)

    def test_coauthor_example_outside_trailer_block_is_ignored(self):
        self.fixture.initial_commit(
            "Alice Example",
            "Bob News",
            message=(
                "fixture\n\n"
                "Co-authored-by: Body Example <body@example.com>\n\n"
                "This line is not a trailer."
            ),
        )
        result = validate(self.fixture.repo)
        self.assertEqual([], result.missing)
        self.assertEqual(0, result.git_coauthors)

    def test_non_human_coauthor_name_is_ignored(self):
        self.fixture.initial_commit(
            "Alice Example",
            "Bob News",
            message=(
                "fixture\n\n"
                "Co-authored-by: Claude Fable 5 <noreply@anthropic.com>"
            ),
        )
        result = validate(self.fixture.repo)
        self.assertEqual([], result.missing)
        self.assertEqual(0, result.git_coauthors)

    def test_news_slashes_separate_people_from_people_and_affiliations(self):
        self.assertEqual(
            ["Regina Obe", "Sandro Santilli"],
            split_news_people("Regina Obe / Sandro Santilli"),
        )
        self.assertEqual(
            ["Dan Baston"],
            split_news_people("Dan Baston / City of Helsinki"),
        )

    def test_released_news_parentheses_are_not_attributions(self):
        self.fixture.initial_commit("Alice Example", "Bob News")
        result = validate(self.fixture.repo)
        self.assertNotIn(("without", "fixture"), result.contributors)
        self.assertEqual([], result.missing)

    def test_technical_news_parentheses_are_not_attributions(self):
        self.fixture.initial_commit("Alice Example", "Bob News")
        self.fixture.write_news("ST_Fixture_Helper")
        self.fixture.git("add", "NEWS")
        env = os.environ.copy()
        env.update(
            {
                "GIT_AUTHOR_NAME": "Alice Alias",
                "GIT_AUTHOR_EMAIL": "alias@example.com",
                "GIT_COMMITTER_NAME": "Fixture Committer",
                "GIT_COMMITTER_EMAIL": "committer@example.com",
            }
        )
        self.fixture.git("commit", "-m", "technical NEWS fixture", env=env)
        result = validate(self.fixture.repo)
        self.assertNotIn(("st", "fixture", "helper"), result.contributors)
        self.assertEqual([], result.missing)

    def test_unicode_names_are_normalized_without_losing_the_script(self):
        self.assertEqual(("евгении", "пример"), normalized_name("Евгений Пример"))
        self.assertEqual(("public", "handle"), normalized_name("public_handle"))

    def test_shallow_history_is_rejected(self):
        self.fixture.initial_commit("Alice Example", "Bob News")
        shallow_file = self.fixture.repo / ".git" / "shallow"
        shallow_file.write_text(
            subprocess.check_output(
                ["git", "rev-parse", "HEAD"],
                cwd=self.fixture.repo,
                text=True,
            ),
            encoding="utf-8",
        )
        with self.assertRaisesRegex(CreditValidationError, "non-shallow"):
            validate(self.fixture.repo)


if __name__ == "__main__":
    unittest.main()
