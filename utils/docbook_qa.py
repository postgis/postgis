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

try:
    from lxml import etree
except ImportError as exc:  # pragma: no cover - dependency issue is environmental
    print(f"docbook_qa.py: lxml is required: {exc}", file=sys.stderr)
    raise SystemExit(2) from None

DOCBOOK_NS = "http://docbook.org/ns/docbook"
XML_NS = "http://www.w3.org/XML/1998/namespace"
NS = {"d": DOCBOOK_NS, "xml": XML_NS}
DB = f"{{{DOCBOOK_NS}}}"
XML_ID = f"{{{XML_NS}}}id"

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

BLOCK_KINDS = {
    "programlisting": ("postgis-example-code", "code", "Code"),
    "screen": ("postgis-example-output", "output", "Output"),
}
FORBIDDEN_SCREEN_CLASSES = {"programlisting", "sql", "hljs", "highlight"}
SQL_STARTERS = (
    "SELECT ",
    "WITH ",
    "VALUES ",
    "INSERT ",
    "UPDATE ",
    "DELETE ",
    "CREATE ",
    "ALTER ",
    "DROP ",
    "EXPLAIN ",
    "DO ",
    "BEGIN;",
    "COMMIT;",
)

IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
OPERATOR_RE = re.compile(r"^[^A-Za-z0-9_\s]+$")

SEVERITY_ORDER = {"warning": 1, "error": 2}


@dataclass(frozen=True)
class Finding:
    severity: str
    category: str
    location: str
    message: str


def load_xml(path: Path) -> etree._ElementTree:
    parser = etree.XMLParser(resolve_entities=False, load_dtd=False, no_network=True)
    return etree.parse(str(path), parser)


def load_html(path: Path) -> etree._ElementTree:
    parser = etree.HTMLParser(encoding="utf-8")
    return etree.parse(str(path), parser)


def local_name(node_or_tag: etree._Element | str) -> str:
    tag = node_or_tag.tag if hasattr(node_or_tag, "tag") else str(node_or_tag)
    return etree.QName(tag).localname if tag.startswith("{") else tag


def normalized_text(node: etree._Element) -> str:
    return " ".join("".join(node.itertext()).split())


def block_text(node: etree._Element) -> str:
    return "".join(node.itertext())


def class_tokens(node: etree._Element) -> set[str]:
    return set((node.get("class") or "").split())


def text_content(node: etree._Element | None) -> str:
    if node is None:
        return ""
    return " ".join("".join(node.itertext()).split())


def raw_text_content(node: etree._Element) -> str:
    return "".join(node.itertext()).strip()


def element_id(node: etree._Element) -> str | None:
    return node.get(XML_ID) or node.get("id")


def display_element_id(node: etree._Element) -> str:
    return node.get("id") or "<no id>"


def nearest_refentry_id(node: etree._Element) -> str:
    for ancestor in node.iterancestors():
        if local_name(ancestor) == "refentry":
            return element_id(ancestor) or "<unknown-refentry>"
    return "<no-refentry>"


def source_location(path: Path, node: etree._Element) -> str:
    line = node.sourceline or "?"
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


def add_failure_options(parser: argparse.ArgumentParser, *, default_fail_on: str, default_max_warnings: int) -> None:
    parser.add_argument(
        "--max-warnings",
        type=int,
        default=default_max_warnings,
        help=f"maximum individual findings to print before the summary (default: {default_max_warnings})",
    )
    parser.add_argument(
        "--fail-on",
        choices=("none", "error", "warning"),
        default=default_fail_on,
        help=f"finding severity that returns non-zero: none, error, or warning (default: {default_fail_on})",
    )


def add_source_findings(root: etree._Element, path: Path) -> list[Finding]:
    findings: list[Finding] = []
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
            findings.append(
                Finding(
                    "warning",
                    "mixed-programlisting-output",
                    source_location(path, node),
                    "programlisting appears to contain SQL input plus output markers; "
                    "split SQL into <programlisting> and result text into <screen> "
                    f"({'; '.join(markers)})",
                )
            )

    for node in root.xpath("//d:screen", namespaces=NS):
        text = block_text(node)
        if SQL_START_RE.search(text) and ";" in text:
            findings.append(
                Finding(
                    "warning",
                    "screen-contains-sql",
                    source_location(path, node),
                    "screen block looks like it contains SQL input; use programlisting for input and screen for output",
                )
            )

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
                findings.append(
                    Finding(
                        "error",
                        "duplicate-refsection-title",
                        source_location(path, title_nodes[0]),
                        f"refentry has more than one {title!r} section; first one is at line {seen[title].sourceline}",
                    )
                )
            else:
                seen[title] = title_nodes[0]

    xpath = "//d:para[@role='availability' or @role='enhanced' or @role='changed']"
    for node in root.xpath(xpath, namespaces=NS):
        role = node.get("role") or "<missing>"
        if any(local_name(ancestor) in ADMONITIONS for ancestor in node.iterancestors()):
            findings.append(
                Finding(
                    "warning",
                    "metadata-role-in-admonition",
                    source_location(path, node),
                    f"metadata para role={role!r} is nested inside an admonition; prefer a plain role para in Description",
                )
            )
    return findings


def looks_like_sql_input(text: str) -> bool:
    normalized = " ".join(text.split()).upper()
    return normalized.startswith(SQL_STARTERS)


def find_label(parent: etree._Element, label_id: str) -> etree._Element | None:
    for child in parent.iter():
        if child is not parent and child.get("id") == label_id and "postgis-example-label" in class_tokens(child):
            return child
    return None


def copy_buttons(parent: etree._Element) -> list[etree._Element]:
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
        parent = pre.getparent()
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

    return findings


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


def refentry_id(refentry: etree._Element) -> str:
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
        for refname in refentry.findall(f"./{DB}refnamediv/{DB}refname"):
            name = text_content(refname)
            if not name:
                continue
            bare = name.split("(", 1)[0].strip()
            if IDENT_RE.match(bare):
                add_function(functions, bare.upper(), bare, entry_id, purpose)
            elif OPERATOR_RE.match(bare):
                add_operator(operators, bare, bare, entry_id, purpose)

    return {"functions": functions, "operators": operators}


def should_fail(findings: Iterable[Finding], fail_on: str) -> bool:
    if fail_on == "none":
        return False
    threshold = SEVERITY_ORDER[fail_on]
    return any(SEVERITY_ORDER[item.severity] >= threshold for item in findings)


def print_findings(findings: list[Finding], max_findings: int) -> None:
    for item in findings[: max(max_findings, 0)]:
        suffix = f": {item.message}" if item.message else ""
        print(f"{item.severity.upper()} {item.category}: {item.location}{suffix}")
    if max_findings >= 0 and len(findings) > max_findings:
        print(f"... {len(findings) - max_findings} additional findings suppressed by --max-warnings")


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
    for page in args.pages:
        path = Path(page)
        if not path.exists():
            findings.append(Finding("error", "html-block-label", f"{path}: file does not exist", ""))
            continue
        findings.extend(check_html_file(path, args.require_kinds))
    print_findings(findings, args.max_warnings)
    print_summary("DocBook block label checker", "no generated HTML block label issues found.", findings, args.fail_on)
    return 1 if should_fail(findings, args.fail_on) else 0


def command_ref_index(args: argparse.Namespace) -> int:
    index = build_index(args.xml)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(index, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n")
    print(
        f"wrote {args.output} with {len(index['functions'])} functions and "
        f"{sum(len(entries) for entries in index['operators'].values())} operator pages",
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
