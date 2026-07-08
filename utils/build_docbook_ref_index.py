#!/usr/bin/env python3
"""Build a small function/operator index for PostGIS manual example links."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any
from xml.etree import ElementTree as ET

DB = "{http://docbook.org/ns/docbook}"
XML_ID = "{http://www.w3.org/XML/1998/namespace}id"
IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
OPERATOR_RE = re.compile(r"^[^A-Za-z0-9_\s]+$")


def text_content(node: ET.Element | None) -> str:
    if node is None:
        return ""
    return " ".join("".join(node.itertext()).split())


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
    tree = ET.parse(input_path)
    root = tree.getroot()
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


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: build_docbook_ref_index.py postgis-out.xml postgis-ref-index.json", file=sys.stderr)
        return 2
    input_path = Path(argv[0])
    output_path = Path(argv[1])
    index = build_index(input_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(index, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n")
    print(
        f"wrote {output_path} with {len(index['functions'])} functions and "
        f"{sum(len(entries) for entries in index['operators'].values())} operator pages",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
