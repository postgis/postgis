#!/usr/bin/env python3

import argparse
import base64
import html
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

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
WKT_TYPES = (
    "POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|"
    "GEOMETRYCOLLECTION|CIRCULARSTRING|COMPOUNDCURVE|CURVEPOLYGON|"
    "MULTICURVE|MULTISURFACE|TIN|TRIANGLE|POLYHEDRALSURFACE"
    "|NURBSCURVE"
)
WKT_START_RE = re.compile(rf"(?:SRID\s*=\s*\d+\s*;\s*)?(?:{WKT_TYPES})(?:\s+(?:ZM|Z|M))?\s*\(", re.I)
VISUAL_ROLE = "visual-primary"
SVG_PALETTES = {
    "Code": ("#2878b8", "#59a4d8", "#0f5f9c"),
    "Output": ("#a62c2b", "#d95f3d", "#ef8a47"),
}
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
            r"PostGIS_Raster_Scripts_Installed|ST_GDALDrivers|ST_FromGDALRaster|ST_AsPNG"
            r")\s*\(",
            text,
            re.I,
        ):
            return "external-context"
        if re.search(
            r"\b(?:CG_3DIntersection|CG_3DConvexHull|CG_AlphaShape|"
            r"CG_ApproxConvexPartition|CG_ExtrudeStraightSkeleton|"
            r"CG_GreeneApproxConvexPartition|CG_OptimalAlphaShape|"
            r"CG_OptimalConvexPartition|CG_YMonotonePartition)\s*\(",
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

    def canonical_geometry_wkt(self, value):
        polygon = self.canonical_polygon_wkt(value)
        if polygon != value:
            return polygon
        multipolygon = self.canonical_multipolygon_wkt(value)
        if multipolygon != value:
            return multipolygon
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

    def geometry_candidates(self, text, quoted_only=False):
        quoted = self.quoted_ranges(text) if quoted_only else []
        candidates = []
        cursor = 0
        while True:
            match = WKT_START_RE.search(text, cursor)
            if not match:
                break
            opening = text.find("(", match.start())
            closing = self.closing_parenthesis(text, opening)
            if closing < 0:
                raise RuntimeError(f"Unclosed geometry candidate near: {text[match.start():match.start() + 80]}")
            cursor = closing + 1
            if match.start() and re.match(r"[A-Za-z0-9_]", text[match.start() - 1]):
                continue
            if closing + 1 < len(text) and re.match(r"[A-Za-z0-9_]", text[closing + 1]):
                continue
            if quoted_only and not any(match.start() >= start and closing + 1 <= end for start, end in quoted):
                continue
            candidate = text[match.start():closing + 1]
            candidates.append(candidate.replace("''", "'"))
        return candidates

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
        if self.has_role(node, EXTERNAL_STATE_ROLE) and not forced:
            return None
        if not forced:
            if not self.looks_query(text):
                return None
            if self.obvious_skip_reason(text):
                return None

        query, expected = self.parse_example_node(node)
        valid = query is not None and expected is not None
        if not forced and (
            not valid or not self.query_is_auto_safe(query) or not self.expected_rows_are_auto_safe(expected)
        ):
            return None

        return {
            "label": self.source_label(node),
            "query": query,
            "expected": expected,
            "valid": valid,
            "forced": forced,
            "version": self.query_is_version_example(query),
            "catalog": self.query_is_catalog_example(query),
            "volatile": self.query_is_version_example(query) or self.query_is_catalog_example(query),
            "visual_id": (
                screen.get(f"{{{XML_NS}}}id")
                if screen is not None and self.has_role(screen, VISUAL_ROLE)
                else None
            ),
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
        result = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8")
        if result.returncode != 0:
            raise RuntimeError(f"psql failed for query:\n{query}\n{result.stdout}{result.stderr}")
        return self.expected_rows_from_psql_lines(result.stdout.split("\n"))

    def run_examples(self, database, keep_going=False, render_dir=None, visual_only=False):
        examples = self.examples()
        if visual_only:
            examples = [example for example in examples if example["visual_id"]]
        else:
            self.check_environment(database)
        failures = []
        ran = 0
        visuals = []

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
        result = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8")
        if result.returncode != 0:
            raise RuntimeError(f"psql failed while rendering a visual example:\n{result.stdout}{result.stderr}")
        return result.stdout.strip()

    def visual_payload(self, database, layers):
        encoded = base64.b64encode(json.dumps(layers, separators=(",", ":")).encode("utf-8")).decode("ascii")
        query = f"""
WITH raw AS (
  SELECT ord, source, label, wkt
  FROM jsonb_to_recordset(
    convert_from(decode('{encoded}', 'base64'), 'UTF8')::jsonb
  ) AS item(ord integer, source text, label text, wkt text)
), parsed AS (
  SELECT ord, source, label, ST_Force2D(ST_GeomFromEWKT(wkt)) AS geom
  FROM raw
), bounds AS (
  SELECT ST_Extent(geom) AS box FROM parsed
), parts AS (
  SELECT ord, source, label, (ST_Dump(geom)).geom AS geom
  FROM parsed
)
SELECT json_build_object(
  'bounds', json_build_array(ST_XMin(box), ST_YMin(box), ST_XMax(box), ST_YMax(box)),
  'parts', (
    SELECT json_agg(json_build_object(
      'ord', ord, 'source', source, 'label', label,
      'type', GeometryType(geom), 'svg', ST_AsSVG(geom, 0, 12),
      'x', CASE WHEN GeometryType(geom) = 'POINT' THEN ST_X(geom) END,
      'y', CASE WHEN GeometryType(geom) = 'POINT' THEN -ST_Y(geom) END
    ) ORDER BY ord)
    FROM parts
  )
)::text
FROM bounds
"""
        payload = self.run_psql_scalar(database, query)
        if not payload:
            raise RuntimeError("PostGIS returned no visual-example payload")
        return json.loads(payload)

    def render_visual_example(self, database, example, actual):
        code = self.geometry_candidates(example["query"], quoted_only=True)
        output = self.geometry_candidates(self.rows_to_string(actual))
        layers = []
        for source, values in (("Code", code), ("Output", output)):
            for index, wkt in enumerate(values, 1):
                layers.append({
                    "ord": len(layers) + 1,
                    "source": source,
                    "label": f"{source} {index}",
                    "wkt": wkt,
                })
        if not output:
            raise RuntimeError(f"Visual example {example['visual_id']} has no geometry output")
        payload = self.visual_payload(database, layers)
        return {
            "id": example["visual_id"],
            "source": example["label"],
            "svg": self.visual_svg(example["visual_id"], payload),
        }

    def visual_svg(self, visual_id, payload):
        min_x, min_y, max_x, max_y = [float(value) for value in payload["bounds"]]
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
        scale = min(560 / (max_x - min_x), 310 / (max_y - min_y))
        used_width = (max_x - min_x) * scale
        used_height = (max_y - min_y) * scale
        left = 20 + (560 - used_width) / 2
        top = 12 + (310 - used_height) / 2
        translate_x = left - min_x * scale
        translate_y = top + max_y * scale

        groups = []
        legend = []
        source_indexes = {"Code": 0, "Output": 0}
        parts_by_ord = {}
        for part in payload.get("parts") or []:
            parts_by_ord.setdefault(part["ord"], []).append(part)
        for ordinal, parts in sorted(parts_by_ord.items()):
            source = parts[0]["source"]
            source_index = source_indexes[source]
            source_indexes[source] += 1
            color = SVG_PALETTES[source][source_index % len(SVG_PALETTES[source])]
            geometry_id = f"{visual_id}-{source.lower()}-{source_index + 1}"
            shapes = []
            for part in parts:
                if part["type"].upper() == "POINT":
                    shapes.append(
                        f'<circle class="point" cx="{float(part["x"]):.12g}" '
                        f'cy="{float(part["y"]):.12g}" r="4" fill="{color}"/>'
                    )
                else:
                    svg_data = html.escape(part["svg"], quote=True)
                    dimension_class = "area" if part["type"].upper() in {
                        "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
                        "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
                    } else "line"
                    fill = color if dimension_class == "area" else "none"
                    shapes.append(
                        f'<path class="{dimension_class}" d="{svg_data}" '
                        f'stroke="{color}" fill="{fill}" fill-rule="evenodd"/>'
                    )
            groups.append(
                f'<g class="geometry-layer" data-postgis-geometry-id="{geometry_id}">' +
                "".join(shapes) + "</g>"
            )
            legend.append((geometry_id, color, parts[0]["label"]))

        legend_svg = []
        cursor = 20
        for geometry_id, color, label in legend:
            safe_label = html.escape(label)
            legend_svg.append(
                f'<g class="legend-row" data-postgis-geometry-id="{geometry_id}" transform="translate({cursor} 354)">'
                f'<rect width="11" height="11" rx="2" fill="{color}"/><text x="16" y="10">{safe_label}</text></g>'
            )
            cursor += 88

        safe_id = html.escape(visual_id, quote=True)
        return (
            '<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 600 380" role="img" aria-labelledby="{safe_id}-title">\n'
            f'<title id="{safe_id}-title">Input and output geometries for {safe_id}</title>\n'
            '<style>.geometry-layer{opacity:.82}.line,.area{stroke-width:2;vector-effect:non-scaling-stroke;stroke-linecap:round;stroke-linejoin:round}.area{fill-opacity:.18}.point{stroke:white;stroke-width:1.2;vector-effect:non-scaling-stroke}.legend-row text{font:12px sans-serif;fill:#344}.geometry-layer.active{opacity:1}.geometry-layer.active .line,.geometry-layer.active .area{stroke-width:3}</style>\n'
            f'<g transform="matrix({scale:.12g} 0 0 {scale:.12g} {translate_x:.12g} {translate_y:.12g})">'
            + "".join(groups) + '</g>\n' + "".join(legend_svg) + '\n</svg>\n'
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
                manifest.append({"id": visual["id"], "source": visual["source"]})
            (temporary / "manifest.json").write_text(
                json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
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
    parser.add_argument("--render-dir", help="write visual-primary SVG assets after a successful --run")
    parser.add_argument("--visual-only", action="store_true", help="run only visual-primary examples")
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
