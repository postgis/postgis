#!/usr/bin/env python3

import argparse
import re
import subprocess
import sys

from lxml import etree


DB = {"db": "http://docbook.org/ns/docbook"}
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
        parser = etree.XMLParser(resolve_entities=False, no_network=True)
        self.doc = etree.parse(str(xml_file), parser)

    def node_text(self, node):
        return "".join(node.itertext()).replace("\r\n", "\n").replace("\r", "\n")

    def has_role(self, node, role):
        return role in node.get("role", "").split()

    def source_label(self, node):
        refentries = node.xpath("ancestor::db:refentry[1]", namespaces=DB)
        refentry_id = refentries[0].get(f"{{{XML_NS}}}id") if refentries else None
        label = refentry_id or f"line_{node.sourceline or 'unknown'}"
        return f"{label}:{node.sourceline or 'unknown'}"

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
        sibling = node.getnext()
        while sibling is not None and not isinstance(sibling.tag, str):
            sibling = sibling.getnext()
        if sibling is not None and etree.QName(sibling).localname == "screen":
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
        }

    def examples(self):
        examples = []
        for node in self.doc.xpath("//db:programlisting", namespaces=DB):
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

        for node in self.doc.xpath("//db:programlisting", namespaces=DB):
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

    def run_examples(self, database):
        self.check_environment(database)

        examples = self.examples()
        ran = 0

        for example in examples:
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
            ran += 1

        print(f"manual example tests passed: {ran}")

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
        usage="%(prog)s --report|--run|--check-environment [--database DB] <postgis-out.xml>",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--report", action="store_true")
    mode.add_argument("--run", action="store_true")
    mode.add_argument("--check-environment", action="store_true")
    parser.add_argument("--database")
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
            tester.run_examples(args.database)
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
