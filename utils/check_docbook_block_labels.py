#!/usr/bin/env python3
"""Check generated manual HTML labels DocBook code and output blocks."""

from __future__ import annotations

import sys
from pathlib import Path
from xml.etree import ElementTree as ET

BLOCK_KINDS = {
    "programlisting": ("postgis-example-code", "code", "Code"),
    "screen": ("postgis-example-output", "output", "Output"),
}
FORBIDDEN_SCREEN_CLASSES = {"programlisting", "sql", "language-sql", "hljs", "highlight"}


def local_name(tag: str) -> str:
    if "}" in tag:
        return tag.rsplit("}", 1)[1]
    return tag


def class_tokens(node: ET.Element) -> set[str]:
    return set((node.get("class") or "").split())


def text_content(node: ET.Element) -> str:
    return "".join(node.itertext()).strip()


def element_id(node: ET.Element) -> str:
    return node.get("id") or "<no id>"


def find_label(parent: ET.Element, label_id: str) -> ET.Element | None:
    for child in list(parent):
        if child.get("id") == label_id and "postgis-example-label" in class_tokens(child):
            return child
    return None


def check_file(path: Path) -> list[str]:
    errors: list[str] = []
    tree = ET.parse(path)
    root = tree.getroot()
    parents = {child: parent for parent in root.iter() for child in list(parent)}
    counts = {"programlisting": 0, "screen": 0}

    for pre in root.iter():
        if local_name(pre.tag) != "pre":
            continue
        tokens = class_tokens(pre)
        block_class = next((name for name in BLOCK_KINDS if name in tokens), None)
        if block_class is None:
            continue

        counts[block_class] += 1
        wrapper_class, expected_data_block, expected_english_label = BLOCK_KINDS[block_class]
        parent = parents.get(pre)
        if parent is None or local_name(parent.tag) != "div":
            errors.append(f"{path}: {block_class} block {element_id(pre)} is not wrapped in a div")
            continue

        parent_tokens = class_tokens(parent)
        if "postgis-example-block" not in parent_tokens or wrapper_class not in parent_tokens:
            errors.append(
                f"{path}: {block_class} block {element_id(pre)} wrapper classes are {sorted(parent_tokens)}"
            )
        if parent.get("role") != "group":
            errors.append(f"{path}: {block_class} block {element_id(pre)} wrapper lacks role=group")
        if parent.get("data-postgis-block") != expected_data_block:
            errors.append(
                f"{path}: {block_class} block {element_id(pre)} wrapper data-postgis-block is "
                f"{parent.get('data-postgis-block')!r}, expected {expected_data_block!r}"
            )

        label_id = parent.get("aria-labelledby")
        if not label_id:
            errors.append(f"{path}: {block_class} block {element_id(pre)} wrapper lacks aria-labelledby")
            continue

        label = find_label(parent, label_id)
        if label is None:
            errors.append(f"{path}: {block_class} block {element_id(pre)} wrapper lacks visible label {label_id}")
            continue
        label_text = text_content(label)
        if not label_text:
            errors.append(f"{path}: {block_class} block {element_id(pre)} wrapper has an empty visible label")
        elif ("postgis-en" in str(path) or str(path).endswith("-en.html")) and label_text != expected_english_label:
            errors.append(
                f"{path}: {block_class} block {element_id(pre)} label is {label_text!r}, "
                f"expected {expected_english_label!r}"
            )

        if block_class == "screen":
            forbidden = tokens & FORBIDDEN_SCREEN_CLASSES
            if forbidden:
                errors.append(
                    f"{path}: screen/output block {element_id(pre)} has code/highlight class tokens {sorted(forbidden)}"
                )

    for block_class, count in counts.items():
        if count == 0:
            errors.append(f"{path}: no generated {block_class} blocks found")

    return errors


def main(argv: list[str]) -> int:
    if not argv:
        print("usage: check_docbook_block_labels.py GENERATED_PAGE.html ...", file=sys.stderr)
        return 2

    errors: list[str] = []
    for arg in argv:
        path = Path(arg)
        if not path.exists():
            errors.append(f"{path}: file does not exist")
            continue
        errors.extend(check_file(path))

    if errors:
        print("DocBook block label check failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
