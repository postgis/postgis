"""Small standard-library XML tree with parent and source-line indexes."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import xml.etree.ElementTree as ET
import xml.sax
from xml.sax.handler import ContentHandler, feature_external_ges, feature_external_pes, feature_namespaces


@dataclass
class IndexedTree:
    tree: ET.ElementTree
    parents: dict[ET.Element, ET.Element]
    lines: dict[ET.Element, int | None]

    def ancestors(self, node):
        while node in self.parents:
            node = self.parents[node]
            yield node

    def next_sibling(self, node):
        parent = self.parents.get(node)
        if parent is None:
            return None
        children = list(parent)
        index = children.index(node) + 1
        return children[index] if index < len(children) else None

    def line(self, node):
        return self.lines.get(node)


class _TreeBuilder(ContentHandler):
    def __init__(self):
        super().__init__()
        self.root = None
        self.stack = []
        self.parents = {}
        self.lines = {}
        self.locator = None

    def setDocumentLocator(self, locator):
        self.locator = locator

    @staticmethod
    def expanded_name(name):
        uri, local = name
        return f"{{{uri}}}{local}" if uri else local

    def startElementNS(self, name, _qualified_name, attributes):
        element = ET.Element(
            self.expanded_name(name),
            {self.expanded_name(key): value for key, value in attributes.items()},
        )
        self.lines[element] = self.locator.getLineNumber() if self.locator else None
        if self.stack:
            self.stack[-1].append(element)
            self.parents[element] = self.stack[-1]
        else:
            self.root = element
        self.stack.append(element)

    def endElementNS(self, _name, _qualified_name):
        self.stack.pop()

    def characters(self, content):
        if not self.stack:
            return
        current = self.stack[-1]
        if len(current):
            current[-1].tail = (current[-1].tail or "") + content
        else:
            current.text = (current.text or "") + content


def parse(path):
    parser = xml.sax.make_parser()
    parser.setFeature(feature_namespaces, True)
    for feature in (feature_external_ges, feature_external_pes):
        try:
            parser.setFeature(feature, False)
        except (xml.sax.SAXNotRecognizedException, xml.sax.SAXNotSupportedException):
            pass
    builder = _TreeBuilder()
    parser.setContentHandler(builder)
    parser.parse(str(Path(path)))
    return IndexedTree(ET.ElementTree(builder.root), builder.parents, builder.lines)
