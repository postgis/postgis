#!/usr/bin/env python3

import argparse
import dataclasses
import re
import subprocess
import sys
import unicodedata
import xml.etree.ElementTree as ET
from pathlib import Path


DOCBOOK = {"db": "http://docbook.org/ns/docbook"}
XML_ID = "{http://www.w3.org/XML/1998/namespace}id"
XLINK_HREF = "{http://www.w3.org/1999/xlink}href"
AUTHOR_RECORD_SEPARATOR = "\x1e"
AUTHOR_FIELD_SEPARATOR = "\x1f"
TRAILER_SEPARATOR = "\x1d"
RELEASE_DATE_RE = re.compile(r"^\d{4}/(?:\d{2}|xx)/(?:\d{2}|xx)$", re.I)
BULLET_RE = re.compile(r"^\s*-\s+")
NEWS_ATTRIBUTION_RE = re.compile(r"\(([^()]+)\)\s*$")
NEWS_TEAM_RE = re.compile(r"^\s*([^()]+?)\s+\([^()]*\bTeam\)\s*$", re.I)
IDENTITY_RE = re.compile(r"^(.*?)\s*<([^<>]+)>$")
CREDIT_SUFFIX_RE = re.compile(r"^(.*?)\s*\(([^()]*)\)\s*$")
GITHUB_HANDLE_PATTERN = r"[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?"
GITHUB_USER_CREDIT_RE = re.compile(
    rf"^@({GITHUB_HANDLE_PATTERN})\s+\(GitHub user\)$"
)
NAMED_GITHUB_USER_CREDIT_RE = re.compile(
    rf"^@({GITHUB_HANDLE_PATTERN})\s+\(GitHub user:\s+([^()@]+)\)$"
)
GITHUB_MENTION_RE = re.compile(rf"@({GITHUB_HANDLE_PATTERN})\b")
NON_ALIAS_CREDIT_SUFFIXES = {("chair",)}
NON_HUMAN_EMAILS = {
    "no@body.net",
    "noreply-mt-deepl@weblate.org",
    "noreply-mt-weblate@weblate.org",
    "noreply@weblate.org",
}
NON_PERSON_NEWS_RE = re.compile(
    r"\b(?:bureau|city of|company|corporation|foundation|highgo|inc\.?|"
    r"koordinates|llc|osgeo|team|university)\b",
    re.I,
)
TECHNICAL_NEWS_RE = re.compile(
    r"(?:--|::|[<>=])|^(?:ST|CG|GDT|RT|LWGEOM)_[A-Za-z0-9_]+$|"
    r"^[A-Z][A-Z0-9.+-]*\s+\d[\w.+-]*$",
    re.I,
)
NEWS_PERSON_SEPARATOR_RE = re.compile(r"\s*(?:,|/|;|&)\s*|\s+and\s+", re.I)


class CreditValidationError(RuntimeError):
    pass


@dataclasses.dataclass
class Contributor:
    name: str
    sources: dict[str, str]


@dataclasses.dataclass
class ValidationResult:
    contributors: dict[tuple[str, ...], Contributor]
    credits: dict[tuple[str, ...], str]
    missing: list[Contributor]
    git_authors: int
    git_coauthors: int
    news_contributors: int


def git(repo, *args, input_text=None):
    try:
        return subprocess.run(
            ["git", *args],
            cwd=repo,
            input=input_text,
            text=True,
            check=True,
            capture_output=True,
        ).stdout
    except (OSError, subprocess.CalledProcessError) as exc:
        detail = getattr(exc, "stderr", "") or str(exc)
        raise CreditValidationError(f"git {' '.join(args)} failed: {detail.strip()}") from exc


def normalized_name(name):
    name = unicodedata.normalize("NFKD", name.strip()).casefold()
    name = "".join(char for char in name if not unicodedata.combining(char))
    return tuple("".join(char if char.isalnum() else " " for char in name).split())


def credit_keys(name):
    match = NAMED_GITHUB_USER_CREDIT_RE.match(name)
    if match:
        return {normalized_name(match.group(1)), normalized_name(match.group(2))}

    match = GITHUB_USER_CREDIT_RE.match(name)
    if match:
        return {normalized_name(match.group(1))}

    match = CREDIT_SUFFIX_RE.match(name)
    if not match:
        key = normalized_name(name)
        return {key} if key else set()

    keys = {normalized_name(match.group(1))}
    suffix = normalized_name(match.group(2))
    if suffix and suffix not in NON_ALIAS_CREDIT_SUFFIXES:
        keys.add(suffix)
    return {key for key in keys if key}


def person_is_credited(name, credits):
    return normalized_name(name) in credits


def credit_member_text(node):
    name = "".join(node.itertext()).strip()
    links = node.findall(".//db:link", DOCBOOK)
    handles = GITHUB_MENTION_RE.findall(name)
    if handles and not (
        GITHUB_USER_CREDIT_RE.fullmatch(name)
        or NAMED_GITHUB_USER_CREDIT_RE.fullmatch(name)
    ):
        raise CreditValidationError(
            f"GitHub credit {name!r} must use '@handle (GitHub user)' or "
            "'@handle (GitHub user: partial name)'"
        )
    for handle in handles:
        expected_text = f"@{handle}"
        expected_href = f"https://github.com/{handle}"
        if not any(
            "".join(link.itertext()).strip() == expected_text
            and link.get(XLINK_HREF) == expected_href
            for link in links
        ):
            raise CreditValidationError(
                f"GitHub mention {expected_text} must link to {expected_href}"
            )
    return name


def contributor_credit_names(credits_path):
    try:
        root = ET.parse(credits_path).getroot()
    except (OSError, ET.ParseError) as exc:
        raise CreditValidationError(f"cannot parse {credits_path}: {exc}") from exc

    names = []
    for section in root.findall(".//db:section", DOCBOOK):
        section_id = section.get(XML_ID, "")
        if section_id == "psc" or section_id in {
            "credits_core_present",
            "credits_core_past",
        }:
            names.extend(
                credit_member_text(node)
                for node in section.findall(".//db:term", DOCBOOK)
            )
        elif section_id == "credits_other_contributors":
            names.extend(
                credit_member_text(node)
                for node in section.findall(".//db:simplelist/db:member", DOCBOOK)
            )

    credits = {}
    for name in names:
        for key in credit_keys(name):
            credits.setdefault(key, name)
    return credits


def add_contributor(contributors, name, source_kind, source_detail):
    key = normalized_name(name)
    if not key:
        return
    contributor = contributors.setdefault(key, Contributor(name=name, sources={}))
    contributor.sources.setdefault(source_kind, source_detail)


def non_human_identity(email):
    return email.strip().casefold() in NON_HUMAN_EMAILS


def git_author_contributors(repo, revision):
    output = git(
        repo,
        "log",
        revision,
        "--no-merges",
        "--use-mailmap",
        f"--format=%aN%x1f%aE%x1f%H%x1e",
    )
    authors = []
    for record in output.split(AUTHOR_RECORD_SEPARATOR):
        fields = record.strip("\n").split(AUTHOR_FIELD_SEPARATOR)
        if len(fields) != 3:
            continue
        name, email, commit = fields
        if not non_human_identity(email):
            authors.append((name, email, commit))
    return authors


def coauthor_identities(repo, revision):
    output = git(
        repo,
        "log",
        revision,
        "--format=%H%x1f%(trailers:key=Co-authored-by,valueonly,"
        "separator=%x1d)%x1e",
    )
    identities = []
    for record in output.split(AUTHOR_RECORD_SEPARATOR):
        fields = record.strip("\n").split(AUTHOR_FIELD_SEPARATOR, 1)
        if len(fields) != 2:
            continue
        commit, trailers = fields
        for trailer in trailers.split(TRAILER_SEPARATOR):
            match = IDENTITY_RE.match(trailer.strip())
            if match:
                identities.append((match.group(1), match.group(2), commit))

    if not identities:
        return []

    mailmap_input = "".join(f"{name} <{email}>\n" for name, email, _ in identities)
    mapped = git(repo, "check-mailmap", "--stdin", input_text=mailmap_input).splitlines()
    normalized = []
    for mapped_identity, (_, _, commit) in zip(mapped, identities):
        match = IDENTITY_RE.match(mapped_identity)
        if match and not non_human_identity(match.group(2)):
            normalized.append((match.group(1).strip(), match.group(2), commit))
    return normalized


def unreleased_news_lines(news_path):
    try:
        lines = news_path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise CreditValidationError(f"cannot read {news_path}: {exc}") from exc

    for index, line in enumerate(lines):
        stripped = line.strip()
        if RELEASE_DATE_RE.fullmatch(stripped) and "xx" not in stripped.casefold():
            return lines[: max(0, index - 1)]
    return lines


def split_news_people(value):
    people = []
    for candidate in NEWS_PERSON_SEPARATOR_RE.split(value):
        candidate = re.sub(r"\s+from\s+.*$", "", candidate, flags=re.I).strip()
        if (
            not candidate
            or NON_PERSON_NEWS_RE.search(candidate)
            or TECHNICAL_NEWS_RE.search(candidate)
        ):
            continue
        if normalized_name(candidate) and re.search(
            r"[^\W\d_]", candidate, re.UNICODE
        ):
            people.append(candidate)
    return people


def news_contributors(news_path):
    lines = unreleased_news_lines(news_path)
    contributors = []
    current = []
    current_line = None

    def finish_entry():
        nonlocal current, current_line
        if current:
            text = " ".join(line.strip() for line in current)
            match = NEWS_ATTRIBUTION_RE.search(text)
            if match:
                contributors.extend(
                    (name, current_line) for name in split_news_people(match.group(1))
                )
        current = []
        current_line = None

    for line_number, line in enumerate(lines, 1):
        if BULLET_RE.match(line):
            finish_entry()
            current = [line]
            current_line = line_number
            continue
        if current and (not line.strip() or line.lstrip().startswith("*")):
            finish_entry()
        elif current:
            current.append(line)
        else:
            match = NEWS_TEAM_RE.match(line)
            if match:
                contributors.extend(
                    (name, line_number) for name in split_news_people(match.group(1))
                )
    finish_entry()
    return contributors


def validate(repo, revision="HEAD", require_full_history=True):
    repo = Path(repo).resolve()
    top_level = Path(git(repo, "rev-parse", "--show-toplevel").strip()).resolve()
    if top_level != repo:
        raise CreditValidationError(f"{repo} is not the Git worktree root ({top_level})")
    if require_full_history:
        shallow = git(repo, "rev-parse", "--is-shallow-repository").strip()
        if shallow == "true":
            raise CreditValidationError(
                "contributor validation requires a full, non-shallow Git history"
            )

    credits_path = repo / "doc" / "credits.xml"
    news_path = repo / "NEWS"
    credits = contributor_credit_names(credits_path)
    contributors = {}

    authors = git_author_contributors(repo, revision)
    for name, _email, commit in authors:
        add_contributor(contributors, name, "Git author", commit[:12])

    coauthors = coauthor_identities(repo, revision)
    for name, _email, commit in coauthors:
        add_contributor(contributors, name, "Git co-author", commit[:12])

    news_people = news_contributors(news_path)
    for name, line_number in news_people:
        add_contributor(contributors, name, "unreleased NEWS", f"line {line_number}")

    missing = sorted(
        (
            contributor
            for contributor in contributors.values()
            if not person_is_credited(contributor.name, credits)
        ),
        key=lambda contributor: normalized_name(contributor.name),
    )
    return ValidationResult(
        contributors=contributors,
        credits=credits,
        missing=missing,
        git_authors=len({normalized_name(name) for name, _email, _commit in authors}),
        git_coauthors=len(
            {normalized_name(name) for name, _email, _commit in coauthors}
        ),
        news_contributors=len({normalized_name(name) for name, _line in news_people}),
    )


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Check that PostGIS Git and NEWS contributors are credited in the manual."
    )
    parser.add_argument(
        "--repo",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="PostGIS worktree root (default: repository containing this script)",
    )
    parser.add_argument(
        "--revision",
        default="HEAD",
        help="Git revision whose reachable history is checked (default: HEAD)",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    try:
        result = validate(args.repo, args.revision)
    except CreditValidationError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2

    if result.missing:
        print(
            f"FAIL: doc/credits.xml is missing {len(result.missing)} contributor(s):",
            file=sys.stderr,
        )
        for contributor in result.missing:
            evidence = ", ".join(
                f"{kind} {detail}" for kind, detail in contributor.sources.items()
            )
            print(f"  - {contributor.name} ({evidence})", file=sys.stderr)
        print(
            "Add each canonical display name to the manual credits, or normalize an alias in .mailmap.",
            file=sys.stderr,
        )
        return 1

    print(
        "PASS: all contributors are credited "
        f"({result.git_authors} Git authors, "
        f"{result.git_coauthors} Git co-authors, "
        f"{result.news_contributors} unreleased NEWS contributors; "
        f"{len(set(result.credits.values()))} manual credit entries)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
