#!/usr/bin/env python3

import argparse
import re
import subprocess
import sys
from pathlib import Path

from lxml import etree


DB = {"db": "http://docbook.org/ns/docbook"}
XML_NS = "http://www.w3.org/XML/1998/namespace"
WKT_TYPES = (
    "POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|"
    "GEOMETRYCOLLECTION|CIRCULARSTRING|COMPOUNDCURVE|CURVEPOLYGON|"
    "MULTICURVE|MULTISURFACE|TIN|TRIANGLE|POLYHEDRALSURFACE"
)
DECIMAL_RE = re.compile(r"(?<![A-Za-z_])-?(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?")


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
        if re.search(r"^\s*\\\w+", text, re.M):
            return "psql-meta"
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
            r"PostGIS_Full_Version|postgis_full_version|PostGIS_GEOS_Version|"
            r"PostGIS_GEOS_Compiled_Version|PostGIS_LibXML_Version|"
            r"PostGIS_LibJSON_Version|PostGIS_PROJ_Version|"
            r"PostGIS_PROJ_Compiled_Version|PostGIS_Wagyu_Version|"
            r"PostGIS_GDAL_Version|PostGIS_Liblwgeom_Version|"
            r"PostGIS_Lib_Build_Date|PostGIS_Lib_Version|"
            r"PostGIS_Raster_Lib_Build_Date|PostGIS_Raster_Lib_Version|"
            r"PostGIS_Raster_Scripts_Installed|ST_GDALDrivers|ST_FromGDALRaster|ST_AsPNG|"
            r"PostGIS_Scripts_Build_Date|PostGIS_Scripts_Installed|"
            r"PostGIS_Scripts_Released|PostGIS_Version)\s*\(",
            text,
            re.I,
        ):
            return "external-context"
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

    def has_inband_expected(self, text):
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

    def parse_inband_example(self, text):
        lines = text.split("\n")

        for i, line in enumerate(lines):
            if re.search(r"--\s*(result|output|wkt collect)\s*--", line, re.I):
                query = "\n".join(lines[:i])
                expected = [line for line in lines[i + 1 :] if re.search(r"\S", line)]
                for j in range(len(expected) - 1):
                    if re.match(r"^\s*[-+─┼]+\s*$", expected[j + 1]):
                        return self.clean_example(query, self.expected_rows_from_psql_lines(expected))
                return self.clean_example(query, self.expected_rows_from_plain_lines(expected))

        for i in range(len(lines) - 1):
            if not re.match(r"^\s*[-+]{3,}\s*$", lines[i + 1]):
                continue
            if re.search(r";\s*$", lines[i]):
                query = "\n".join(lines[: i + 1])
                return self.clean_example(query, self.expected_rows_from_plain_lines(lines[i + 2 :]))
            query = "\n".join(lines[:i])
            return self.clean_example(query, self.expected_rows_from_psql_lines(lines[i:]))

        return None, None

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
        return [part.strip() for part in re.split(r"\s+\|\s+", line)]

    def expected_rows_from_psql_lines(self, lines):
        clean = [line.replace("\r\n", "\n").replace("\r", "\n") for line in lines if re.search(r"\S", line)]
        if not clean:
            return []

        for i in range(len(clean) - 1):
            if not re.match(r"^\s*[-+─┼]+\s*$", clean[i + 1]):
                continue
            rows = []
            for line in clean[i + 2 :]:
                if re.match(r"^\s*$", line) or re.match(r"^\s*\(\d+ rows?\)\s*$", line):
                    break
                if re.match(r"^\s*[-+─┼]+\s*$", line):
                    continue
                rows.append(self.split_psql_row(line))
            if rows:
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

    def values_equal(self, left, right):
        left_value = self.comparable_value(left)
        right_value = self.comparable_value(right)
        if left_value == right_value:
            return True
        if re.match(r"^-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?$", left_value) and re.match(
            r"^-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?$", right_value
        ):
            return abs(float(left_value) - float(right_value)) <= 1e-12
        return False

    def rows_equal(self, left, right):
        if len(left) == 1 and len(left[0]) == 1 and len(right) > 1:
            if all(len(row) == 1 for row in right):
                actual = self.comparable_value(left[0][0])
                if actual == self.comparable_value("".join(row[0] for row in right)):
                    return True
                if actual == self.comparable_value(" ".join(row[0] for row in right)):
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

    def rows_to_string(self, rows):
        return "\n".join(" | ".join(row) for row in rows)

    def opt_in_examples(self):
        examples = []
        nodes = self.doc.xpath(
            "//db:programlisting[contains(concat(' ', normalize-space(@role), ' '), ' example-test ')]",
            namespaces=DB,
        )
        for node in nodes:
            text = self.node_text(node)
            query, expected = None, None
            if self.has_inband_expected(text):
                query, expected = self.parse_inband_example(text)
            else:
                screen = self.following_screen(node)
                if screen is not None:
                    query, expected = self.parse_adjacent_example(text, self.node_text(screen))
            examples.append(
                {
                    "label": self.source_label(node),
                    "query": query,
                    "expected": expected,
                    "valid": query is not None and expected is not None,
                }
            )
        return examples

    def print_report(self):
        stats = {
            "programlisting_total": 0,
            "sql_like": 0,
            "select_or_with": 0,
            "inband_expected": 0,
            "psql_meta": 0,
            "external_context": 0,
            "placeholder_output": 0,
            "notice_or_error": 0,
            "cleanish_runnable_query": 0,
            "obvious_not_runnable_query": 0,
            "cleanish_inband_expected": 0,
            "obvious_bad_inband_expected": 0,
            "adjacent_programlisting_screen": 0,
            "cleanish_adjacent_expected": 0,
            "obvious_bad_adjacent_expected": 0,
        }

        for node in self.doc.xpath("//db:programlisting", namespaces=DB):
            text = self.node_text(node)
            stats["programlisting_total"] += 1
            if self.looks_sql(text):
                stats["sql_like"] += 1
            if not self.looks_query(text):
                continue
            stats["select_or_with"] += 1

            reason = self.obvious_skip_reason(text)
            if reason == "psql-meta":
                stats["psql_meta"] += 1
            if reason == "external-context":
                stats["external_context"] += 1
            if reason == "placeholder-output":
                stats["placeholder_output"] += 1
            if reason == "notice-or-error":
                stats["notice_or_error"] += 1

            if reason:
                stats["obvious_not_runnable_query"] += 1
            else:
                stats["cleanish_runnable_query"] += 1

            if self.has_inband_expected(text):
                stats["inband_expected"] += 1
                if reason:
                    stats["obvious_bad_inband_expected"] += 1
                else:
                    stats["cleanish_inband_expected"] += 1

            screen = self.following_screen(node)
            if screen is not None:
                stats["adjacent_programlisting_screen"] += 1
                screen_text = self.node_text(screen)
                bad_screen = re.search(r"\.\.\.|NOTICE:|ERROR:|ST_AsText output|Packaging extension", screen_text, re.I)
                if reason or bad_screen:
                    stats["obvious_bad_adjacent_expected"] += 1
                else:
                    stats["cleanish_adjacent_expected"] += 1

        examples = self.opt_in_examples()
        stats["opt_in_example_tests"] = len(examples)
        stats["opt_in_parseable_example_tests"] = len([example for example in examples if example["valid"]])

        for key in sorted(stats):
            print(f"{key}={stats[key]}")

    def sql_quote(self, text):
        return "'" + text.replace("'", "''") + "'"

    def sql_array(self, values):
        return "ARRAY[" + ", ".join(self.sql_quote(value) for value in values) + "]::text[]"

    def sql_2d_array(self, rows):
        return "ARRAY[" + ", ".join(self.sql_array(row) for row in rows) + "]::text[][]"

    def generate_sql(self):
        examples = self.opt_in_examples()

        print("-- Generated by utils/postgis_exampletest.py. Do not edit by hand.")
        print("\\set ON_ERROR_STOP on")
        print("BEGIN;")
        print(
            """CREATE OR REPLACE FUNCTION pg_temp._postgis_exampletest_check(
\ttest_label text,
\ttest_query text,
\texpected text[][]
)
RETURNS void
LANGUAGE plpgsql
AS $$
DECLARE
\tactual text[][];
BEGIN
\tEXECUTE format(
\t\t$fmt$
\t\tWITH q AS (%s),
\t\tnumbered AS (
\t\t\tSELECT row_number() OVER () AS rn, q.* FROM q
\t\t),
\t\trows AS (
\t\t\tSELECT rn, array_agg(value ORDER BY ord) AS vals
\t\t\tFROM numbered
\t\t\tCROSS JOIN LATERAL json_each_text(row_to_json(numbered)) WITH ORDINALITY AS e(key, value, ord)
\t\t\tWHERE key <> 'rn'
\t\t\tGROUP BY rn
\t\t)
\t\tSELECT coalesce(array_agg(vals ORDER BY rn), ARRAY[]::text[][])
\t\tFROM rows
\t\t$fmt$,
\t\ttest_query
\t) INTO actual;

\tIF actual IS DISTINCT FROM expected THEN
\t\tRAISE EXCEPTION E'Example test failed: %\\nQuery: %\\nExpected: %\\nActual: %',
\t\t\ttest_label, test_query, expected, actual;
\tEND IF;
END;
$$;
"""
        )

        emitted = 0
        for example in examples:
            if not example["valid"]:
                raise RuntimeError(f"Could not parse example-test at {example['label']}")
            emitted += 1
            print("SELECT pg_temp._postgis_exampletest_check(")
            print(f"\t{self.sql_quote(example['label'])},")
            print(f"\t{self.sql_quote(example['query'])},")
            print(f"\t{self.sql_2d_array(example['expected'])}")
            print(");\n")

        print("COMMIT;")
        print(f"\\echo 'manual example tests passed: {emitted}'")

    def run_psql_query(self, database, query):
        cmd = ["psql", "-X", "-v", "ON_ERROR_STOP=1", "-P", "null=null", "-d", database, "-c", query]
        result = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8")
        if result.returncode != 0:
            raise RuntimeError(f"psql failed for query:\n{query}\n{result.stdout}{result.stderr}")
        return self.expected_rows_from_psql_lines(result.stdout.split("\n"))

    def run_examples(self, database):
        examples = self.opt_in_examples()
        ran = 0

        for example in examples:
            if not example["valid"]:
                raise RuntimeError(f"Could not parse example-test at {example['label']}")

            try:
                actual = self.run_psql_query(database, example["query"])
            except RuntimeError as exc:
                raise RuntimeError(f"Example test failed to run: {example['label']}\n{exc}") from exc

            if not self.rows_equal(actual, example["expected"]):
                raise RuntimeError(
                    f"Example test failed: {example['label']}\n"
                    f"Query:\n{example['query']}\n"
                    f"Expected:\n{self.rows_to_string(example['expected'])}\n"
                    f"Actual:\n{self.rows_to_string(actual)}"
                )
            ran += 1

        print(f"manual example tests passed: {ran}")


def markable_programlisting(body):
    tester = ExampleTester.__new__(ExampleTester)
    if not tester.looks_query(body):
        return False
    if not tester.has_inband_expected(body):
        return False
    if tester.obvious_skip_reason(body):
        return False
    if "postgis=#" in body:
        return False

    query, expected = tester.parse_inband_example(body)
    if query is None or not expected:
        return False
    for row in expected:
        for value in row:
            if re.search(r"\b(?:SELECT|WITH)\b", value, re.I):
                return False
    if not re.search(r"^\s*(?:--[^\n]*\n\s*)*(?:SELECT|WITH)\b", query, re.I):
        return False
    if query.count(";") > 1:
        return False
    return True


def add_example_test_role(tag):
    if re.search(r"\brole\s*=\s*[\"'][^\"']*\bexample-test\b", tag):
        return tag
    match = re.search(r'\brole\s*=\s*"([^"]*)"', tag)
    if match:
        roles = (match.group(1) + " example-test").strip()
        return re.sub(r'\brole\s*=\s*"[^"]*"', f'role="{roles}"', tag, count=1)
    match = re.search(r"\brole\s*=\s*'([^']*)'", tag)
    if match:
        roles = (match.group(1) + " example-test").strip()
        return re.sub(r"\brole\s*=\s*'[^']*'", f'role="{roles}"', tag, count=1)
    return re.sub(r">", ' role="example-test">', tag, count=1)


def remove_example_test_role(tag):
    if not re.search(r"\brole\s*=", tag):
        return tag

    double_match = re.search(r'\brole\s*=\s*"([^"]*)"', tag)
    if double_match:
        roles = [role for role in double_match.group(1).split() if role != "example-test"]
        if roles:
            return re.sub(r'\brole\s*=\s*"[^"]*"', f'role="{" ".join(roles)}"', tag, count=1)
        return re.sub(r'\s+\brole\s*=\s*"[^"]*"', "", tag, count=1)

    single_match = re.search(r"\brole\s*=\s*'([^']*)'", tag)
    if single_match:
        roles = [role for role in single_match.group(1).split() if role != "example-test"]
        if roles:
            return re.sub(r"\brole\s*=\s*'[^']*'", f'role="{" ".join(roles)}"', tag, count=1)
        return re.sub(r"\s+\brole\s*=\s*'[^']*'", "", tag, count=1)

    return tag


def mark_source_files(files):
    listing_re = re.compile(r"(<programlisting\b[^>]*>)(.*?)(</programlisting>)", re.S)
    for filename in files:
        path = Path(filename)
        content = path.read_text(encoding="utf-8")
        changed = 0

        def replace(match):
            nonlocal changed
            opening, body, closing = match.groups()
            if markable_programlisting(body):
                new_opening = add_example_test_role(opening)
            else:
                new_opening = remove_example_test_role(opening)
            if new_opening != opening:
                changed += 1
            return new_opening + body + closing

        new_content = listing_re.sub(replace, content)
        if changed:
            path.write_text(new_content, encoding="utf-8")
        print(f"{filename}: marked {changed} examples")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Report, generate, or run opt-in manual example tests.",
        usage="%(prog)s --report|--generate-sql|--run [--database DB] <postgis-out.xml>\n"
        "       %(prog)s --mark-source <xml-file> [<xml-file> ...]",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--report", action="store_true")
    mode.add_argument("--generate-sql", action="store_true")
    mode.add_argument("--run", action="store_true")
    mode.add_argument("--mark-source", action="store_true")
    parser.add_argument("--database")
    parser.add_argument("files", nargs="+")
    return parser.parse_args()


def main():
    args = parse_args()

    try:
        if args.mark_source:
            mark_source_files(args.files)
            return 0

        if len(args.files) != 1:
            raise RuntimeError("exactly one XML file is required")
        tester = ExampleTester(args.files[0])

        if args.report:
            tester.print_report()
        elif args.generate_sql:
            tester.generate_sql()
        elif args.run:
            if not args.database:
                raise RuntimeError("--database is required with --run")
            tester.run_examples(args.database)
        return 0
    except Exception as exc:
        print(exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
