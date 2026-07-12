#!/usr/bin/env python3

import argparse
import base64
from concurrent.futures import ThreadPoolExecutor
import html
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from xml_tree import parse as parse_xml


DOCBOOK_NS = "http://docbook.org/ns/docbook"
XML_NS = "http://www.w3.org/XML/1998/namespace"
FORCE_ROLE = "example-test"
EXTERNAL_STATE_ROLE = "requires-external-state"
ENVIRONMENT_CHECKS = (
    {
        "label": "PROJ grid au_icsm_GDA94_GDA2020_conformal_and_distortion.tif",
        "query": """SELECT ST_AsText(ST_TransformPipeline(
    'SRID=4939;POINT(143.0 -37.0)'::geometry,
    'urn:ogc:def:coordinateOperation:EPSG::8447'
))""",
        "expected": [["POINT(143.0000063280214 -36.999986718287545)"]],
        "hint": "Install the PROJ data package containing au_icsm_GDA94_GDA2020_conformal_and_distortion.tif, or fetch it with projsync so the PostgreSQL/PostGIS PROJ search path can find it.",
    },
)


class QueryRows(list):
    def __init__(self, rows, headers=None, types=None, visuals=None):
        super().__init__(rows)
        self.headers = headers or []
        self.types = types or []
        self.visuals = visuals or []
WKT_TYPES = (
    "POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|"
    "GEOMETRYCOLLECTION|CIRCULARSTRING|COMPOUNDCURVE|CURVEPOLYGON|"
    "MULTICURVE|MULTISURFACE|TIN|TRIANGLE|POLYHEDRALSURFACE"
    "|NURBSCURVE"
)
WKT_START_RE = re.compile(rf"(?:SRID\s*=\s*\d+\s*;\s*)?(?:{WKT_TYPES})(?:\s+(?:ZM|Z|M))?\s*\(", re.I)
NUMBER_PATTERN = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
MAKE_ENVELOPE_RE = re.compile(
    rf"\bST_MakeEnvelope\s*\(\s*({NUMBER_PATTERN})\s*,\s*({NUMBER_PATTERN})\s*,\s*"
    rf"({NUMBER_PATTERN})\s*,\s*({NUMBER_PATTERN})(?:\s*,\s*(-?\d+))?\s*\)",
    re.I,
)
POINT_CONSTRUCTOR_RE = re.compile(
    rf"\bST_(?:Make)?Point\s*\(\s*({NUMBER_PATTERN})\s*,\s*({NUMBER_PATTERN})\s*\)",
    re.I,
)
VISUAL_ROLE = "visual-primary"
VISUAL_SKIP_ROLE = "visual-skip"
SVG_PALETTES = {
    "Code": ("#2878b8", "#59a4d8", "#0f5f9c"),
    "Output": ("#a62c2b", "#d95f3d", "#ef8a47"),
}
POINT_TYPES = {"POINT", "MULTIPOINT"}
MAX_VISUAL_GEOMETRIES = 16
MAX_VISUAL_WKT_BYTES = 100000
NONVISUAL_SINGLE_INPUT_RE = re.compile(
    r"\b(?:ST_(?:AS(?:TEXT|EWKT|BINARY|EWKB|HEXEWKB|GML|KML|GEOJSON|X3D|"
    r"MARC21|GEOBUF|TWKB|ENCODEDPOLYLINE|LATLONTEXT)|GEOHASH|SRID|GEOMETRYTYPE|"
    r"COORDDIM|NDIMS|ZMFLAG|MEMSIZE|NUMGEOMETRIES|NUMPOINTS|NPOINTS|NUMRINGS|"
    r"NUMPATCHES|X|Y|Z|M)|CG_3DAREA)\s*\(",
    re.I,
)
DECIMAL_RE = re.compile(r"(?<![A-Za-z_])-?(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?")
VERSION_FUNCTION_RE = re.compile(
    r"\b(?:"
    r"PostGIS_Full_Version|postgis_full_version|PostGIS_GEOS_Version|"
    r"PostGIS_GEOS_Compiled_Version|PostGIS_LibXML_Version|"
    r"PostGIS_LibJSON_Version|PostGIS_PROJ_Version|"
    r"PostGIS_PROJ_Compiled_Version|PostGIS_Wagyu_Version|"
    r"PostGIS_Lib_Build_Date|PostGIS_Lib_Version|"
    r"PostGIS_Scripts_Build_Date|PostGIS_Scripts_Installed|"
    r"PostGIS_Scripts_Released|PostGIS_Version"
    r")\s*\(",
    re.I,
)
CATALOG_QUERY_RE = re.compile(r"\bpg_available_extensions\b", re.I)


class ExampleTester:
    def __init__(self, xml_file):
        self.index = parse_xml(xml_file)
        self.doc = self.index.tree

    def node_text(self, node):
        return "".join(node.itertext()).replace("\r\n", "\n").replace("\r", "\n")

    def has_role(self, node, role):
        return role in node.get("role", "").split()

    def source_label(self, node):
        refentry = next(
            (ancestor for ancestor in self.index.ancestors(node) if ancestor.tag == f"{{{DOCBOOK_NS}}}refentry"),
            None,
        )
        line = self.index.line(node) or "unknown"
        refentry_id = refentry.get(f"{{{XML_NS}}}id") if refentry is not None else None
        label = refentry_id or f"line_{line}"
        return f"{label}:{line}"

    def obvious_skip_reason(self, text):
        if re.search(r"SELECT\s+'<\?xml", text, re.I):
            return "document-output"
        if re.search(
            r"\b(sometable|gps_points|boston_polys|trajectories|fe_edges|"
            r"mytable|myrasters|mytestraster|dummy_rast|Projected\.tif|/home/raster|"
            r"towns|va2005|bc_roads|o_4_boston|"
            r"bc_municipality|nyc_|ma\.|features\.|aerials\.|"
            r"ma_topo|topo_boston_test|city_data|tt|"
            r"tiger\.|loader_|raster2pgsql|shp2pgsql|psql\b|wget\b|curl\b|"
            r"write_file|topology\.topologysummary|topology\.createtopology|"
            r"topology\.st_inittopogeo)\b",
            text,
            re.I,
        ):
            return "external-context"
        if re.search(
            r"\b(?:DropGeometryColumn|DropGeometryTable|Find_SRID|"
            r"ST_MinimumSpanningTree|PostGIS_Extensions_Upgrade|"
            r"PostGIS_GDAL_Version|PostGIS_Liblwgeom_Version|"
            r"PostGIS_Raster_Lib_Build_Date|PostGIS_Raster_Lib_Version|"
            r"PostGIS_Raster_Scripts_Installed|ST_GDALDrivers|ST_FromGDALRaster"
            r")\s*\(",
            text,
            re.I,
        ):
            return "external-context"
        if re.search(
            r"\b(?:CG_3DIntersection|CG_3DConvexHull|CG_AlphaShape|"
            r"CG_ApproxConvexPartition|CG_ExtrudeStraightSkeleton|"
            r"CG_GreeneApproxConvexPartition|CG_OptimalAlphaShape|"
            r"CG_OptimalConvexPartition|CG_YMonotonePartition|"
            r"ST_ClusterKMeans)\s*\(",
            text,
            re.I,
        ):
            return "unstable-output"
        if (
            "..." in text
            or re.search(r"\.\.(?:[),]|\s*$)", text, re.M)
            or re.search(r"(?:AD INFINITUM|--ETC--)", text, re.I)
        ):
            return "placeholder-output"
        if re.search(r"\b(NOTICE|ERROR):", text):
            return "notice-or-error"
        return ""

    def looks_sql(self, text):
        return re.search(r"\b(SELECT|WITH|CREATE|INSERT|UPDATE|DELETE|ALTER|DROP)\b", text, re.I)

    def looks_query(self, text):
        return re.search(r"\b(SELECT|WITH)\b", text, re.I)

    def has_legacy_inband_expected(self, text):
        return (
            re.search(r"--\s*(result|output|wkt collect)\s*--", text, re.I)
            or re.search(r"^\s*\S.*\n\s*[-+]{3,}\s*\n", text, re.S | re.M)
            or re.search(r"\(\d+ rows?\)", text)
        )

    def following_screen(self, node):
        sibling = self.index.next_sibling(node)
        if sibling is not None and sibling.tag == f"{{{DOCBOOK_NS}}}screen":
            return sibling
        return None

    def visual_location(self, screen):
        refentry = next(
            (ancestor for ancestor in self.index.ancestors(screen)
             if ancestor.tag == f"{{{DOCBOOK_NS}}}refentry"),
            None,
        )
        if refentry is None:
            screens = [
                candidate for candidate in self.doc.getroot().iter(f"{{{DOCBOOK_NS}}}screen")
                if not any(
                    ancestor.tag == f"{{{DOCBOOK_NS}}}refentry"
                    for ancestor in self.index.ancestors(candidate)
                )
            ]
            return "", screens.index(screen) + 1
        screens = list(refentry.iter(f"{{{DOCBOOK_NS}}}screen"))
        return refentry.get(f"{{{XML_NS}}}id", ""), screens.index(screen) + 1

    def visual_id(self, screen):
        explicit_id = screen.get(f"{{{XML_NS}}}id")
        if explicit_id:
            return explicit_id
        refentry, ordinal = self.visual_location(screen)
        slug = re.sub(r"[^a-z0-9]+", "-", refentry.lower()).strip("-") or "manual"
        return f"visual-{slug}-{ordinal:02d}"

    def geometry_type(self, value):
        match = WKT_START_RE.search(value or "")
        if not match:
            return None
        prefix = re.sub(r"^\s*SRID\s*=\s*\d+\s*;\s*", "", value, flags=re.I)
        match = re.match(r"([A-Z]+)", prefix.strip(), re.I)
        return match.group(1).upper() if match else None

    def normalized_geometry(self, value):
        return re.sub(r"\s+", "", value or "").upper()

    def visual_candidate(self, query, expected, explicit=False):
        code = [candidate["wkt"] for candidate in self.code_geometry_candidates(query)]
        output = self.geometry_candidates(self.rows_to_string(expected))
        values = code + output
        if explicit:
            return {"kind": "explicit", "preferred": True}
        if not values:
            return None
        if len(values) > MAX_VISUAL_GEOMETRIES or sum(len(value) for value in values) > MAX_VISUAL_WKT_BYTES:
            return None
        code_types = [self.geometry_type(value) for value in code]
        output_types = [self.geometry_type(value) for value in output]
        if len(code) == len(output) == 1 and self.normalized_geometry(code[0]) == self.normalized_geometry(output[0]):
            return None
        if output:
            if all(value in POINT_TYPES for value in output_types) and not (
                len(values) > 1 and any(value not in POINT_TYPES for value in code_types)
            ):
                return None
            return {"kind": "geometry-output", "preferred": True}
        if not code or all(value in POINT_TYPES for value in code_types):
            return None
        if len(code) == 1 and NONVISUAL_SINGLE_INPUT_RE.search(query):
            return None
        return {"kind": "input-context", "preferred": False}

    def parse_adjacent_example(self, query, screen):
        return self.clean_example(query, self.expected_rows_from_psql_lines(screen.split("\n")))

    def clean_example(self, query, expected_rows):
        query = query.strip()
        query = re.sub(r";\s*$", "", query)
        if not query or not expected_rows:
            return None, None
        return query, expected_rows

    def expected_rows_from_plain_lines(self, lines):
        rows = []
        for line in lines:
            line = line.strip()
            if not line or re.match(r"^\(\d+ rows?\)$", line):
                continue
            rows.append(self.split_psql_row(line) if re.search(r"│|\s+\|\s+", line) else [line])
        return rows

    def split_psql_row(self, line):
        if "│" in line:
            return [part.strip() for part in line.split("│")]
        parts = re.split(r"\s+\|\s+", line)
        if len(parts) > 1 and parts[-1].endswith("|"):
            parts[-1] = parts[-1][:-1]
            parts.append("")
        return [part.strip() for part in parts]

    def psql_delimiter_positions(self, header, separator):
        positions = [index for index, char in enumerate(header) if char in "|│"]
        if not positions:
            positions = [index for index, char in enumerate(separator) if char in "+┼"]
        return positions

    def split_psql_table_row(self, line, header, separator):
        positions = self.psql_delimiter_positions(header, separator)
        if not positions:
            return [line.strip()]

        values = []
        start = 0
        actual_delimiters = [index for index, char in enumerate(line) if char in "|│"]
        for position in positions:
            candidates = [candidate for candidate in actual_delimiters if candidate >= start]
            delimiter = min(candidates, key=lambda candidate: abs(candidate - position)) if candidates else position
            values.append(line[start:delimiter].strip())
            start = delimiter + 1
        values.append(line[start:].strip())
        return values

    def expected_rows_from_psql_lines(self, lines):
        clean = [line.replace("\r\n", "\n").replace("\r", "\n") for line in lines if re.search(r"\S", line)]
        if not clean:
            return []

        for i in range(len(clean) - 1):
            if not re.match(r"^\s*[-+─┼]+\s*$", clean[i + 1]):
                continue
            rows = []
            saw_row_count = False
            for line in clean[i + 2 :]:
                if re.match(r"^\s*$", line):
                    break
                if re.match(r"^\s*\(\d+ rows?\)\s*$", line):
                    saw_row_count = True
                    break
                if re.match(r"^\s*[-+─┼]+\s*$", line):
                    continue
                rows.append(self.split_psql_table_row(line, clean[i], clean[i + 1]))
            if rows or saw_row_count:
                return rows

        return self.expected_rows_from_plain_lines(clean)

    def psql_headers_from_lines(self, lines):
        clean = [line for line in lines if re.search(r"\S", line)]
        for i in range(len(clean) - 1):
            if re.match(r"^\s*[-+─┼]+\s*$", clean[i + 1]):
                return self.split_psql_table_row(clean[i], clean[i], clean[i + 1])
        return []

    def comparable_value(self, value):
        if value is None:
            return ""
        if value == "":
            return "null"
        if re.match(r"^null$", value, re.I):
            return "null"
        if re.match(rf"^(?:SRID=\d+;)?(?:{WKT_TYPES})\b", value, re.I):
            value = re.sub(rf"\b({WKT_TYPES})\s+\(", r"\1(", value, flags=re.I)
            value = re.sub(r",\s+", ",", value)
        value = DECIMAL_RE.sub(lambda match: f"{float(match.group(0)):.12g}", value)
        return value

    def canonical_ring(self, ring):
        points = ring.split(",")
        if len(points) > 1 and points[0] == points[-1]:
            points = points[:-1]
        if not points:
            return ring

        rotations = []
        for candidate in (points, list(reversed(points))):
            for index in range(len(candidate)):
                rotated = candidate[index:] + candidate[:index]
                rotations.append(rotated)
        points = min(rotations)
        return ",".join(points + [points[0]])

    def canonical_polygon_wkt(self, value):
        match = re.match(r"^(?:SRID=\d+;)?POLYGON\(\((.*)\)\)$", value, re.I | re.S)
        if not match:
            return value
        rings = match.group(1).split("),(")
        return "POLYGON((" + "),(".join(self.canonical_ring(ring) for ring in rings) + "))"

    def split_wkt_parts(self, value):
        parts = []
        depth = 0
        start = 0
        for index, char in enumerate(value):
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
            elif char == "," and depth == 0:
                parts.append(value[start:index])
                start = index + 1
        parts.append(value[start:])
        return [part.strip() for part in parts if part.strip()]

    def canonical_multipolygon_wkt(self, value):
        match = re.match(r"^(?:SRID=\d+;)?MULTIPOLYGON\((.*)\)$", value, re.I | re.S)
        if not match:
            return value

        polygons = []
        for polygon in self.split_wkt_parts(match.group(1)):
            polygon_match = re.match(r"^\(\((.*)\)\)$", polygon)
            if not polygon_match:
                return value
            rings = polygon_match.group(1).split("),(")
            polygons.append("((" + "),(".join(self.canonical_ring(ring) for ring in rings) + "))")

        return "MULTIPOLYGON(" + ",".join(sorted(polygons)) + ")"

    def canonical_geometrycollection_wkt(self, value):
        match = re.match(r"^(?:SRID=\d+;)?GEOMETRYCOLLECTION\((.*)\)$", value, re.I | re.S)
        if not match:
            return value

        geometries = [self.canonical_geometry_wkt(part) for part in self.split_wkt_parts(match.group(1))]
        return "GEOMETRYCOLLECTION(" + ",".join(sorted(geometries)) + ")"

    def canonical_surface_wkt(self, value):
        match = re.match(
            r"^(?:SRID=\d+;)?(POLYHEDRALSURFACE|TIN)\((.*)\)$",
            value, re.I | re.S,
        )
        if not match:
            return value
        faces = []
        for face in self.split_wkt_parts(match.group(2)):
            face_match = re.match(r"^\(\((.*)\)\)$", face, re.S)
            if not face_match:
                return value
            polygon = self.canonical_polygon_wkt(f"POLYGON(({face_match.group(1)}))")
            if not polygon.startswith("POLYGON"):
                return value
            faces.append(polygon[len("POLYGON"):])
        return f"{match.group(1).upper()}(" + ",".join(sorted(faces)) + ")"

    def canonical_geometry_wkt(self, value):
        polygon = self.canonical_polygon_wkt(value)
        if polygon != value:
            return polygon
        multipolygon = self.canonical_multipolygon_wkt(value)
        if multipolygon != value:
            return multipolygon
        surface = self.canonical_surface_wkt(value)
        if surface != value:
            return surface
        return self.canonical_geometrycollection_wkt(value)

    def values_equal(self, left, right):
        left_value = self.comparable_value(left)
        right_value = self.comparable_value(right)
        if left_value == right_value:
            return True
        if self.canonical_geometry_wkt(left_value) == self.canonical_geometry_wkt(right_value):
            return True
        if re.match(r"^-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?$", left_value) and re.match(
            r"^-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?$", right_value
        ):
            return abs(float(left_value) - float(right_value)) <= 1e-12
        return False

    def rows_equal(self, left, right):
        if len(left) == 1 and len(left[0]) == 1 and len(right) > 1:
            if all(len(row) == 1 for row in right):
                actual = left[0][0]
                if self.values_equal(actual, "".join(row[0] for row in right)):
                    return True
                if self.values_equal(actual, " ".join(row[0] for row in right)):
                    return True

        if len(left) != len(right):
            return False
        for left_row, right_row in zip(left, right):
            if len(left_row) != len(right_row):
                return False
            for left_value, right_value in zip(left_row, right_row):
                if not self.values_equal(left_value, right_value):
                    return False
        return True

    def version_rows_equal(self, actual, expected):
        if len(actual) != len(expected):
            return False
        for actual_row, expected_row in zip(actual, expected):
            if len(actual_row) != len(expected_row):
                return False
        return True

    def catalog_rows_equal(self, actual, expected):
        if not expected:
            return False
        if not actual:
            return True
        expected_width = len(expected[0])
        return all(len(row) == expected_width for row in actual)

    def rows_to_string(self, rows):
        return "\n".join(" | ".join(row) for row in rows)

    def parse_example_node(self, node):
        text = self.node_text(node)
        screen = self.following_screen(node)
        if screen is not None:
            return self.parse_adjacent_example(text, self.node_text(screen))
        return None, None

    def quoted_ranges(self, text):
        ranges = []
        index = 0
        while index < len(text):
            if text.startswith("--", index):
                newline = text.find("\n", index + 2)
                index = len(text) if newline < 0 else newline + 1
                continue
            if text.startswith("/*", index):
                closing = text.find("*/", index + 2)
                index = len(text) if closing < 0 else closing + 2
                continue
            if text[index] == "$":
                delimiter = re.match(r"\$(?:[A-Za-z_][A-Za-z0-9_]*)?\$", text[index:])
                if delimiter:
                    token = delimiter.group(0)
                    start = index + len(token)
                    closing = text.find(token, start)
                    if closing < 0:
                        break
                    ranges.append((start, closing))
                    index = closing + len(token)
                    continue
            if text[index] != "'":
                index += 1
                continue
            start = index + 1
            index += 1
            while index < len(text):
                if text[index] != "'":
                    index += 1
                    continue
                if index + 1 < len(text) and text[index + 1] == "'":
                    index += 2
                    continue
                ranges.append((start, index))
                index += 1
                break
        return ranges

    def closing_parenthesis(self, text, opening):
        depth = 0
        for index in range(opening, len(text)):
            if text[index] == "(":
                depth += 1
            elif text[index] == ")":
                depth -= 1
                if depth == 0:
                    return index
        return -1

    def geometry_candidate_matches(self, text, quoted_only=False):
        quoted = self.quoted_ranges(text) if quoted_only else []
        candidates = []
        cursor = 0
        while True:
            match = WKT_START_RE.search(text, cursor)
            if not match:
                break
            if match.start() and re.match(r"[A-Za-z0-9_]", text[match.start() - 1]):
                cursor = match.end()
                continue
            opening = text.find("(", match.start())
            closing = self.closing_parenthesis(text, opening)
            if closing < 0:
                raise RuntimeError(f"Unclosed geometry candidate near: {text[match.start():match.start() + 80]}")
            cursor = closing + 1
            if closing + 1 < len(text) and re.match(r"[A-Za-z0-9_]", text[closing + 1]):
                continue
            if quoted_only and not any(match.start() >= start and closing + 1 <= end for start, end in quoted):
                continue
            candidate = text[match.start():closing + 1].replace("''", "'")
            candidates.append({"start": match.start(), "end": closing + 1, "wkt": candidate})
        return candidates

    def geometry_candidates(self, text, quoted_only=False):
        return [match["wkt"] for match in self.geometry_candidate_matches(text, quoted_only)]

    def code_geometry_candidates(self, text):
        candidates = self.geometry_candidate_matches(text, quoted_only=True)
        for candidate in candidates:
            alias = re.match(
                r"\s*'\s*::\s*(?:geometry|geography)\s+(?:AS\s+)?([A-Za-z_][A-Za-z0-9_]*)",
                text[candidate["end"]:],
                re.I,
            )
            if alias:
                candidate["label"] = alias.group(1)
        for match in MAKE_ENVELOPE_RE.finditer(text):
            min_x, min_y, max_x, max_y = match.group(1, 2, 3, 4)
            wkt = (
                f"POLYGON(({min_x} {min_y},{max_x} {min_y},{max_x} {max_y},"
                f"{min_x} {max_y},{min_x} {min_y}))"
            )
            if match.group(5) is not None:
                wkt = f"SRID={match.group(5)};{wkt}"
            candidates.append({
                "start": match.start(), "end": match.end(), "wkt": wkt, "label": "Envelope",
            })
        for match in POINT_CONSTRUCTOR_RE.finditer(text):
            candidates.append({
                "start": match.start(),
                "end": match.end(),
                "wkt": f"POINT({match.group(1)} {match.group(2)})",
                "label": "Point",
            })
        return sorted(candidates, key=lambda candidate: candidate["start"])

    def query_output_headers(self, query):
        """Return explicit aliases from the outer SELECT list.

        Documented-only visual examples are not executed because their result
        ordering is intentionally unstable.  Their SQL aliases still carry the
        same input/output layer semantics as executed examples, so retain those
        aliases without introducing a SQL parser dependency.
        """
        masked = list(query)
        for start, end in self.quoted_ranges(query):
            masked[start:end] = " " * (end - start)
        masked_text = "".join(masked)
        masked_text = re.sub(r"--[^\n]*|/\*.*?\*/", lambda match: " " * len(match.group(0)), masked_text, flags=re.S)

        depth = 0
        select_start = None
        from_start = None
        index = 0
        while index < len(masked_text):
            char = masked_text[index]
            if char == "(":
                depth += 1
            elif char == ")":
                depth = max(0, depth - 1)
            elif depth == 0:
                token = re.match(r"(?i)(SELECT|FROM)\b", masked_text[index:])
                if token and (index == 0 or not re.match(r"[A-Za-z0-9_]", masked_text[index - 1])):
                    if token.group(1).upper() == "SELECT":
                        select_start = index + len(token.group(0))
                        from_start = None
                    elif select_start is not None:
                        from_start = index
                        break
                    index += len(token.group(0))
                    continue
            index += 1
        if select_start is None:
            return []
        select_end = from_start if from_start is not None else len(query)

        items = []
        item_start = select_start
        depth = 0
        for index in range(select_start, select_end):
            char = masked_text[index]
            if char == "(":
                depth += 1
            elif char == ")":
                depth = max(0, depth - 1)
            elif char == "," and depth == 0:
                items.append(query[item_start:index])
                item_start = index + 1
        items.append(query[item_start:select_end])

        headers = []
        for item in items:
            alias = re.search(r'(?i)\bAS\s+("(?:""|[^"])+"|[A-Za-z_][A-Za-z0-9_]*)\s*$', item)
            header = alias.group(1) if alias else ""
            if header.startswith('"'):
                header = header[1:-1].replace('""', '"')
            headers.append(header)
        return headers

    def query_is_auto_safe(self, query):
        if query is None:
            return False
        if not re.search(r"^\s*(?:--[^\n]*\n\s*)*(?:SELECT|WITH)\b", query, re.I):
            return False
        return query.count(";") <= 1

    def query_is_version_example(self, query):
        return query is not None and bool(VERSION_FUNCTION_RE.search(query))

    def query_is_catalog_example(self, query):
        return query is not None and bool(CATALOG_QUERY_RE.search(query))

    def expected_rows_are_auto_safe(self, expected):
        if not expected:
            return False
        for row in expected:
            for value in row:
                if re.search(r"\b(?:SELECT|WITH)\b", value, re.I):
                    return False
                if (
                    "..." in value
                    or re.search(r"\.\.(?:[),]|\s*$)", value)
                    or re.search(r"(?:AD INFINITUM|NOTICE:|ERROR:)", value, re.I)
                ):
                    return False
        return True

    def example_for_node(self, node):
        text = self.node_text(node)
        screen = self.following_screen(node)
        forced = self.has_role(node, FORCE_ROLE)
        explicit_visual = screen is not None and self.has_role(screen, VISUAL_ROLE)
        skip_reason = self.obvious_skip_reason(text)
        documented_only = bool(explicit_visual and skip_reason)
        if self.has_role(node, EXTERNAL_STATE_ROLE) and not forced and not documented_only:
            return None
        if not forced:
            if not self.looks_query(text):
                return None
            if skip_reason and not documented_only:
                return None

        query, expected = self.parse_example_node(node)
        valid = query is not None and expected is not None
        if not forced and (
            not valid or not self.query_is_auto_safe(query) or not self.expected_rows_are_auto_safe(expected)
        ):
            return None

        refentry = ""
        screen_ordinal = 0
        expected_headers = self.psql_headers_from_lines(self.node_text(screen).split("\n")) if screen is not None else []
        skip_visual = self.has_role(node, VISUAL_SKIP_ROLE) or (
            screen is not None and self.has_role(screen, VISUAL_SKIP_ROLE)
        )
        visual = None
        if screen is not None and not skip_visual:
            refentry, screen_ordinal = self.visual_location(screen)
            visual = self.visual_candidate(query, expected, explicit_visual)

        return {
            "label": self.source_label(node),
            "query": query,
            "expected": expected,
            "expected_headers": expected_headers,
            "valid": valid,
            "forced": forced,
            "version": self.query_is_version_example(query),
            "catalog": self.query_is_catalog_example(query),
            "documented_only": documented_only,
            "volatile": self.query_is_version_example(query) or self.query_is_catalog_example(query),
            "visual_id": self.visual_id(screen) if visual is not None else None,
            "visual_kind": visual["kind"] if visual is not None else None,
            "visual_preferred": visual["preferred"] if visual is not None else False,
            "visual_refentry": refentry,
            "visual_screen": screen_ordinal,
        }

    def examples(self):
        examples = []
        for node in self.doc.getroot().iter(f"{{{DOCBOOK_NS}}}programlisting"):
            example = self.example_for_node(node)
            if example is not None:
                examples.append(example)
        return examples

    def print_report(self):
        stats = {
            "programlisting_total": 0,
            "sql_like": 0,
            "select_or_with": 0,
            "legacy_inband_expected": 0,
            "external_context": 0,
            "placeholder_output": 0,
            "document_output": 0,
            "unstable_output": 0,
            "notice_or_error": 0,
            "cleanish_runnable_query": 0,
            "obvious_not_runnable_query": 0,
            "cleanish_legacy_inband_expected": 0,
            "obvious_bad_legacy_inband_expected": 0,
            "adjacent_programlisting_screen": 0,
            "cleanish_adjacent_expected": 0,
            "obvious_bad_adjacent_expected": 0,
            "requires_external_state": 0,
            "auto_example_tests": 0,
            "forced_example_tests": 0,
            "volatile_version_tests": 0,
        }

        for node in self.doc.getroot().iter(f"{{{DOCBOOK_NS}}}programlisting"):
            text = self.node_text(node)
            stats["programlisting_total"] += 1
            if self.has_role(node, EXTERNAL_STATE_ROLE):
                stats["requires_external_state"] += 1
            if self.looks_sql(text):
                stats["sql_like"] += 1
            if not self.looks_query(text):
                continue
            stats["select_or_with"] += 1

            reason = self.obvious_skip_reason(text)
            if reason == "external-context":
                stats["external_context"] += 1
            if reason == "placeholder-output":
                stats["placeholder_output"] += 1
            if reason == "document-output":
                stats["document_output"] += 1
            if reason == "unstable-output":
                stats["unstable_output"] += 1
            if reason == "notice-or-error":
                stats["notice_or_error"] += 1

            if reason:
                stats["obvious_not_runnable_query"] += 1
            else:
                stats["cleanish_runnable_query"] += 1

            if self.has_legacy_inband_expected(text):
                stats["legacy_inband_expected"] += 1
                if reason:
                    stats["obvious_bad_legacy_inband_expected"] += 1
                else:
                    stats["cleanish_legacy_inband_expected"] += 1

            screen = self.following_screen(node)
            if screen is not None:
                stats["adjacent_programlisting_screen"] += 1
                screen_text = self.node_text(screen)
                bad_screen = re.search(r"\.\.\.|NOTICE:|ERROR:", screen_text, re.I)
                if reason or bad_screen:
                    stats["obvious_bad_adjacent_expected"] += 1
                else:
                    stats["cleanish_adjacent_expected"] += 1

        examples = self.examples()
        stats["auto_example_tests"] = len([example for example in examples if not example["forced"]])
        stats["forced_example_tests"] = len([example for example in examples if example["forced"]])
        stats["volatile_version_tests"] = len([example for example in examples if example["volatile"]])
        visual_kinds = {
            kind: len([example for example in examples if example["visual_kind"] == kind])
            for kind in ("explicit", "geometry-output", "input-context")
        }
        stats["visual_examples"] = sum(visual_kinds.values())
        stats["visual_explicit"] = visual_kinds["explicit"]
        stats["visual_geometry_output"] = visual_kinds["geometry-output"]
        stats["visual_input_context"] = visual_kinds["input-context"]
        stats["example_tests"] = len(examples)
        stats["parseable_example_tests"] = len([example for example in examples if example["valid"]])
        invalid_tests = stats["example_tests"] - stats["parseable_example_tests"]
        skipped_query_candidates = stats["select_or_with"] - stats["example_tests"]
        if invalid_tests:
            status = f"ERROR: {invalid_tests} selected example test(s) could not be parsed."
        elif stats["example_tests"] == 0:
            status = "WARNING: no manual example tests were selected."
        else:
            status = "OK: selected manual example tests are parseable."

        print("Manual example test report")
        print(f"Status: {status}")
        print("")
        print("Selected tests:")
        print(f"  {stats['example_tests']} total examples selected for --run")
        print(f"  {stats['parseable_example_tests']} parseable examples")
        print(f"  {stats['volatile_version_tests']} version/catalog examples checked with relaxed comparison")
        print(f"  {stats['forced_example_tests']} forced by role=\"{FORCE_ROLE}\"")
        print(f"  {stats['visual_examples']} useful geometry figures selected at build time")
        print(f"    {stats['visual_geometry_output']} geometry outputs")
        print(f"    {stats['visual_input_context']} input-context figures with textual output")
        print(f"    {stats['visual_explicit']} explicit role=\"{VISUAL_ROLE}\" overrides")
        print("")
        print("Skipped by design:")
        print(f"  {stats['requires_external_state']} marked role=\"{EXTERNAL_STATE_ROLE}\"")
        print(f"  {skipped_query_candidates} SELECT/WITH program listings not selected")
        print(f"  {stats['external_context']} need tables, files, topology state, or other external context")
        print(f"  {stats['placeholder_output']} have placeholder output")
        print(f"  {stats['unstable_output']} have intentionally unstable output")
        print(f"  {stats['notice_or_error']} show NOTICE/ERROR output")
        print("")
        print("Document scan:")
        print(f"  {stats['programlisting_total']} programlisting blocks")
        print(f"  {stats['sql_like']} SQL-like blocks")
        print(f"  {stats['select_or_with']} SELECT/WITH query blocks")
        print(f"  {stats['adjacent_programlisting_screen']} query blocks with adjacent <screen> output")
        print("")
        print("Raw counters:")

        for key in sorted(stats):
            print(f"{key}={stats[key]}")

    def run_psql_query(self, database, query):
        cmd = ["psql", "-X", "-v", "ON_ERROR_STOP=1", "-P", "null=null", "-d", database, "-c", query]
        result = subprocess.run(
            cmd, capture_output=True, text=True, encoding="utf-8",
            env=self.psql_environment(),
        )
        if result.returncode != 0:
            raise RuntimeError(f"psql failed for query:\n{query}\n{result.stdout}{result.stderr}")
        lines = result.stdout.split("\n")
        return QueryRows(
            self.expected_rows_from_psql_lines(lines),
            self.psql_headers_from_lines(lines),
        )

    def psql_environment(self):
        env = os.environ.copy()
        options = env.get("PGOPTIONS", "").strip()
        png_driver = "-c postgis.gdal_enabled_drivers=PNG"
        if "postgis.gdal_enabled_drivers" not in options:
            options = f"{options} {png_driver}".strip()
        env["PGOPTIONS"] = options
        return env

    def query_output_types(self, database, query):
        query = re.sub(r";\s*$", "", query.strip())
        cmd = [
            "psql", "-X", "-A", "-t", "-F", "\t", "-P", "footer=off",
            "-v", "ON_ERROR_STOP=1", "-d", database,
        ]
        result = subprocess.run(
            cmd, input=f"{query}\n\\gdesc\n", capture_output=True, text=True,
            encoding="utf-8", env=self.psql_environment(),
        )
        if result.returncode != 0:
            raise RuntimeError(
                f"psql failed while describing a visual example:\n{query}\n"
                f"{result.stdout}{result.stderr}"
            )
        return [
            line.rsplit("\t", 1)[1].strip().lower()
            for line in result.stdout.splitlines()
            if "\t" in line
        ]

    def bytea_image(self, value):
        if not isinstance(value, str) or not value.startswith("\\x"):
            return None
        try:
            raw = bytes.fromhex(value[2:])
        except ValueError:
            return None
        if raw.startswith(b"\x89PNG\r\n\x1a\n") and len(raw) >= 24:
            width = int.from_bytes(raw[16:20], "big")
            height = int.from_bytes(raw[20:24], "big")
            return {
                "format": "PNG", "media_type": "image/png", "width": width,
                "height": height, "data": base64.b64encode(raw).decode("ascii"),
            }
        return None

    def prepare_visual_rows(self, actual, types):
        if not actual:
            return QueryRows(actual, getattr(actual, "headers", []), types)
        width = max((len(row) for row in actual), default=0)
        if len(types) != width:
            raise RuntimeError(
                f"Visual query returned {width} columns but psql described {len(types)}"
            )
        headers = getattr(actual, "headers", [])
        rows = []
        visuals = []
        for row_index, row in enumerate(actual, 1):
            row_label = next((
                value.strip() for column_index, value in enumerate(row)
                if types[column_index] != "bytea"
                and isinstance(value, str) and 0 < len(value.strip()) <= 80
                and not self.geometry_type(value)
            ), "")
            display_row = []
            for column_index, value in enumerate(row):
                image = self.bytea_image(value) if types[column_index] == "bytea" else None
                if image is None:
                    display_row.append(value)
                    continue
                header = headers[column_index] if column_index < len(headers) else ""
                source = "Code" if header.lower().startswith("input_") else "Output"
                label = header[len("input_"):] if source == "Code" else header
                row_labeled = False
                if not label or label.lower() in {"?column?", "image", "st_aspng"}:
                    label = row_label or source
                    row_labeled = bool(row_label)
                if len(actual) > 1 and not row_labeled:
                    label = f"{label} {row_index}"
                image.update({
                    "source": source, "label": label, "row": row_index,
                    "column": column_index + 1,
                })
                visuals.append(image)
                display_row.append(
                    f"{image['format']} image, {image['width']} x {image['height']} pixels"
                )
            rows.append(display_row)
        return QueryRows(rows, headers, types, visuals)

    def run_examples(self, database, keep_going=False, render_dir=None, visual_only=False, jobs=1):
        all_examples = self.examples()
        documented_visuals = [
            example for example in all_examples
            if example.get("documented_only") and example["visual_id"]
        ]
        examples = [example for example in all_examples if not example.get("documented_only")]
        if visual_only:
            examples = [example for example in examples if example["visual_id"]]
        else:
            self.check_environment(database)
        failures = []
        ran = 0
        visuals = []

        if visual_only and jobs > 1 and not keep_going:
            def run_visual(example):
                actual = self.run_one_example(database, example)
                return self.render_visual_example(database, example, actual)

            with ThreadPoolExecutor(max_workers=jobs) as pool:
                visuals = list(pool.map(run_visual, examples))
            visuals.extend(
                self.render_visual_example(
                    database, example,
                    QueryRows(example["expected"], example.get("expected_headers", []))
                )
                for example in documented_visuals
            )
            ran = len(examples)
            if render_dir:
                self.publish_visual_examples(render_dir, visuals)
            print(f"manual example tests passed: {ran}")
            if render_dir:
                print(f"visual examples rendered: {len(visuals)}")
            return

        for example in examples:
            try:
                actual = self.run_one_example(database, example)
                visual = self.render_visual_example(database, example, actual) if example["visual_id"] else None
            except RuntimeError as exc:
                if not keep_going:
                    raise
                print(exc, file=sys.stderr)
                failures.append(example["label"])
                continue
            ran += 1
            if visual:
                visuals.append(visual)

        if failures:
            raise RuntimeError(f"FAILED {len(failures)} example(s): {', '.join(failures)}")

        if render_dir:
            self.publish_visual_examples(render_dir, visuals)

        print(f"manual example tests passed: {ran}")
        if render_dir:
            print(f"visual examples rendered: {len(visuals)}")

    def run_one_example(self, database, example):
        if not example["valid"]:
            raise RuntimeError(f"Could not parse manual example test at {example['label']}")

        try:
            actual = self.run_psql_query(database, example["query"])
            if example.get("visual_kind") == "explicit":
                actual = self.prepare_visual_rows(
                    actual, self.query_output_types(database, example["query"])
                )
        except RuntimeError as exc:
            raise RuntimeError(f"Example test failed to run: {example['label']}\n{exc}") from exc

        if example["catalog"]:
            equal = self.catalog_rows_equal(actual, example["expected"])
        elif example["version"]:
            equal = self.version_rows_equal(actual, example["expected"])
        else:
            equal = self.rows_equal(actual, example["expected"])
        if not equal:
            raise RuntimeError(
                f"Example test failed: {example['label']}\n"
                f"Query:\n{example['query']}\n"
                f"Expected:\n{self.rows_to_string(example['expected'])}\n"
                f"Actual:\n{self.rows_to_string(actual)}"
            )
        return actual

    def run_psql_scalar(self, database, query):
        cmd = ["psql", "-X", "-A", "-t", "-v", "ON_ERROR_STOP=1", "-d", database, "-c", query]
        result = subprocess.run(
            cmd, capture_output=True, text=True, encoding="utf-8",
            env=self.psql_environment(),
        )
        if result.returncode != 0:
            raise RuntimeError(f"psql failed while rendering a visual example:\n{result.stdout}{result.stderr}")
        return result.stdout.strip()

    def visual_payload(self, database, layers):
        encoded = base64.b64encode(json.dumps(layers, separators=(",", ":")).encode("utf-8")).decode("ascii")
        query = f"""
WITH raw AS (
  SELECT ord, source, label, row_num, column_num, wkt
  FROM jsonb_to_recordset(
    convert_from(decode('{encoded}', 'base64'), 'UTF8')::jsonb
  ) AS item(ord integer, source text, label text, row_num integer, column_num integer, wkt text)
), parsed_raw AS (
  SELECT ord, source, label, row_num, column_num,
    ST_Force2D(ST_GeomFromEWKT(wkt)) AS geom
  FROM raw
), source_dimensions AS (
  SELECT source, max(ST_Dimension(geom)) AS dimension
  FROM parsed_raw
  GROUP BY source
), frame_policy AS (
  SELECT
    EXISTS (SELECT 1 FROM parsed_raw WHERE source = 'Code')
    AND EXISTS (SELECT 1 FROM parsed_raw WHERE source = 'Output')
    AND (
      (SELECT dimension FROM source_dimensions WHERE source = 'Code') =
        (SELECT dimension FROM source_dimensions WHERE source = 'Output')
      OR (SELECT count(DISTINCT ST_SRID(geom)) FROM parsed_raw) > 1
    ) AS separate_sources,
    (SELECT count(DISTINCT ST_SRID(geom)) FROM parsed_raw) = 1 AS shared_coordinate_space
  FROM parsed_raw
  LIMIT 1
), framed AS (
  SELECT ord, source, label, row_num, column_num, geom, shared_coordinate_space,
    CASE WHEN separate_sources THEN source ELSE 'Overlay' END AS frame
  FROM parsed_raw CROSS JOIN frame_policy
), frame_bounds_raw AS (
  SELECT frame, min(ord) AS first_ord,
    CASE WHEN bool_and(shared_coordinate_space)
      THEN (SELECT ST_Extent(all_geometries.geom) FROM framed AS all_geometries)
      ELSE ST_Extent(geom)
    END AS box,
    CASE WHEN count(DISTINCT ST_SRID(geom)) = 1 THEN max(ST_SRID(geom)) ELSE 0 END AS srid
  FROM framed
  GROUP BY frame
), frame_bounds AS (
  SELECT frame, first_ord, srid,
    ST_XMin(box) AS min_x, ST_YMin(box) AS min_y,
    ST_XMax(box) - ST_XMin(box) AS width,
    ST_YMax(box) - ST_YMin(box) AS height
  FROM frame_bounds_raw
  WHERE box IS NOT NULL
), parts AS (
  SELECT ord, source, label, row_num, column_num, frame,
    ST_NPoints(geom) AS total_points,
    GeometryType(geom) AS root_type,
    (ST_Dump(geom)).geom AS geom
  FROM framed
), translated_parts AS (
  SELECT ord, source, label, row_num, column_num, parts.frame, total_points, root_type,
    ST_Translate(geom, -min_x, -min_y) AS geom
  FROM parts JOIN frame_bounds USING (frame)
)
SELECT json_build_object(
  'frames', (
    SELECT json_agg(json_build_object(
      'id', frame,
      'label', CASE WHEN srid > 0 THEN frame || ' (SRID ' || srid || ')' ELSE frame END,
      'bounds', json_build_array(0, 0, width, height)
    ) ORDER BY first_ord)
    FROM frame_bounds
  ),
  'parts', (
    SELECT json_agg(json_build_object(
      'ord', ord, 'source', source, 'label', label,
      'row', row_num, 'column', column_num, 'frame', frame,
      'root_type', root_type,
      'type', GeometryType(geom), 'svg', ST_AsSVG(geom, 0, 12),
      'dimension', ST_Dimension(geom), 'srid', ST_SRID(geom),
      'x', CASE WHEN GeometryType(geom) = 'POINT' THEN ST_X(geom) END,
      'y', CASE WHEN GeometryType(geom) = 'POINT' THEN -ST_Y(geom) END,
      'closed', CASE
        WHEN GeometryType(geom) IN ('LINESTRING', 'CIRCULARSTRING', 'COMPOUNDCURVE')
          THEN ST_IsClosed(geom)
      END,
      'vertices', CASE
        WHEN root_type IN ('LINESTRING', 'POLYGON', 'TRIANGLE')
          AND total_points <= 64
        THEN (
          SELECT json_agg(json_build_array(ST_X(vertex.geom), -ST_Y(vertex.geom)))
          FROM ST_DumpPoints(geom) AS vertex
        )
      END
    ) ORDER BY ord)
    FROM translated_parts
  )
)::text
"""
        payload = self.run_psql_scalar(database, query)
        if not payload:
            raise RuntimeError("PostGIS returned no visual-example payload")
        return json.loads(payload)

    def visual_image_svg(self, visual_id, images):
        count = len(images)
        columns = 1 if count == 1 else min(2, count)
        rows = math.ceil(count / columns)
        gap = 14
        outer = 20
        label_height = 24 if count > 1 else 0
        cell_width = (560 - gap * (columns - 1)) / columns
        cell_height = 520 if count == 1 else 320
        svg_height = outer + rows * (label_height + cell_height) + (rows - 1) * gap + 44
        panels = []
        legends = []
        source_indexes = {"Code": 0, "Output": 0}
        for index, image in enumerate(images):
            column = index % columns
            row = index // columns
            left = outer + column * (cell_width + gap)
            top = outer + row * (label_height + cell_height + gap)
            image_top = top + label_height
            available_scale = min(cell_width / image["width"], cell_height / image["height"])
            scale = max(1, math.floor(available_scale)) if available_scale >= 1 else available_scale
            width = image["width"] * scale
            height = image["height"] * scale
            x = left + (cell_width - width) / 2
            y = image_top + (cell_height - height) / 2
            source_indexes[image["source"]] += 1
            layer_id = (
                f"{visual_id}-{image['source'].lower()}-"
                f"{source_indexes[image['source']]}"
            )
            label = html.escape(image["label"])
            if count > 1:
                panels.append(
                    f'<text class="frame-label" x="{left + cell_width / 2:.12g}" '
                    f'y="{top + 16:.12g}" text-anchor="middle">{label}</text>'
                )
            panels.append(
                f'<rect class="image-background" x="{x:.12g}" y="{y:.12g}" '
                f'width="{width:.12g}" height="{height:.12g}" fill="white"/>'
                f'<g class="geometry-layer image-layer" data-postgis-geometry-id="{layer_id}">'
                f'<image x="{x:.12g}" y="{y:.12g}" width="{width:.12g}" '
                f'height="{height:.12g}" preserveAspectRatio="none" '
                'style="image-rendering:crisp-edges;image-rendering:pixelated" '
                f'href="data:{image["media_type"]};base64,{image["data"]}"/>'
                '</g>'
            )
            if count == 1:
                legends.append(
                    f'<g class="legend-row" data-postgis-geometry-id="{layer_id}" '
                    f'transform="translate(20 {svg_height - 26})">'
                    '<rect width="11" height="11" rx="2" fill="#a62c2b"/>'
                    f'<text x="16" y="10">{label}</text></g>'
                )
        safe_id = html.escape(visual_id, quote=True)
        return (
            '<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" width="600" height="{svg_height}" '
            f'viewBox="0 0 600 {svg_height}" role="img" aria-labelledby="{safe_id}-title">\n'
            f'<title id="{safe_id}-title">Rendered SQL image output for {safe_id}</title>\n'
            '<style>.frame-label{font-family:sans-serif;font-size:12px;font-weight:600;fill:#344}'
            '.geometry-layer{opacity:1;transition:opacity 90ms ease}'
            '.image-layer image{image-rendering:crisp-edges;image-rendering:pixelated}'
            '.legend-row text{font-family:sans-serif;font-size:12px;fill:#344}'
            'svg:has(.geometry-layer.active) .geometry-layer:not(.active){opacity:.18}'
            '.geometry-layer.active{filter:brightness(.72)}</style>\n'
            + ''.join(panels) + '\n' + ''.join(legends) + '\n</svg>\n'
        )

    def render_image_visual_example(self, example, actual):
        layers = []
        source_indexes = {"Code": 0, "Output": 0}
        for image in actual.visuals:
            source = image["source"]
            source_indexes[source] += 1
            layers.append({
                "id": f"{example['visual_id']}-{source.lower()}-{source_indexes[source]}",
                "source": source, "label": image["label"], "row": image["row"],
                "column": image["column"], "media_type": image["media_type"],
                "width": image["width"], "height": image["height"], "frame": image["label"],
            })
        return {
            "id": example["visual_id"], "source": example["label"],
            "refentry": example["visual_refentry"], "screen": example["visual_screen"],
            "preferred": example["visual_preferred"], "kind": "image-output",
            "output_omitted": False, "layers": layers,
            "svg": self.visual_image_svg(example["visual_id"], actual.visuals),
        }

    def render_visual_example(self, database, example, actual):
        if getattr(actual, "visuals", None):
            return self.render_image_visual_example(example, actual)
        code = self.code_geometry_candidates(example["query"])
        output = []
        headers = getattr(actual, "headers", []) or self.query_output_headers(example["query"])
        for row_index, row in enumerate(actual, 1):
            for column_index, value in enumerate(row):
                matches = self.geometry_candidates(value)
                for match_index, wkt in enumerate(matches, 1):
                    label = headers[column_index] if column_index < len(headers) else ""
                    if not label or label.lower() in {"?column?", "st_astext", "st_asewkt"}:
                        label = "Output"
                    source = "Code" if label.lower().startswith("input_") else "Output"
                    if source == "Code":
                        label = label[len("input_"):]
                    if len(actual) > 1:
                        label = f"{label} {row_index}"
                    if len(matches) > 1:
                        label = f"{label}.{match_index}"
                    output.append({
                        "wkt": wkt, "label": label, "source": source,
                        "row_num": row_index, "column_num": column_index + 1,
                    })
        if any(candidate["source"] == "Code" for candidate in output):
            # Explicit input_* result columns define the authored input layers.
            # Do not repeat geometry literals and constructor arguments inferred
            # from the query, but retain them for ordinary named output columns.
            code = []
        layers = []
        code_geometry_count = sum("label" not in candidate for candidate in code)
        for index, candidate in enumerate(code, 1):
            if candidate.get("label"):
                label = candidate["label"]
            elif code_geometry_count == 1:
                label = "Code"
            else:
                code_index = sum("label" not in previous for previous in code[:index])
                label = f"Code {code_index}"
            layers.append({
                "ord": len(layers) + 1,
                "source": "Code",
                "label": label,
                "row_num": None,
                "column_num": None,
                "wkt": candidate["wkt"],
            })
        for source in ("Code", "Output"):
            values = [candidate for candidate in output if candidate["source"] == source]
            for index, candidate in enumerate(values, 1):
                meaningful_label = candidate["label"] != source and (len(output) > 1 or bool(code))
                layers.append({
                    "ord": len(layers) + 1,
                    "source": source,
                    "label": candidate["label"] if meaningful_label else source if len(values) == 1 else (
                        candidate["label"] if candidate["label"] != source else f"{source} {index}"
                    ),
                    "row_num": candidate["row_num"],
                    "column_num": candidate["column_num"],
                    "wkt": candidate["wkt"],
                })
        if not layers:
            raise RuntimeError(f"Visual example {example['visual_id']} has no geometry input or output")
        output_omitted = False
        try:
            payload = self.visual_payload(database, layers)
        except RuntimeError as exc:
            parse_failure = re.search(
                r"geometry (?:contains non-closed rings|requires more points)|parse error",
                str(exc),
                re.I,
            )
            if not output or not code or not parse_failure:
                raise
            layers = [layer for layer in layers if layer["source"] == "Code"]
            payload = self.visual_payload(database, layers)
            output_omitted = True
        rendered_layers = []
        layer_source_indexes = {"Code": 0, "Output": 0}
        parts_by_ord = {}
        for part in payload.get("parts") or []:
            parts_by_ord.setdefault(part["ord"], []).append(part)
        for ordinal, parts in sorted(parts_by_ord.items()):
            first = parts[0]
            source_index = layer_source_indexes[first["source"]]
            layer_source_indexes[first["source"]] += 1
            closed_values = [part.get("closed") for part in parts if part.get("closed") is not None]
            rendered_layers.append({
                "id": f"{example['visual_id']}-{first['source'].lower()}-{source_index + 1}",
                "source": first["source"],
                "label": first["label"],
                "row": first.get("row"),
                "column": first.get("column"),
                "dimension": first.get("dimension"),
                "closed": all(closed_values) if closed_values else None,
                "srid": first.get("srid"),
                "frame": first.get("frame", "Overlay"),
            })
        return {
            "id": example["visual_id"],
            "source": example["label"],
            "refentry": example["visual_refentry"],
            "screen": example["visual_screen"],
            "preferred": example["visual_preferred"] and not output_omitted,
            "kind": "input-context-fallback" if output_omitted else example["visual_kind"],
            "output_omitted": output_omitted,
            "layers": rendered_layers,
            "svg": self.visual_svg(example["visual_id"], payload),
        }

    def grid_step(self, span, target_lines=8):
        if span <= 0:
            return 1
        rough = span / target_lines
        power = 10 ** math.floor(math.log10(rough))
        normalized = rough / power
        nice = 1 if normalized <= 1 else 2 if normalized <= 2 else 5 if normalized <= 5 else 10
        return nice * power

    def visual_svg(self, visual_id, payload):
        frames = payload.get("frames") or [{
            "id": "Overlay",
            "bounds": payload["bounds"],
        }]
        frame_count = len(frames)
        panel_gap = 20 if frame_count > 1 else 0
        panel_width = (560 - panel_gap * (frame_count - 1)) / frame_count
        plot_top = 30 if frame_count > 1 else 12
        plot_bottom = 322
        plot_height = plot_bottom - plot_top
        layouts = {}
        backgrounds = []
        frame_labels = []
        grid = []
        for frame_index, frame in enumerate(frames):
            frame_id = str(frame["id"])
            min_x, min_y, max_x, max_y = [float(value) for value in frame["bounds"]]
            width = max_x - min_x
            height = max_y - min_y
            fallback = max(width, height, 1.0)
            if width == 0:
                min_x -= fallback / 2
                max_x += fallback / 2
            if height == 0:
                min_y -= fallback / 2
                max_y += fallback / 2
            width = max_x - min_x
            height = max_y - min_y
            min_x -= width * 0.08
            max_x += width * 0.08
            min_y -= height * 0.08
            max_y += height * 0.08
            panel_left = 20 + frame_index * (panel_width + panel_gap)
            panel_right = panel_left + panel_width
            scale = min(panel_width / (max_x - min_x), plot_height / (max_y - min_y))
            used_width = (max_x - min_x) * scale
            used_height = (max_y - min_y) * scale
            left = panel_left + (panel_width - used_width) / 2
            top = plot_top + (plot_height - used_height) / 2
            translate_x = left - min_x * scale
            translate_y = top + max_y * scale
            layouts[frame_id] = {
                "scale": scale,
                "translate_x": translate_x,
                "translate_y": translate_y,
            }
            backgrounds.append(
                f'<rect class="plot-background" x="{panel_left:.12g}" y="{plot_top}" '
                f'width="{panel_width:.12g}" height="{plot_height}" fill="#fbfcfd"/>'
            )
            if frame_count > 1:
                frame_label = str(frame.get("label", frame_id))
                frame_labels.append(
                    f'<text class="frame-label" x="{(panel_left + panel_right) / 2:.12g}" y="20" '
                    f'text-anchor="middle">{html.escape(frame_label)}</text>'
                )

            step = self.grid_step(max(max_x - min_x, max_y - min_y))
            first_x = math.ceil(min_x / step) * step
            first_y = math.ceil(min_y / step) * step
            value = first_x
            for _ in range(32):
                if value > max_x + step * 1e-9:
                    break
                x = translate_x + value * scale
                grid.append(
                    f'<line x1="{x:.12g}" y1="{plot_top}" x2="{x:.12g}" y2="{plot_bottom}" '
                    'stroke="#dce2e7" stroke-width="1"/>'
                )
                next_value = value + step
                if next_value <= value:
                    break
                value = next_value
            value = first_y
            for _ in range(32):
                if value > max_y + step * 1e-9:
                    break
                y = translate_y - value * scale
                grid.append(
                    f'<line x1="{panel_left:.12g}" y1="{y:.12g}" '
                    f'x2="{panel_right:.12g}" y2="{y:.12g}" '
                    'stroke="#dce2e7" stroke-width="1"/>'
                )
                next_value = value + step
                if next_value <= value:
                    break
                value = next_value

        area_groups = []
        line_groups = []
        point_groups = []
        legend = []
        source_indexes = {"Code": 0, "Output": 0}
        parts_by_ord = {}
        for part in payload.get("parts") or []:
            parts_by_ord.setdefault(part["ord"], []).append(part)
        for ordinal, parts in sorted(parts_by_ord.items()):
            source = parts[0]["source"]
            frame_id = str(parts[0].get("frame", frames[0]["id"]))
            layout = layouts[frame_id]
            scale = layout["scale"]
            source_index = source_indexes[source]
            source_indexes[source] += 1
            color = SVG_PALETTES[source][source_index % len(SVG_PALETTES[source])]
            geometry_id = f"{visual_id}-{source.lower()}-{source_index + 1}"
            shapes = []
            root_type = parts[0].get("root_type", "").upper()
            multipart_areas = len(parts) > 1 and all(part["type"].upper() in {
                "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
                "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
            } for part in parts) and root_type != "POLYHEDRALSURFACE"
            for part_index, part in enumerate(parts):
                part_color = (
                    SVG_PALETTES[source][(source_index + part_index) % len(SVG_PALETTES[source])]
                    if multipart_areas else color
                )
                if part["type"].upper() == "POINT":
                    point_x = float(part["x"])
                    point_y = float(part["y"])
                    radius = (6.5 if source == "Code" else 4.5) / scale
                    shapes.append(
                        f'<path class="point" d="M {point_x - radius:.12g} {point_y:.12g} '
                        f'A {radius:.12g} {radius:.12g} 0 1 0 {point_x + radius:.12g} {point_y:.12g} '
                        f'A {radius:.12g} {radius:.12g} 0 1 0 {point_x - radius:.12g} {point_y:.12g} Z" '
                        f'fill="{part_color}" stroke="white" stroke-width="{1.2 / scale:.12g}"/>'
                    )
                else:
                    if not part.get("svg"):
                        raise RuntimeError(
                            f"PostGIS returned an empty SVG path for {visual_id} {part['type']}"
                        )
                    if re.search(r"(?:^|\s)[Aa]\s+-(?:\d|\.)|\b(?:nan|inf)\b", part["svg"], re.I):
                        raise RuntimeError(
                            f"PostGIS returned an invalid SVG path for {visual_id} {part['type']}: {part['svg']}"
                        )
                    svg_data = html.escape(part["svg"], quote=True)
                    dimension_class = "area" if part["type"].upper() in {
                        "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
                        "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
                    } else "line"
                    fill = color if dimension_class == "area" else "none"
                    stroke_pixels = (
                        5 if source == "Code" and dimension_class == "line"
                        else 1.8 if root_type == "POLYHEDRALSURFACE"
                        else 3
                    )
                    stroke_width = stroke_pixels / scale
                    shapes.append(
                        f'<path class="{dimension_class}" d="{svg_data}" '
                        f'stroke="{part_color}" fill="{part_color if dimension_class == "area" else fill}" '
                        f'fill-opacity="{0.24 if dimension_class == "area" else 0}" stroke-opacity="1" '
                        'fill-rule="evenodd" '
                        f'stroke-width="{stroke_width:.12g}" '
                        'stroke-linecap="round" stroke-linejoin="round"/>'
                    )
                    vertices = part.get("vertices") or []
                    if len(vertices) > 1 and vertices[0] == vertices[-1]:
                        vertices = vertices[:-1]
                    if dimension_class == "line" and not part.get("closed") and len(vertices) >= 2:
                        end_x, end_y = [float(value) for value in vertices[-1]]
                        previous_x, previous_y = [float(value) for value in vertices[-2]]
                        delta_x = end_x - previous_x
                        delta_y = end_y - previous_y
                        length = math.hypot(delta_x, delta_y)
                        if length > 0:
                            arrow_length = (10 if source == "Code" else 8) / scale
                            arrow_half_width = arrow_length * 0.45
                            unit_x = delta_x / length
                            unit_y = delta_y / length
                            base_x = end_x - unit_x * arrow_length
                            base_y = end_y - unit_y * arrow_length
                            perpendicular_x = -unit_y * arrow_half_width
                            perpendicular_y = unit_x * arrow_half_width
                            shapes.append(
                                f'<path class="direction-arrow" d="M {end_x:.12g} {end_y:.12g} '
                                f'L {base_x + perpendicular_x:.12g} {base_y + perpendicular_y:.12g} '
                                f'L {base_x - perpendicular_x:.12g} {base_y - perpendicular_y:.12g} Z" '
                                f'fill="{part_color}" stroke="white" stroke-width="{0.7 / scale:.12g}"/>'
                            )
                    for x, y in vertices:
                        shapes.append(
                            f'<circle class="vertex" cx="{float(x):.12g}" cy="{float(y):.12g}" '
                            f'r="{3 / scale:.12g}" fill="{part_color}" stroke="white" '
                            f'stroke-width="{0.8 / scale:.12g}"/>'
                        )
            group_svg = (
                f'<g class="geometry-layer" opacity="1" data-postgis-geometry-id="{geometry_id}" '
                f'data-postgis-frame="{html.escape(frame_id, quote=True)}" '
                f'transform="matrix({scale:.12g} 0 0 {scale:.12g} '
                f'{layout["translate_x"]:.12g} {layout["translate_y"]:.12g})">' +
                "".join(shapes) + "</g>"
            )
            if all(part["type"].upper() in POINT_TYPES for part in parts):
                point_groups.append(group_svg)
            elif all(part["type"].upper() in {
                "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
                "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
            } for part in parts):
                area_groups.append(group_svg)
            else:
                line_groups.append(group_svg)
            legend.append((geometry_id, color, parts[0]["label"]))

        legend_svg = []
        cursor_x = 20
        cursor_y = 354
        for geometry_id, color, label in legend:
            safe_label = html.escape(label)
            item_width = max(72, 34 + len(label) * 7)
            if cursor_x + item_width > 580 and cursor_x > 20:
                cursor_x = 20
                cursor_y += 22
            legend_svg.append(
                f'<g class="legend-row" data-postgis-geometry-id="{geometry_id}" transform="translate({cursor_x} {cursor_y})">'
                f'<rect width="11" height="11" rx="2" fill="{color}"/>'
                f'<text x="16" y="10" font-family="sans-serif" font-size="12" fill="#344">{safe_label}</text></g>'
            )
            cursor_x += item_width

        svg_height = cursor_y + 26

        safe_id = html.escape(visual_id, quote=True)
        return (
            '<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" width="600" height="{svg_height}" viewBox="0 0 600 {svg_height}" role="img" aria-labelledby="{safe_id}-title">\n'
            f'<title id="{safe_id}-title">Input and output geometries for {safe_id}</title>\n'
            '<style>.plot-grid line{stroke:#dce2e7;stroke-width:1}.frame-label{font-family:sans-serif;font-size:12px;font-weight:600;fill:#344}.geometry-layer{opacity:1;transition:opacity 90ms ease}.line,.area{stroke-linecap:round;stroke-linejoin:round;pointer-events:stroke}.point,.vertex{pointer-events:all}.legend-row{cursor:default}.legend-row text{font-family:sans-serif;font-size:12px;fill:#344}svg:has(.geometry-layer.active) .geometry-layer:not(.active){opacity:.18}.geometry-layer.active{filter:brightness(.72)}</style>\n'
            + "".join(backgrounds) + "\n" + "".join(frame_labels) + "\n"
            '<g class="plot-grid" aria-hidden="true">' + "".join(grid) + '</g>\n'
            + "".join(area_groups + line_groups + point_groups) + '\n' + "".join(legend_svg) + '\n</svg>\n'
        )

    def publish_visual_examples(self, render_dir, visuals):
        destination = Path(render_dir)
        destination.parent.mkdir(parents=True, exist_ok=True)
        temporary = Path(tempfile.mkdtemp(prefix=f".{destination.name}-", dir=destination.parent))
        try:
            manifest = []
            seen = set()
            for visual in visuals:
                if not visual["id"] or visual["id"] in seen:
                    raise RuntimeError(f"Duplicate or missing visual example id: {visual['id']!r}")
                seen.add(visual["id"])
                (temporary / f"{visual['id']}.svg").write_text(visual["svg"], encoding="utf-8")
                manifest.append({
                    "id": visual["id"],
                    "kind": visual["kind"],
                    "output_omitted": visual.get("output_omitted", False),
                    "preferred": visual["preferred"],
                    "refentry": visual["refentry"],
                    "screen": visual["screen"],
                    "source": visual["source"],
                    "layers": visual.get("layers", []),
                })
            (temporary / "manifest.json").write_text(
                json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
            )
            manifest_root = ET.Element("visual-examples")
            for item in manifest:
                visual_node = ET.SubElement(manifest_root, "visual", {
                    key: ("true" if value is True else "false" if value is False else str(value))
                    for key, value in item.items()
                    if key != "layers"
                })
                for layer in item["layers"]:
                    ET.SubElement(visual_node, "layer", {
                        key: ("true" if value is True else "false" if value is False else str(value))
                        for key, value in layer.items()
                        if value is not None
                    })
            ET.ElementTree(manifest_root).write(
                temporary / "manifest.xml", encoding="utf-8", xml_declaration=True
            )
            backup = destination.with_name(f".{destination.name}.old")
            if backup.exists():
                shutil.rmtree(backup)
            if destination.exists():
                destination.rename(backup)
            temporary.rename(destination)
            if backup.exists():
                shutil.rmtree(backup)
        except Exception:
            shutil.rmtree(temporary, ignore_errors=True)
            raise

    def check_environment(self, database):
        for check in ENVIRONMENT_CHECKS:
            try:
                actual = self.run_psql_query(database, check["query"])
            except RuntimeError as exc:
                raise RuntimeError(
                    f"Manual example test environment check failed: {check['label']}\n"
                    f"{check['hint']}\n{exc}"
                ) from exc

            if not self.rows_equal(actual, check["expected"]):
                raise RuntimeError(
                    f"Manual example test environment check failed: {check['label']}\n"
                    f"{check['hint']}\n"
                    f"Expected:\n{self.rows_to_string(check['expected'])}\n"
                    f"Actual:\n{self.rows_to_string(actual)}"
                )

        print(f"manual example test environment checks passed: {len(ENVIRONMENT_CHECKS)}")


def parse_source_example(body, screen_body=None):
    tester = ExampleTester.__new__(ExampleTester)
    if screen_body is not None:
        return tester.parse_adjacent_example(body, screen_body)
    return None, None


def parse_args():
    parser = argparse.ArgumentParser(
        description="Report or run manual example tests.",
        usage="%(prog)s --report|--run|--check-environment [--database DB] [--keep-going] [--render-dir DIR] <postgis-out.xml>",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--report", action="store_true")
    mode.add_argument("--run", action="store_true")
    mode.add_argument("--check-environment", action="store_true")
    parser.add_argument("--database")
    parser.add_argument("--keep-going", action="store_true", help="run all examples before failing")
    parser.add_argument("--render-dir", help="write selected build-time SVG assets after a successful --run")
    parser.add_argument("--visual-only", action="store_true", help="run only selected visual examples")
    parser.add_argument("--jobs", type=int, default=1, help="parallel workers for --visual-only (default: 1)")
    parser.add_argument("xml_file")
    return parser.parse_args()


def main():
    args = parse_args()

    try:
        tester = ExampleTester(args.xml_file)

        if args.report:
            tester.print_report()
        elif args.run:
            if not args.database:
                raise RuntimeError("--database is required with --run")
            tester.run_examples(
                args.database,
                keep_going=args.keep_going,
                render_dir=args.render_dir,
                visual_only=args.visual_only,
                jobs=max(1, args.jobs),
            )
        elif args.check_environment:
            if not args.database:
                raise RuntimeError("--database is required with --check-environment")
            tester.check_environment(args.database)
        return 0
    except Exception as exc:
        print(exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
