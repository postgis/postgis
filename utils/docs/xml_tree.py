"""Small standard-library XML tree with parent and source-line indexes."""

from __future__ import annotations

import io
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


def _internal_subset_bounds(data):
    """Return the (start, end) byte offsets of the DOCTYPE's internal subset,
    the "[" ... "]" span right after the doctype name, or None if the document
    has no internal subset at all.

    A previous version of this guard located the DOCTYPE with a raw text
    search for the bytes "<!DOCTYPE", which also matched lookalike text
    protected inside a CDATA section (the X3D example in
    reference_output.xml) and had no way to tell a literal "]" or "]>" inside
    a quoted entity value or a DTD comment from the internal subset's real
    closing bracket. expat parses the doctypedecl grammar production for
    real: StartDoctypeDeclHandler only fires for a genuine prolog DOCTYPE, and
    its CurrentByteIndex lands exactly on the internal subset's opening "[";
    EndDoctypeDeclHandler's CurrentByteIndex lands exactly on the
    declaration's closing ">", with any nested brackets, quoted strings, or
    comments in between already correctly consumed by expat's own tokenizer.
    Nothing here resolves or fetches anything external: no
    ExternalEntityRefHandler is registered, and parameter-entity parsing is
    left at its default (off), so a %-parameter-entity reference in the
    internal subset is not expanded during this scan either. It does not need
    to be: the whole internal subset is removed below regardless of what it
    declares.
    """
    bounds = []

    def start_doctype_decl(_name, _system_id, _public_id, has_internal_subset):
        if has_internal_subset:
            bounds.append(parser.CurrentByteIndex)

    def end_doctype_decl():
        if bounds:
            bounds.append(parser.CurrentByteIndex)

    parser = xml.parsers.expat.ParserCreate()
    parser.StartDoctypeDeclHandler = start_doctype_decl
    parser.EndDoctypeDeclHandler = end_doctype_decl
    parser.Parse(data, True)
    return tuple(bounds) if len(bounds) == 2 else None


def _without_internal_subset(path):
    """Return the document bytes with any DOCTYPE internal subset removed.

    The internal subset is the only place a document can declare a general or
    parameter entity with literal replacement text. Removing it outright,
    rather than trying to predict which of its declared entities the document
    body goes on to reference, is what makes this guard complete instead of
    another partial pattern match: no entity is left declared for the real
    parse below to expand, whether the reference appears in element content
    or inside an attribute value (attribute-value normalization substitutes
    entities before startElement handlers ever see them, so it cannot be
    intercepted there), and no matter how the declaration or the reference is
    spelled, including a non-ASCII entity name or a general entity declared
    indirectly through a parameter entity. With no entity declared, expat
    itself rejects any "&name;" reference left in the body as an undefined
    entity, which is a plain well-formedness error, not a pattern this module
    has to anticipate.

    Our own generated postgis-out.xml carries a DOCTYPE whose internal subset
    declares dozens of SYSTEM entities plus a handful of literal ones
    (last_release_version and its neighbors): xmllint --noent has already
    substituted every reference to any of them by the time this module sees
    the file, so their declarations are dead text and dropping them is a
    no-op. The same is true of any other DOCTYPE whose internal subset the
    document does not actually depend on.
    """
    data = Path(path).read_bytes()
    bounds = _internal_subset_bounds(data)
    if bounds is None:
        return data
    start, end = bounds
    return data[:start] + data[end:]


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
    source = InputSource()
    source.setSystemId(str(Path(path)))
    source.setByteStream(io.BytesIO(_without_internal_subset(path)))
    parser.parse(source)
    return IndexedTree(ET.ElementTree(builder.root), builder.parents, builder.lines)
