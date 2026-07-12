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
SQL_KEYWORD_DATA = (
    # keyword, starts a semicolon-terminated SQL statement, starts an example code block
    ("ADD", False, False),
    ("ALTER", True, True),
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
    ("DISTINCT", False, False),
    ("DO", False, True),
    ("DROP", True, True),
    ("ELSE", False, False),
    ("END", False, False),
    ("EXCEPT", False, False),
    ("EXPLAIN", True, True),
    ("FALSE", False, False),
    ("FROM", False, False),
    ("FULL", False, False),
    ("GROUP", False, False),
    ("HAVING", False, False),
    ("IN", False, False),
    ("INNER", False, False),
    ("INSERT", True, True),
    ("INTERSECT", False, False),
    ("INTO", False, False),
    ("IS", False, False),
    ("JOIN", False, False),
    ("LEFT", False, False),
    ("LIMIT", False, False),
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
    ("SET", False, False),
    ("THEN", False, False),
    ("TRUE", False, False),
    ("UNION", False, False),
    ("UPDATE", True, True),
    ("USING", False, False),
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
PSQL_ROW_COUNT_RE = re.compile(r"(?m)^\s*\(\d+\s+rows?\)\s*$")
PSQL_TABLE_SEPARATOR_RE = re.compile(r"(?m)^\s*[-+]{3,}(?:\s*\+\s*[-+]{2,})+\s*$")
OUTPUT_DASH_RUN_RE = re.compile(r"(?m)^\s*-{3,}")
PSQL_MESSAGE_RE = re.compile(r"(?m)^\s*(?:ERROR|NOTICE|WARNING|DETAIL|HINT|CONTEXT):")
REDUNDANT_OUTPUT_CAPTION_RE = re.compile(r"(?:output|result|results|returns|the output):?", re.IGNORECASE)
ANCIENT_VERSION_RE = re.compile(r"\b(?:0\.\d+|1\.[0-5])(?:\.\d+)?\b")
SELECT_START_RE = re.compile(r"^\s*SELECT\b", re.IGNORECASE | re.MULTILINE)
EXAMPLE_SECTION_TITLE_RE = re.compile(r"^(?:same\s+)?examples?\b", re.IGNORECASE)

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


def section_has_example_title(section: ET.Element) -> bool:
    title = first_named_child(section, "title")
    return title is not None and "Example" in normalized_text(title)


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


def make_entry(label: str, entry_id: str, purpose: str) -> dict[str, str]:
    return {
        "id": entry_id,
        "label": label,
        "href": f"{entry_id}.html",
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


def build_index(input_path: Path) -> dict[str, Any]:
    root = load_xml(input_path).getroot()
    functions: dict[str, dict[str, str]] = {}
    operators: dict[str, list[dict[str, str]]] = {}

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

    return {"functions": functions, "operators": operators, "keywords": list(SQL_HIGHLIGHT_KEYWORDS)}


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
        f"wrote {args.output} and {script_output} with {len(index['functions'])} functions, "
        f"{sum(len(entries) for entries in index['operators'].values())} operator pages, and "
        f"{len(index['keywords'])} SQL keywords",
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
