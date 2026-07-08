#!/usr/bin/env python3
"""Report suspicious PostGIS DocBook example markup patterns.

This checker is intentionally conservative and warning-oriented.  It does not
try to prove every manual example is runnable; it looks for structural smells
that make the rendered manual harder to read, especially old examples where SQL
input and psql output are still mixed in one programlisting block.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

try:
    from lxml import etree
except ImportError as exc:  # pragma: no cover - dependency issue is environmental
    print(f"check_docbook_markup.py: lxml is required: {exc}", file=sys.stderr)
    raise SystemExit(2) from None

NS = {"d": "http://docbook.org/ns/docbook", "xml": "http://www.w3.org/XML/1998/namespace"}
ADMONITIONS = {"note", "tip", "important", "warning", "caution"}

SQL_START_RE = re.compile(
    r"(?im)^\s*(?:--\s*)?(?:SELECT|WITH|INSERT|UPDATE|DELETE|CREATE|ALTER|DROP|EXPLAIN|VALUES)\b"
)
SQL_STATEMENT_RE = re.compile(
    r"(?ims)^\s*(?:--[^\n]*\n\s*)*(?:SELECT|WITH|INSERT|UPDATE|DELETE|CREATE|ALTER|DROP|EXPLAIN|VALUES)\b.*;"
)
PSQL_ROW_COUNT_RE = re.compile(r"(?m)^\s*\(\d+\s+rows?\)\s*$")
PSQL_TABLE_SEPARATOR_RE = re.compile(r"(?m)^\s*[-+]{3,}(?:\s*\+\s*[-+]{2,})+\s*$")
PSQL_MESSAGE_RE = re.compile(r"(?m)^\s*(?:ERROR|NOTICE|WARNING|DETAIL|HINT|CONTEXT):")


@dataclass(frozen=True)
class Finding:
    severity: str
    code: str
    location: str
    message: str


def local_name(node: etree._Element) -> str:
    return etree.QName(node).localname


def normalized_text(node: etree._Element) -> str:
    return " ".join("".join(node.itertext()).split())


def block_text(node: etree._Element) -> str:
    return "".join(node.itertext())


def element_id(node: etree._Element) -> str | None:
    return node.get(f"{{{NS['xml']}}}id") or node.get("id")


def nearest_refentry_id(node: etree._Element) -> str:
    for ancestor in node.iterancestors():
        if local_name(ancestor) == "refentry":
            return element_id(ancestor) or "<unknown-refentry>"
    return "<no-refentry>"


def location(path: Path, node: etree._Element) -> str:
    line = node.sourceline or "?"
    return f"{path}:{line} ({nearest_refentry_id(node)})"


def first_matching(pattern: re.Pattern[str], text: str) -> str | None:
    match = pattern.search(text)
    if not match:
        return None
    return match.group(0).strip()


def find_mixed_programlisting_warnings(path: Path, root: etree._Element) -> list[Finding]:
    warnings: list[Finding] = []
    for node in root.xpath("//d:programlisting", namespaces=NS):
        text = block_text(node)
        if not SQL_STATEMENT_RE.search(text):
            continue

        markers: list[str] = []
        for label, pattern in (
            ("psql row count", PSQL_ROW_COUNT_RE),
            ("psql table separator", PSQL_TABLE_SEPARATOR_RE),
            ("server message", PSQL_MESSAGE_RE),
        ):
            snippet = first_matching(pattern, text)
            if snippet:
                markers.append(f"{label}: {snippet!r}")

        if markers:
            warnings.append(
                Finding(
                    "warning",
                    "mixed-programlisting-output",
                    location(path, node),
                    "programlisting appears to contain SQL input plus output markers; "
                    "split SQL into <programlisting> and result text into <screen> "
                    f"({'; '.join(markers)})",
                )
            )
    return warnings


def find_screen_sql_warnings(path: Path, root: etree._Element) -> list[Finding]:
    warnings: list[Finding] = []
    for node in root.xpath("//d:screen", namespaces=NS):
        text = block_text(node)
        if SQL_START_RE.search(text) and ";" in text:
            warnings.append(
                Finding(
                    "warning",
                    "screen-contains-sql",
                    location(path, node),
                    "screen block looks like it contains SQL input; use programlisting for input and screen for output",
                )
            )
    return warnings


def find_duplicate_section_warnings(path: Path, root: etree._Element) -> list[Finding]:
    warnings: list[Finding] = []
    section_xpath = "./d:refsection|./d:section|./d:sect1"
    for refentry in root.xpath("//d:refentry", namespaces=NS):
        seen: dict[str, etree._Element] = {}
        for section in refentry.xpath(section_xpath, namespaces=NS):
            title_nodes = section.xpath("./d:title[1]", namespaces=NS)
            if not title_nodes:
                continue
            title = normalized_text(title_nodes[0])
            if title not in {"Description", "Examples"}:
                continue
            if title in seen:
                warnings.append(
                    Finding(
                        "error",
                        "duplicate-refsection-title",
                        location(path, title_nodes[0]),
                        f"refentry has more than one {title!r} section; first one is at line {seen[title].sourceline}",
                    )
                )
            else:
                seen[title] = title_nodes[0]
    return warnings


def find_metadata_note_warnings(path: Path, root: etree._Element) -> list[Finding]:
    warnings: list[Finding] = []
    xpath = "//d:para[@role='availability' or @role='enhanced' or @role='changed']"
    for node in root.xpath(xpath, namespaces=NS):
        role = node.get("role") or "<missing>"
        if any(local_name(ancestor) in ADMONITIONS for ancestor in node.iterancestors()):
            warnings.append(
                Finding(
                    "warning",
                    "metadata-role-in-admonition",
                    location(path, node),
                    f"metadata para role={role!r} is nested inside an admonition; prefer a plain role para in Description",
                )
            )
    return warnings


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml", type=Path, help="generated postgis-out.xml or another DocBook XML file")
    parser.add_argument(
        "--max-warnings",
        type=int,
        default=50,
        help="maximum individual warnings to print before the summary (default: 50)",
    )
    parser.add_argument(
        "--fail-on",
        choices=("none", "error", "warning"),
        default="none",
        help="finding severity that returns non-zero: none, error, or warning (default: none)",
    )
    parser.add_argument(
        "--fail-on-warning",
        action="store_true",
        help="deprecated alias for --fail-on warning",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    parser = etree.XMLParser(resolve_entities=False, load_dtd=False, no_network=True)
    root = etree.parse(str(args.xml), parser).getroot()

    findings: list[Finding] = []
    findings.extend(find_mixed_programlisting_warnings(args.xml, root))
    findings.extend(find_screen_sql_warnings(args.xml, root))
    findings.extend(find_duplicate_section_warnings(args.xml, root))
    findings.extend(find_metadata_note_warnings(args.xml, root))

    for item in findings[: max(args.max_warnings, 0)]:
        print(f"{item.severity.upper()} {item.code}: {item.location}: {item.message}")
    if args.max_warnings >= 0 and len(findings) > args.max_warnings:
        print(f"... {len(findings) - args.max_warnings} additional findings suppressed by --max-warnings")

    severity_counts = Counter(item.severity for item in findings)
    code_counts = Counter(item.code for item in findings)
    if code_counts:
        severity_summary = ", ".join(f"{severity}={count}" for severity, count in sorted(severity_counts.items()))
        code_summary = ", ".join(f"{code}={count}" for code, count in sorted(code_counts.items()))
        print(f"DocBook markup checker summary: {len(findings)} finding(s): {severity_summary}; {code_summary}")
    else:
        print("DocBook markup checker summary: no suspicious DocBook example markup found.")

    fail_on = "warning" if args.fail_on_warning else args.fail_on
    should_fail = (
        (fail_on == "warning" and bool(findings))
        or (fail_on == "error" and any(item.severity == "error" for item in findings))
    )
    if not should_fail and findings:
        print(f"DocBook markup checker did not fail because --fail-on={fail_on}.")

    return 1 if should_fail else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
