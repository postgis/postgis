#!/usr/bin/env python3
"""PostGIS DocBook documentation quality checks and reference index builder."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable
import xml.etree.ElementTree as ET

from xml_tree import parse as parse_xml

DOCBOOK_NS = "http://docbook.org/ns/docbook"
XML_NS = "http://www.w3.org/XML/1998/namespace"
DB = f"{{{DOCBOOK_NS}}}"
XML_ID = f"{{{XML_NS}}}id"

ADMONITIONS = {"note", "tip", "important", "warning", "caution"}
DEPRECATED_ALIAS_ROLE = "deprecated-alias"
COMMAND_REFERENCE_ROLE = "reference-target"
CHUNK_OWNER_ELEMENTS = {"appendix", "chapter", "preface", "refentry"}
DESCRIPTION_METADATA_ROLES = {"availability", "enhanced", "changed"}
SQL_KEYWORD_DATA = (
    # keyword, starts a semicolon-terminated SQL statement, starts an example code block
    ("ADD", False, False),
    ("ALTER", True, True),
    ("ANALYZE", True, True),
    ("AND", False, False),
    ("AS", False, False),
    ("ASC", False, False),
    ("BEGIN", False, True),
    ("BY", False, False),
    ("CASE", False, False),
    ("CAST", False, False),
    ("COMMIT", False, True),
    ("CREATE", True, True),
    ("CROSS", False, False),
    ("DELETE", True, True),
    ("DESC", False, False),
    ("DECLARE", False, False),
    ("DISTINCT", False, False),
    ("DO", False, True),
    ("DROP", True, True),
    ("ELSE", False, False),
    ("END", False, False),
    ("EXCEPT", False, False),
    ("EXECUTE", False, False),
    ("EXPLAIN", True, True),
    ("FALSE", False, False),
    ("FOR", False, False),
    ("FROM", False, False),
    ("FULL", False, False),
    ("GROUP", False, False),
    ("HAVING", False, False),
    ("IF", False, False),
    ("IN", False, False),
    ("INNER", False, False),
    ("INSERT", True, True),
    ("INTERSECT", False, False),
    ("INTO", False, False),
    ("IS", False, False),
    ("JOIN", False, False),
    ("LEFT", False, False),
    ("LIMIT", False, False),
    ("LOOP", False, False),
    ("NOT", False, False),
    ("NULL", False, False),
    ("OFFSET", False, False),
    ("ON", False, False),
    ("OR", False, False),
    ("ORDER", False, False),
    ("OUTER", False, False),
    ("OVER", False, False),
    ("RETURNING", False, False),
    ("RIGHT", False, False),
    ("SELECT", True, True),
    ("SET", True, True),
    ("THEN", False, False),
    ("TRUE", False, False),
    ("UNION", False, False),
    ("UPDATE", True, True),
    ("USING", False, False),
    ("VACUUM", True, True),
    ("VALUES", True, True),
    ("WHEN", False, False),
    ("WHERE", False, False),
    ("WITH", True, True),
)
SQL_HIGHLIGHT_KEYWORDS = tuple(keyword for keyword, _statement, _block in SQL_KEYWORD_DATA)
SQL_STATEMENT_START_KEYWORDS = tuple(keyword for keyword, statement, _block in SQL_KEYWORD_DATA if statement)
SQL_BLOCK_STARTER_KEYWORDS = tuple(keyword for keyword, _statement, block in SQL_KEYWORD_DATA if block)
SQL_BLOCK_STARTER_EXTRAS = tuple(
    keyword for keyword in SQL_BLOCK_STARTER_KEYWORDS if keyword not in SQL_STATEMENT_START_KEYWORDS
)


def keyword_pattern(keywords: Iterable[str]) -> str:
    return "|".join(re.escape(keyword) for keyword in keywords)


SQL_START_RE = re.compile(rf"(?im)^\s*(?:--\s*)?(?:{keyword_pattern(SQL_STATEMENT_START_KEYWORDS)})\b")
SQL_STATEMENT_RE = re.compile(
    rf"(?ims)^\s*(?:--[^\n]*\n\s*)*(?:{keyword_pattern(SQL_STATEMENT_START_KEYWORDS)})\b.*;"
)
PSQL_META_COMMAND_PATTERN = r"\\(?:[A-Za-z][A-Za-z_+.-]*|[?!])"
LEADING_SQL_COMMENT_RE = re.compile(
    rf"(?is)^\s*(?:--(?!-)[^\n]*(?:\n|$)\s*)+"
    rf"(?=(?:(?:{keyword_pattern(SQL_STATEMENT_START_KEYWORDS)})\b|"
    rf"{PSQL_META_COMMAND_PATTERN}(?:[ \t]|$)))"
)
PSQL_META_COMMAND_RE = re.compile(rf"(?m)^[ \t]*{PSQL_META_COMMAND_PATTERN}(?:[ \t]|$)")
PSQL_ROW_COUNT_RE = re.compile(r"(?m)^\s*\(\d+\s+rows?\)\s*$")
PSQL_TABLE_SEPARATOR_RE = re.compile(r"(?m)^\s*[-+]{3,}(?:\s*\+\s*[-+]{2,})+\s*$")
PSQL_UNICODE_TABLE_SEPARATOR_RE = re.compile(r"(?m)^\s*[─┼]{3,}(?:\s*┼\s*[─┼]{2,})+\s*$")
OUTPUT_DASH_RUN_RE = re.compile(r"(?m)^\s*-{3,}")
PSQL_MESSAGE_RE = re.compile(r"(?m)^\s*(?:ERROR|NOTICE|WARNING|DETAIL|HINT|CONTEXT):")
REDUNDANT_OUTPUT_CAPTION_RE = re.compile(r"(?:output|result|results|returns|the output):?", re.IGNORECASE)
ANCIENT_VERSION_RE = re.compile(r"\b(?:0\.\d+|1\.[0-5])(?:\.\d+)?\b")
SELECT_START_RE = re.compile(r"^\s*SELECT\b", re.IGNORECASE | re.MULTILINE)
EXAMPLE_SECTION_TITLE_RE = re.compile(r"^(?:same\s+)?examples?\b", re.IGNORECASE)
RASTER_IMAGE_EXTENSIONS = {".gif", ".jpeg", ".jpg", ".png", ".webp"}
ALLOWED_SOURCE_RASTER_IMAGES = {
    "PostGIS_logo.png",
    "apple.png",
    "apple_st_grayscale.png",
    "ccbysa.png",
    "check.png",
    "matrix_autocast.png",
    "matrix_checkmark.png",
    "matrix_sfcgal_enhanced.png",
    "matrix_sfcgal_required.png",
    "matrix_transform.png",
    "osgeo_logo.png",
    "rt_st_transform01.png",
    "rt_st_transform02.png",
    "rt_st_transform03.png",
    "st_band01.png",
    "st_band02.png",
    "st_band03.png",
    "st_clip01.png",
    "st_clip02.png",
    "st_clip03.png",
    "st_clip04.png",
    "st_clip05.png",
    "st_clusterkmeans03.png",
    "st_hexagongrid01.png",
    "st_hexagongrid03.png",
    "st_mapalgebraexpr01.png",
    "st_mapalgebraexpr02.png",
    "st_mapalgebraexpr2_08.png",
    "st_mapalgebrafctngb01.png",
    "st_mapalgebrafctngb02.png",
}

VERBATIM_LINE_LIMIT = 120
VERBATIM_TABLE_LINE_LIMIT = 120
VERBATIM_LONG_LINE_ROLE = "docbook-qa-ignore-wide-lines"
VERBATIM_SQL_INDENT_ROLE = "docbook-qa-ignore-sql-indent"
VERBATIM_HEX_PAYLOAD_RE = re.compile(r"\A\\x[0-9A-Fa-f]{64,}\Z")
VERBATIM_BASE64_PAYLOAD_RE = re.compile(r"\A[A-Za-z0-9+/]{88,}={0,2}\Z")
VERBATIM_PSQL_TABLE_ROW_RE = re.compile(r"^\s*[^|]*\|[^|]*\|.*$")
VERBATIM_SIMPLE_SCALAR_RE = re.compile(r"^[A-Za-z0-9_.:+-]+$")
SQL_STANDALONE_FROM_STAIRCASE_RE = re.compile(r"(?mi)^ {8,}FROM\s*$")
SQL_SUBQUERY_STAIRCASE_RE = re.compile(
    r"(?mi)^ {4,}FROM\s*\(\s*(?:\n\s*)?SELECT\b[^\n]*\n"
    r" {12,}(?:SELECT|FROM|WHERE|GROUP BY|ORDER BY|HAVING|JOIN|INNER JOIN|LEFT JOIN|RIGHT JOIN)\b"
)
SQL_INLINE_SUBQUERY_STAIRCASE_RE = re.compile(
    r"(?mi)^ {4,}.*\b(?:EXISTS|ARRAY)[ \t]*\([ \t]*SELECT\b[^\n]*\n"
    r" {8,}(?:SELECT|FROM|WHERE|GROUP BY|ORDER BY|HAVING|JOIN|INNER JOIN|LEFT JOIN|RIGHT JOIN)\b"
)
SQL_SPLIT_CLAUSE_INDENT_RE = re.compile(
    r"(?mi)^(?:FROM|JOIN|INNER JOIN|LEFT JOIN|RIGHT JOIN|CROSS JOIN)\s*$\n"
    r"^[A-Za-z_][A-Za-z0-9_]*(?:\s|\()"
)

BLOCK_KINDS = {
    "programlisting": ("postgis-example-code", "code", "Code"),
    "screen": ("postgis-example-output", "output", "Output"),
}
FORBIDDEN_SCREEN_CLASSES = {"programlisting", "sql", "hljs", "highlight"}
SQL_STARTERS = tuple(
    f"{keyword} " if keyword not in {"BEGIN", "COMMIT"} else f"{keyword};" for keyword in SQL_BLOCK_STARTER_KEYWORDS
)

IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
OPERATOR_RE = re.compile(r"^[^A-Za-z0-9_\s]+$")

SEVERITY_ORDER = {"info": 0, "warning": 1, "error": 2}
_INDEX = None


@dataclass(frozen=True)
class Finding:
    severity: str
    category: str
    location: str
    message: str


def load_xml(path: Path) -> ET.ElementTree:
    global _INDEX
    _INDEX = parse_xml(path)
    return _INDEX.tree


def load_html(path: Path) -> ET.ElementTree:
    return load_xml(path)


def local_name(node_or_tag: ET.Element | str) -> str:
    tag = node_or_tag.tag if hasattr(node_or_tag, "tag") else str(node_or_tag)
    return tag.rsplit("}", 1)[-1]


def normalized_text(node: ET.Element) -> str:
    return " ".join("".join(node.itertext()).split())


def block_text(node: ET.Element) -> str:
    return "".join(node.itertext())


def class_tokens(node: ET.Element) -> set[str]:
    return set((node.get("class") or "").split())


def role_tokens(node: ET.Element) -> set[str]:
    return set((node.get("role") or "").split())


def text_content(node: ET.Element | None) -> str:
    if node is None:
        return ""
    return " ".join("".join(node.itertext()).split())


def raw_text_content(node: ET.Element) -> str:
    return "".join(node.itertext()).strip()


def element_id(node: ET.Element) -> str | None:
    return node.get(XML_ID) or node.get("id")


def display_element_id(node: ET.Element) -> str:
    return node.get("id") or "<no id>"


def ancestors(node: ET.Element):
    return _INDEX.ancestors(node) if _INDEX is not None else ()


def nearest_refentry_id(node: ET.Element) -> str:
    for ancestor in ancestors(node):
        if local_name(ancestor) == "refentry":
            return element_id(ancestor) or "<unknown-refentry>"
    return "<no-refentry>"


def source_location(path: Path, node: ET.Element) -> str:
    line = (_INDEX.line(node) if _INDEX is not None else None) or "?"
    return f"{path}:{line} ({nearest_refentry_id(node)})"


def html_location(path: Path, message: str) -> str:
    # Keep the legacy generated-HTML checker's location text as the finding location
    # so behavior comparisons can match the old path/message strings directly.
    return f"{path}: {message}"


def first_matching(pattern: re.Pattern[str], text: str) -> str | None:
    match = pattern.search(text)
    if not match:
        return None
    return match.group(0).strip()


def next_element_sibling(node: ET.Element) -> ET.Element | None:
    return _INDEX.next_sibling(node) if _INDEX is not None else None


def refname_used_in_examples(refname: str, example_text: str) -> bool:
    bare = refname.split("(", 1)[0].strip()
    if IDENT_RE.fullmatch(bare):
        return re.search(rf"(?i)(?<![A-Za-z0-9_]){re.escape(bare)}\s*\(", example_text) is not None
    if OPERATOR_RE.fullmatch(bare):
        operator_chars = r"~!@#%^&|`?+\-*/<=>"
        return re.search(
            rf"(?<![{operator_chars}]){re.escape(bare)}(?![{operator_chars}])",
            example_text,
        ) is not None
    return False


def add_failure_options(parser: argparse.ArgumentParser, *, default_fail_on: str, default_max_warnings: int) -> None:
    parser.add_argument(
        "--max-warnings",
        type=int,
        default=default_max_warnings,
        help=(
            "maximum warning/error findings to print before the summary; "
            f"info findings have a separate report allowance (default: {default_max_warnings})"
        ),
    )
    parser.add_argument(
        "--fail-on",
        choices=("none", "error", "warning"),
        default=default_fail_on,
        help=f"finding severity that returns non-zero: none, error, or warning (default: {default_fail_on})",
    )


def named_descendants(node: ET.Element, *names: str):
    wanted = set(names)
    return (child for child in node.iter() if child is not node and local_name(child) in wanted)


def named_children(node: ET.Element, *names: str):
    wanted = set(names)
    return [child for child in node if local_name(child) in wanted]


def first_named_child(node: ET.Element, name: str):
    return next((child for child in node if local_name(child) == name), None)


def deprecated_alias_replacement(refentry: ET.Element) -> str | None:
    if DEPRECATED_ALIAS_ROLE not in role_tokens(refentry):
        return None
    warning = next(named_descendants(refentry, "warning"), None)
    if warning is None:
        return None
    targets = {
        node.get("linkend")
        for node in named_descendants(warning, "xref")
        if node.get("linkend")
    }
    return next(iter(targets)) if len(targets) == 1 else None


def is_description_badge(node: ET.Element) -> bool:
    if local_name(node) != "para":
        return False
    text = normalized_text(node)
    return (
        next(named_descendants(node, "inlinemediaobject"), None) is not None
        and text.startswith(("This function supports", "This method supports", "This method implements"))
    )


def description_is_support_only(section: ET.Element) -> bool:
    found_badge = False
    for child in section:
        if local_name(child) == "title":
            continue
        if local_name(child) == "para" and (child.get("role") or "") in DESCRIPTION_METADATA_ROLES:
            continue
        if is_description_badge(child):
            found_badge = True
            continue
        if normalized_text(child):
            return False
    return found_badge


def section_has_example_title(section: ET.Element) -> bool:
    title = first_named_child(section, "title")
    return title is not None and "Example" in normalized_text(title)


def line_contains_verbatim_payload(line: str) -> bool:
    stripped = line.strip()
    if not stripped:
        return False
    return (
        VERBATIM_HEX_PAYLOAD_RE.fullmatch(stripped) is not None
        or VERBATIM_BASE64_PAYLOAD_RE.fullmatch(stripped) is not None
    )


def line_is_psql_table_separator(line: str) -> bool:
    return PSQL_TABLE_SEPARATOR_RE.fullmatch(line) is not None or PSQL_UNICODE_TABLE_SEPARATOR_RE.fullmatch(line) is not None


def psql_table_delimiter_positions(line: str, delimiters: str) -> list[int]:
    return [index for index, char in enumerate(line) if char in delimiters]


def has_misaligned_psql_table(text: str) -> bool:
    lines = [line for line in text.splitlines() if line.strip()]
    for index in range(len(lines) - 1):
        header, separator = lines[index], lines[index + 1]
        if not line_is_psql_table_separator(separator):
            continue
        separator_positions = psql_table_delimiter_positions(separator, "+┼")
        if not separator_positions:
            continue
        expected_columns = len(separator_positions) + 1
        rows = [header]
        for row in lines[index + 2 :]:
            if PSQL_ROW_COUNT_RE.fullmatch(row) or line_is_psql_table_separator(row):
                break
            rows.append(row)
        for row in rows:
            row_positions = psql_table_delimiter_positions(row, "|│")
            if not row_positions:
                continue
            if len(row_positions) != expected_columns - 1:
                return True
            if any(abs(row_position - separator_position) > 1 for row_position, separator_position in zip(row_positions, separator_positions)):
                return True
    return False


def line_is_wide_psql_table(line: str) -> bool:
    stripped = line.strip()
    if not stripped:
        return False
    return line_is_psql_table_separator(line) or VERBATIM_PSQL_TABLE_ROW_RE.fullmatch(stripped) is not None


def summarize_long_verbatim_lines(node: ET.Element) -> tuple[list[int], bool]:
    if VERBATIM_LONG_LINE_ROLE in role_tokens(node):
        return [], False

    long_lines: list[int] = []
    has_psql_table = False
    is_output = local_name(node) == "screen"
    for line_no, line in enumerate(block_text(node).splitlines(), start=1):
        if line_contains_verbatim_payload(line):
            continue
        if is_output and line_is_wide_psql_table(line):
            if len(line) >= VERBATIM_TABLE_LINE_LIMIT:
                long_lines.append(line_no)
                has_psql_table = True
            continue
        if len(line) > VERBATIM_LINE_LIMIT:
            long_lines.append(line_no)

    return long_lines, has_psql_table


def wide_verbatim_recommendation(has_psql_table: bool) -> str:
    if has_psql_table:
        return (
            "line is wide; use psql expanded output (`\\x on`) or split this block into smaller"
            " table/output pieces"
        )
    return (
        "wrap long lines, split the example into smaller steps or blocks,"
        " and consider psql expanded output (`\\x on`) for wide tables"
    )


def has_ragged_scalar_output_indent(text: str) -> bool:
    lines = [line for line in text.splitlines() if line.strip()]
    if len(lines) < 2:
        return False
    if any(
        PSQL_ROW_COUNT_RE.fullmatch(line)
        or line_is_psql_table_separator(line)
        or line.strip().startswith("---")
        for line in lines
    ):
        return False
    first, second = lines[0], lines[1]
    if not first.startswith((" ", "\t")) or second.startswith((" ", "\t")):
        return False
    return all(VERBATIM_SIMPLE_SCALAR_RE.fullmatch(line.strip()) for line in lines)


def add_source_findings(root: ET.Element, path: Path) -> list[Finding]:
    findings: list[Finding] = []

    refentries = list(named_descendants(root, "refentry"))
    refentry_ids = {element_id(refentry) for refentry in refentries if element_id(refentry)}
    deprecated_aliases: dict[str, str] = {}
    for refentry in refentries:
        if DEPRECATED_ALIAS_ROLE not in role_tokens(refentry):
            continue
        alias = element_id(refentry)
        replacement = deprecated_alias_replacement(refentry)
        if not alias or not replacement or replacement == alias or replacement not in refentry_ids:
            findings.append(Finding(
                "error", "deprecated-alias-replacement", source_location(path, refentry),
                "deprecated alias must contain one warning xref to a different documented replacement",
            ))
            continue
        deprecated_aliases[alias] = replacement
        if any(section_has_example_title(section) for section in named_descendants(refentry, "refsection")):
            findings.append(Finding(
                "error", "deprecated-alias-examples", source_location(path, refentry),
                f"deprecated alias {alias} duplicates examples; keep examples on {replacement}",
            ))

    for node in named_descendants(root, "xref"):
        target = node.get("linkend")
        if target not in deprecated_aliases or nearest_refentry_id(node) == target:
            continue
        findings.append(Finding(
            "error", "deprecated-alias-link", source_location(path, node),
            f"link targets deprecated alias {target}; use {deprecated_aliases[target]}",
        ))

    for node in named_descendants(root, "imagedata"):
        fileref = (node.get("fileref") or "").strip()
        image_name = Path(fileref).name
        if (
            Path(image_name).suffix.lower() in RASTER_IMAGE_EXTENSIONS
            and image_name not in ALLOWED_SOURCE_RASTER_IMAGES
        ):
            findings.append(Finding(
                "error", "legacy-raster-image", source_location(path, node),
                f"substantive raster image {fileref!r} must be replaced by an executable "
                "visual example, DocBook markup, or another vector source",
            ))

    for refentry in refentries:
        if DEPRECATED_ALIAS_ROLE in role_tokens(refentry):
            continue
        sections = named_children(refentry, "refsection", "section", "sect1")
        description_sections = []
        for section in sections:
            title = first_named_child(section, "title")
            if title is None or normalized_text(title) != "Description":
                continue
            description_sections.append(section)
            if description_is_support_only(section):
                findings.append(Finding(
                    "warning", "support-only-description", source_location(path, section),
                    "Description contains support/compliance badges but no semantic prose",
                ))
        refnamediv = first_named_child(refentry, "refnamediv")
        if (
            not description_sections
            and refnamediv is not None
            and first_named_child(refnamediv, "refpurpose") is not None
            and first_named_child(refentry, "refsynopsisdiv") is not None
        ):
            findings.append(Finding(
                "warning", "missing-description", source_location(path, refentry),
                "documented reference entry has a synopsis but no Description section",
            ))

    for node in named_descendants(root, "programlisting", "title"):
        owner = nearest_refentry_id(node)
        text = block_text(node)
        for alias, replacement in deprecated_aliases.items():
            if owner == alias or not re.search(rf"(?<![A-Za-z0-9_]){re.escape(alias)}(?![A-Za-z0-9_])", text):
                continue
            findings.append(Finding(
                "error", "deprecated-alias-reference", source_location(path, node),
                f"content references deprecated alias {alias}; use {replacement}",
            ))

    for node in named_descendants(root, "programlisting", "screen"):
        if "\t" in block_text(node):
            findings.append(Finding(
                "info", "tab-in-verbatim", source_location(path, node),
                f"{local_name(node)} contains a TAB character; tabs render at 8 columns and can break alignment",
            ))

    for node in named_descendants(root, *ADMONITIONS):
        sibling = next_element_sibling(node)
        if sibling is not None and local_name(sibling) in ADMONITIONS:
            findings.append(Finding(
                "info", "stacked-admonitions", source_location(path, sibling),
                f"adjacent {local_name(node)} and {local_name(sibling)} admonitions should be consolidated or moved into prose",
            ))
        verbatim = next(named_descendants(node, "programlisting", "screen"), None)
        if verbatim is not None:
            findings.append(Finding(
                "error", "admonition-contains-verbatim", source_location(path, verbatim),
                f"{local_name(node)} contains a {local_name(verbatim)} block; move code and output blocks next to the admonition",
            ))
        if version := ANCIENT_VERSION_RE.search(block_text(node)):
            findings.append(Finding(
                "info", "ancient-version-note", source_location(path, node),
                f"admonition mentions pre-2.0 PostGIS version {version.group(0)!r}; move relevant history into prose or drop it",
            ))

    for section in named_descendants(root, "refsection"):
        if not section_has_example_title(section):
            continue
        for node in named_descendants(section, "programlisting"):
            if SELECT_START_RE.search(block_text(node)):
                sibling = next_element_sibling(node)
                if sibling is None or local_name(sibling) != "screen":
                    findings.append(Finding(
                        "info", "example-missing-output", source_location(path, node),
                        "SELECT example is not immediately followed by a screen containing its output",
                    ))

    for refentry in named_descendants(root, "refentry"):
        example_sections = [
            section for section in named_descendants(refentry, "refsection")
            if section_has_example_title(section)
        ]
        programlistings = [
            node for section in example_sections
            for node in named_descendants(section, "programlisting")
        ]
        synopsis = first_named_child(refentry, "refsynopsisdiv")
        if (
            not programlistings
            or synopsis is None
            or next(named_descendants(synopsis, "function"), None) is None
        ):
            continue
        refnamediv = first_named_child(refentry, "refnamediv")
        documented_calls = [
            alias.strip()
            for node in (named_children(refnamediv, "refname") if refnamediv is not None else [])
            for alias in normalized_text(node).split("/") if alias.strip()
        ]
        example_text = "\n".join(block_text(node) for node in programlistings)
        if documented_calls and not any(refname_used_in_examples(name, example_text) for name in documented_calls):
            findings.append(Finding(
                "info", "current-function-in-examples", source_location(path, example_sections[0]),
                f"Examples do not call the documented function or operator ({', '.join(documented_calls)})",
            ))

    for node in named_descendants(root, "para"):
        sibling = next_element_sibling(node)
        if sibling is not None and local_name(sibling) == "screen":
            caption = normalized_text(node)
            if REDUNDANT_OUTPUT_CAPTION_RE.fullmatch(caption):
                findings.append(Finding(
                    "warning", "redundant-output-caption", source_location(path, node),
                    f"bare output caption {caption!r} duplicates the following screen block label; remove the para",
                ))

    for node in named_descendants(root, "programlisting"):
        text = block_text(node)
        if LEADING_SQL_COMMENT_RE.match(text):
            findings.append(Finding(
                "warning", "leading-sql-comment", source_location(path, node),
                "programlisting begins with comments before its first SQL or psql command; "
                "move the example description into a preceding <para>",
            ))
        if (
            VERBATIM_SQL_INDENT_ROLE not in role_tokens(node)
            and (
                SQL_STANDALONE_FROM_STAIRCASE_RE.search(text)
                or SQL_SUBQUERY_STAIRCASE_RE.search(text)
                or SQL_INLINE_SUBQUERY_STAIRCASE_RE.search(text)
                or SQL_SPLIT_CLAUSE_INDENT_RE.search(text)
            )
        ):
            findings.append(Finding(
                "warning", "sql-staircase-indent", source_location(path, node),
                "SQL programlisting contains deeply-indented nested SELECT/FROM clauses; "
                "format nested SELECT/FROM blocks with ordinary two-space indentation",
            ))
        if not SQL_STATEMENT_RE.search(text):
            continue
        markers = []
        for label, pattern in (
            ("psql row count", PSQL_ROW_COUNT_RE),
            ("psql table separator", PSQL_TABLE_SEPARATOR_RE),
            ("output dash run", OUTPUT_DASH_RUN_RE),
            ("server message", PSQL_MESSAGE_RE),
        ):
            if snippet := first_matching(pattern, text):
                markers.append(f"{label}: {snippet!r}")
        if markers:
            findings.append(Finding(
                "warning", "mixed-programlisting-output", source_location(path, node),
                "programlisting appears to contain SQL input plus output markers; "
                "split SQL into <programlisting> and result text into <screen> "
                f"({'; '.join(markers)})",
            ))

    for node in named_descendants(root, "screen"):
        text = block_text(node)
        if SQL_START_RE.search(text) and ";" in text:
            findings.append(Finding(
                "warning", "screen-contains-sql", source_location(path, node),
                "screen block looks like it contains SQL input; use programlisting for input and screen for output",
            ))
        if command := first_matching(PSQL_META_COMMAND_RE, text):
            findings.append(Finding(
                "warning", "screen-contains-psql-command", source_location(path, node),
                f"screen block contains psql input {command!r}; move the backslash command to a preceding programlisting",
            ))
        if has_ragged_scalar_output_indent(text):
            findings.append(Finding(
                "warning", "ragged-scalar-output-indent", source_location(path, node),
                "screen output starts with a stray leading indent on the first scalar line; "
                "left-align scalar output or format it as an aligned psql table",
            ))
        if has_misaligned_psql_table(text):
            findings.append(Finding(
                "info", "misaligned-psql-table", source_location(path, node),
                "screen contains a psql-style table whose column separators do not line up; "
                "align header, separator, and data rows before rendering",
            ))

    for node in named_descendants(root, "programlisting", "screen"):
        long_lines, has_psql_table = summarize_long_verbatim_lines(node)
        if not long_lines:
            continue
        preview = ", ".join(str(line_no) for line_no in long_lines[:3])
        if len(long_lines) > 3:
            preview += f", ... (+{len(long_lines) - 3} more)"
        findings.append(Finding(
            "info",
            "wide-output-table" if has_psql_table else "wide-verbatim-line",
            source_location(path, node),
            f"{local_name(node)} has {len(long_lines)} wide line(s) ({'lines' if len(long_lines) != 1 else 'line'}: {preview}); {wide_verbatim_recommendation(has_psql_table)}",
        ))

    for refentry in named_descendants(root, "refentry"):
        seen: dict[str, ET.Element] = {}
        example_titles: list[ET.Element] = []
        for section in named_children(refentry, "refsection", "section", "sect1"):
            title_node = first_named_child(section, "title")
            if title_node is None:
                continue
            title = normalized_text(title_node)
            if EXAMPLE_SECTION_TITLE_RE.match(title):
                example_titles.append(title_node)
            if title not in {"Description", "Examples"}:
                continue
            if title in seen:
                first_line = _INDEX.line(seen[title]) if _INDEX is not None else None
                findings.append(Finding(
                    "error", "duplicate-refsection-title", source_location(path, title_node),
                    f"refentry has more than one {title!r} section; first one is at line {first_line or '?'}",
                ))
            else:
                seen[title] = title_node
        for title_node in example_titles[1:]:
            first_line = _INDEX.line(example_titles[0]) if _INDEX is not None else None
            findings.append(Finding(
                "error", "repeated-example-sections", source_location(path, title_node),
                "refentry has multiple Example/Examples sections; keep one 'Examples' section "
                f"and use introductory paragraphs for individual cases (first at line {first_line or '?'})",
            ))

    for node in named_descendants(root, "para"):
        role = node.get("role") or "<missing>"
        if role in {"availability", "enhanced", "changed"} and any(
            local_name(ancestor) in ADMONITIONS for ancestor in ancestors(node)
        ):
            findings.append(Finding(
                "warning", "metadata-role-in-admonition", source_location(path, node),
                f"metadata para role={role!r} is nested inside an admonition; prefer a plain role para in Description",
            ))
    return findings


def looks_like_sql_input(text: str) -> bool:
    normalized = " ".join(text.split()).upper()
    return normalized.startswith(SQL_STARTERS)


def find_label(parent: ET.Element, label_id: str) -> ET.Element | None:
    for child in parent.iter():
        if child is not parent and child.get("id") == label_id and "postgis-example-label" in class_tokens(child):
            return child
    return None


def copy_buttons(parent: ET.Element) -> list[ET.Element]:
    return [node for node in parent.iter() if "postgis-copy-button" in class_tokens(node)]


def html_error(path: Path, message: str) -> Finding:
    return Finding("error", "html-block-label", html_location(path, message), "")


def check_html_file(path: Path, required_kinds: set[str]) -> list[Finding]:
    findings: list[Finding] = []
    tree = load_html(path)
    root = tree.getroot()
    counts = {"programlisting": 0, "screen": 0}

    for pre in root.iter():
        if local_name(pre) != "pre":
            continue
        tokens = class_tokens(pre)
        block_class = next((name for name in BLOCK_KINDS if name in tokens), None)
        if block_class is None:
            continue

        counts[block_class] += 1
        wrapper_class, expected_data_block, expected_english_label = BLOCK_KINDS[block_class]
        parent = _INDEX.parents.get(pre) if _INDEX is not None else None
        if parent is None or local_name(parent) != "div":
            findings.append(html_error(path, f"{block_class} block {display_element_id(pre)} is not wrapped in a div"))
            continue

        parent_tokens = class_tokens(parent)
        if "postgis-example-block" not in parent_tokens or wrapper_class not in parent_tokens:
            findings.append(
                html_error(path, f"{block_class} block {display_element_id(pre)} wrapper classes are {sorted(parent_tokens)}")
            )
        if parent.get("role") != "group":
            findings.append(html_error(path, f"{block_class} block {display_element_id(pre)} wrapper lacks role=group"))
        if parent.get("data-postgis-block") != expected_data_block:
            findings.append(
                html_error(
                    path,
                    f"{block_class} block {display_element_id(pre)} wrapper data-postgis-block is "
                    f"{parent.get('data-postgis-block')!r}, expected {expected_data_block!r}",
                )
            )

        label_id = parent.get("aria-labelledby")
        if not label_id:
            findings.append(html_error(path, f"{block_class} block {display_element_id(pre)} wrapper lacks aria-labelledby"))
            continue

        label = find_label(parent, label_id)
        if label is None:
            findings.append(html_error(path, f"{block_class} block {display_element_id(pre)} wrapper lacks visible label {label_id}"))
            continue
        label_text = raw_text_content(label)
        if not label_text:
            findings.append(html_error(path, f"{block_class} block {display_element_id(pre)} wrapper has an empty visible label"))
        elif ("postgis-en" in str(path) or str(path).endswith("-en.html")) and label_text != expected_english_label:
            findings.append(
                html_error(
                    path,
                    f"{block_class} block {display_element_id(pre)} label is {label_text!r}, "
                    f"expected {expected_english_label!r}",
                )
            )

        buttons = copy_buttons(parent)
        if block_class == "programlisting":
            language = pre.get("data-postgis-language") or parent.get("data-postgis-language")
            if len(buttons) != 1:
                findings.append(html_error(path, f"code block {display_element_id(pre)} has {len(buttons)} copy buttons, expected 1"))
            elif buttons:
                button = buttons[0]
                if button.get("type") != "button":
                    findings.append(html_error(path, f"code block {display_element_id(pre)} copy button lacks type=button"))
                if not (button.get("aria-label") and button.get("title")):
                    findings.append(html_error(path, f"code block {display_element_id(pre)} copy button lacks accessible label/title"))

            if language:
                expected_token = f"language-{language}"
                expected_wrapper_token = f"postgis-example-language-{language}"
                if pre.get("data-postgis-language") != language:
                    findings.append(html_error(path, f"code block {display_element_id(pre)} lacks pre data-postgis-language={language!r}"))
                if parent.get("data-postgis-language") != language:
                    findings.append(html_error(path, f"code block {display_element_id(pre)} lacks wrapper data-postgis-language={language!r}"))
                if expected_token not in tokens:
                    findings.append(html_error(path, f"code block {display_element_id(pre)} lacks {expected_token!r} class"))
                if expected_wrapper_token not in parent_tokens:
                    findings.append(html_error(path, f"code block {display_element_id(pre)} lacks {expected_wrapper_token!r} wrapper class"))
            elif looks_like_sql_input(raw_text_content(pre)):
                findings.append(html_error(path, f"SQL-looking code block {display_element_id(pre)} lacks data-postgis-language='sql'"))
        if block_class == "screen":
            forbidden = (tokens & FORBIDDEN_SCREEN_CLASSES) | {token for token in tokens if token.startswith("language-")}
            if forbidden:
                findings.append(
                    html_error(path, f"screen/output block {display_element_id(pre)} has code/highlight class tokens {sorted(forbidden)}")
                )
            for attr in ("data-postgis-language", "data-postgis-highlighted"):
                if pre.get(attr) is not None or parent.get(attr) is not None:
                    findings.append(html_error(path, f"screen/output block {display_element_id(pre)} unexpectedly has {attr}"))
            if buttons:
                findings.append(html_error(path, f"screen/output block {display_element_id(pre)} unexpectedly has a copy button"))

    for block_class in required_kinds:
        if counts.get(block_class, 0) == 0:
            findings.append(html_error(path, f"no generated {block_class} blocks found"))

    for figure in root.iter():
        if local_name(figure) != "div" or "postgis-geometry-figure" not in class_tokens(figure):
            continue
        visual_id = figure.get("data-postgis-visual-id")
        parent = _INDEX.parents.get(figure) if _INDEX is not None else None
        siblings = list(parent) if parent is not None else []
        index = siblings.index(figure) if figure in siblings else -1

        def previous_stack_sibling(position: int):
            position -= 1
            while position >= 0:
                sibling = siblings[position]
                if local_name(sibling) == "p" and not raw_text_content(sibling).strip() and not list(sibling):
                    position -= 1
                    continue
                return sibling, position
            return None, -1

        output, output_index = previous_stack_sibling(index)
        code, _code_index = previous_stack_sibling(output_index)
        if not visual_id:
            findings.append(html_error(path, "generated geometry figure lacks data-postgis-visual-id"))
            continue
        if (
            output is None or code is None
            or "postgis-example-output" not in class_tokens(output)
            or "postgis-example-code" not in class_tokens(code)
        ):
            findings.append(html_error(path, f"geometry figure {visual_id!r} is not adjacent to Code and Output blocks"))
            continue
        if output.get("data-postgis-visual-id") != visual_id or code.get("data-postgis-visual-id") != visual_id:
            findings.append(html_error(path, f"Code, Output, and Figure do not share visual id {visual_id!r}"))

    return findings


def refentry_filename(refentry: ET.Element) -> str | None:
    entry_id = refentry_id(refentry)
    if not entry_id:
        return None
    return f"{entry_id}.html"


def has_programlisting_outside_refsynopsis(refentry: ET.Element) -> bool:
    for node in refentry.findall(f".//{DB}programlisting"):
        if not any(local_name(ancestor) == "refsynopsisdiv" for ancestor in ancestors(node)):
            return True
    return False


def refentry_expected_kinds(refentry: ET.Element) -> set[str]:
    kinds: set[str] = set()
    if has_programlisting_outside_refsynopsis(refentry):
        kinds.add("programlisting")
    if refentry.find(f".//{DB}screen") is not None:
        kinds.add("screen")
    return kinds


def build_html_expectations(input_path: Path) -> dict[str, set[str]]:
    root = load_xml(input_path).getroot()
    expectations: dict[str, set[str]] = {}
    for refentry in root.findall(f".//{DB}refentry"):
        filename = refentry_filename(refentry)
        if filename is None:
            continue
        expectations.setdefault(filename, set()).update(refentry_expected_kinds(refentry))
    return expectations


def expected_kinds_for_page(expectations: dict[str, set[str]], page: Path) -> set[str]:
    return expectations.get(page.name, set())


def parse_required_kinds(value: str) -> set[str]:
    if not value:
        return set()
    kinds = {item.strip() for item in value.split(",") if item.strip()}
    unknown = kinds - set(BLOCK_KINDS)
    if unknown:
        raise argparse.ArgumentTypeError(f"unknown block kind(s): {', '.join(sorted(unknown))}")
    return kinds


def brief(text: str, limit: int = 220) -> str:
    text = " ".join(text.split())
    if len(text) <= limit:
        return text
    cut = text.rfind(" ", 0, limit - 1)
    if cut < 80:
        cut = limit - 1
    return text[:cut].rstrip() + "…"


def refentry_id(refentry: ET.Element) -> str:
    return refentry.get(XML_ID) or refentry.get("id") or ""


def make_entry(label: str, entry_id: str, purpose: str, href: str | None = None) -> dict[str, str]:
    return {
        "id": entry_id,
        "label": label,
        "href": href or f"{entry_id}.html",
        "title": brief(purpose) if purpose else label,
    }


def add_function(target: dict[str, dict[str, str]], key: str, label: str, entry_id: str, purpose: str) -> None:
    if not key or key in target:
        return
    target[key] = make_entry(label, entry_id, purpose)


def add_operator(target: dict[str, list[dict[str, str]]], key: str, label: str, entry_id: str, purpose: str) -> None:
    if not key:
        return
    entries = target.setdefault(key, [])
    if any(entry["id"] == entry_id for entry in entries):
        return
    entries.append(make_entry(label, entry_id, purpose))


def command_reference(command: ET.Element) -> tuple[str, dict[str, str]] | None:
    if COMMAND_REFERENCE_ROLE not in role_tokens(command):
        return None
    label = text_content(command)
    target = next((ancestor for ancestor in ancestors(command) if element_id(ancestor)), None)
    owner = next(
        (
            ancestor
            for ancestor in ancestors(command)
            if local_name(ancestor) in CHUNK_OWNER_ELEMENTS and element_id(ancestor)
        ),
        None,
    )
    if not label or target is None or owner is None:
        return None
    target_id = element_id(target) or ""
    owner_id = element_id(owner) or ""
    target_title = text_content(first_named_child(target, "title")) or label
    href = f"{owner_id}.html"
    if target_id != owner_id:
        href += f"#{target_id}"
    return label, make_entry(label, target_id, target_title, href)


def build_index(input_path: Path) -> dict[str, Any]:
    root = load_xml(input_path).getroot()
    functions: dict[str, dict[str, str]] = {}
    operators: dict[str, list[dict[str, str]]] = {}
    commands: dict[str, dict[str, str]] = {}

    for refentry in root.findall(f".//{DB}refentry"):
        entry_id = refentry_id(refentry)
        if not entry_id:
            continue
        purpose = text_content(refentry.find(f"./{DB}refnamediv/{DB}refpurpose"))
        indexed_entry_id = deprecated_alias_replacement(refentry) or entry_id
        for refname in refentry.findall(f"./{DB}refnamediv/{DB}refname"):
            name = text_content(refname)
            if not name:
                continue
            bare = name.split("(", 1)[0].strip()
            if IDENT_RE.match(bare):
                add_function(functions, bare.upper(), bare, indexed_entry_id, purpose)
            elif OPERATOR_RE.match(bare):
                add_operator(operators, bare, bare, indexed_entry_id, purpose)

    for command in root.findall(f".//{DB}command"):
        reference = command_reference(command)
        if reference is not None:
            label, entry = reference
            commands.setdefault(label, entry)

    return {
        "commands": commands,
        "functions": functions,
        "operators": operators,
        "keywords": list(SQL_HIGHLIGHT_KEYWORDS),
    }


def should_fail(findings: Iterable[Finding], fail_on: str) -> bool:
    if fail_on == "none":
        return False
    threshold = SEVERITY_ORDER[fail_on]
    return any(SEVERITY_ORDER[item.severity] >= threshold for item in findings)


def print_findings(findings: list[Finding], max_findings: int) -> None:
    limit = max(max_findings, 0)
    for severity_group, label in (
        ([item for item in findings if item.severity != "info"], "warning/error"),
        ([item for item in findings if item.severity == "info"], "info"),
    ):
        for item in severity_group[:limit]:
            suffix = f": {item.message}" if item.message else ""
            print(f"{item.severity.upper()} {item.category}: {item.location}{suffix}")
        if max_findings >= 0 and len(severity_group) > max_findings:
            print(f"... {len(severity_group) - max_findings} additional {label} findings suppressed")


def print_summary(tool_name: str, clean_message: str, findings: list[Finding], fail_on: str) -> None:
    severity_counts = Counter(item.severity for item in findings)
    category_counts = Counter(item.category for item in findings)
    if category_counts:
        severity_summary = ", ".join(f"{severity}={count}" for severity, count in sorted(severity_counts.items()))
        category_summary = ", ".join(f"{category}={count}" for category, count in sorted(category_counts.items()))
        print(f"{tool_name} summary: {len(findings)} finding(s): {severity_summary}; {category_summary}")
    else:
        print(f"{tool_name} summary: {clean_message}")
    if not should_fail(findings, fail_on) and findings:
        print(f"{tool_name} did not fail because --fail-on={fail_on}.")


def command_lint_source(args: argparse.Namespace) -> int:
    fail_on = "warning" if getattr(args, "fail_on_warning", False) else args.fail_on
    findings = add_source_findings(load_xml(args.xml).getroot(), args.xml)
    print_findings(findings, args.max_warnings)
    print_summary("DocBook markup checker", "no suspicious DocBook example markup found.", findings, fail_on)
    return 1 if should_fail(findings, fail_on) else 0


def command_lint_html(args: argparse.Namespace) -> int:
    findings: list[Finding] = []
    expectations = build_html_expectations(args.expect_from) if args.expect_from else {}
    for page in args.pages:
        path = Path(page)
        if not path.exists():
            findings.append(Finding("error", "html-block-label", f"{path}: file does not exist", ""))
            continue
        required_kinds = set(args.require_kinds) | expected_kinds_for_page(expectations, path)
        if args.show_expectations and path.name in expectations:
            expected = ",".join(sorted(required_kinds)) or "<none>"
            print(f"EXPECT {path.name}: {expected}", file=sys.stderr)
        findings.extend(check_html_file(path, required_kinds))
    print_findings(findings, args.max_warnings)
    print_summary("DocBook block label checker", "no generated HTML block label issues found.", findings, args.fail_on)
    return 1 if should_fail(findings, args.fail_on) else 0


def command_ref_index(args: argparse.Namespace) -> int:
    index = build_index(args.xml)
    serialized_index = json.dumps(index, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    script_output = args.output.with_suffix(".js")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(serialized_index + "\n")
    script_output.write_text(f"window.POSTGIS_REF_INDEX = {serialized_index};\n")
    print(
        f"wrote {args.output} and {script_output}; commands={len(index['commands'])}, "
        f"functions={len(index['functions'])}, "
        f"operator pages={sum(len(entries) for entries in index['operators'].values())}, "
        f"SQL keywords={len(index['keywords'])}",
        file=sys.stderr,
    )
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    source = subparsers.add_parser("lint-source", help="lint expanded DocBook XML source")
    add_failure_options(source, default_fail_on="none", default_max_warnings=50)
    source.add_argument("--fail-on-warning", action="store_true", help="deprecated alias for --fail-on warning")
    source.add_argument("xml", type=Path, help="generated postgis-out.xml or another DocBook XML file")
    source.set_defaults(func=command_lint_source)

    html = subparsers.add_parser("lint-html", help="lint generated chunked manual HTML block labels")
    add_failure_options(html, default_fail_on="error", default_max_warnings=50)
    html.add_argument(
        "--require-kinds",
        type=parse_required_kinds,
        default=set(),
        help="comma-separated block kinds that every input page must contain, for focused fixtures",
    )
    html.add_argument(
        "--expect-from",
        type=Path,
        help="expanded DocBook XML whose refentries define per-page required block kinds",
    )
    html.add_argument(
        "--show-expectations",
        action="store_true",
        help="print derived per-page block-kind expectations to stderr",
    )
    html.add_argument("pages", nargs="+", metavar="GENERATED_PAGE.html")
    html.set_defaults(func=command_lint_html)

    index = subparsers.add_parser("ref-index", help="build function/operator JSON index for manual links")
    index.add_argument("xml", type=Path, help="generated postgis-out.xml")
    index.add_argument("output", type=Path, help="output postgis-ref-index.json")
    index.set_defaults(func=command_ref_index)

    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
