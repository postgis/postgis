"""Small standard-library XML tree with parent and source-line indexes."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
import xml.etree.ElementTree as ET
import xml.parsers.expat
import xml.sax
from xml.sax.handler import ContentHandler, EntityResolver, feature_external_ges, feature_external_pes, feature_namespaces
from xml.sax.xmlreader import InputSource


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


_GENERAL_ENTITY_REFERENCE_RE = re.compile(rb"&([A-Za-z_][\w.-]*);")


def _internal_general_entity_names(path):
    """Return the names of literal-value ("internal") general entities the
    document's DOCTYPE declares, without resolving or fetching anything external.

    Our own generated postgis-out.xml carries a DOCTYPE whose internal subset
    declares dozens of SYSTEM entities; xmllint uses those only to assemble
    doc/*.xml into one file at build time, so by the time this module sees the
    output they are unreferenced text in the DOCTYPE. They are external, so
    ExternalEntityRefHandler is left unset here and they are never resolved or
    counted. Only an entity with a literal replacement value can inject
    attacker-controlled text or drive an entity-expansion blowup if the document
    goes on to reference it, so only those names are worth tracking.
    """
    names = set()

    def entity_decl(name, is_parameter_entity, value, _base, _system_id, _public_id, _notation_name):
        if value is not None and not is_parameter_entity:
            names.add(name)

    parser = xml.parsers.expat.ParserCreate()
    parser.EntityDeclHandler = entity_decl
    with Path(path).open("rb") as handle:
        parser.ParseFile(handle)
    return names


def _reject_internal_entity_references(path):
    entity_names = _internal_general_entity_names(path)
    if not entity_names:
        return
    data = Path(path).read_bytes()
    for match in _GENERAL_ENTITY_REFERENCE_RE.finditer(data):
        name = match.group(1).decode("ascii", "replace")
        if name in entity_names:
            raise xml.sax.SAXException(f"entity references are not supported: &{name};")


class _EmptyEntityResolver(EntityResolver):
    """Refuse to fetch an external entity's replacement content.

    feature_external_ges/feature_external_pes below already tell a conformant
    parser not to resolve external entities, but that support is best-effort
    (see the try/except immediately below). Overriding resolveEntity makes the
    refusal unconditional: any external SYSTEM/PUBLIC identifier, including the
    DocBook DTD itself, resolves to an empty document instead of a real fetch.
    """

    def resolveEntity(self, _public_id, _system_id):
        return InputSource()


def parse(path):
    _reject_internal_entity_references(path)
    parser = xml.sax.make_parser()
    parser.setFeature(feature_namespaces, True)
    for feature in (feature_external_ges, feature_external_pes):
        try:
            parser.setFeature(feature, False)
        except (xml.sax.SAXNotRecognizedException, xml.sax.SAXNotSupportedException):
            pass
    parser.setEntityResolver(_EmptyEntityResolver())
    builder = _TreeBuilder()
    parser.setContentHandler(builder)
    parser.parse(str(Path(path)))
    return IndexedTree(ET.ElementTree(builder.root), builder.parents, builder.lines)
