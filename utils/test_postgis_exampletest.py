#!/usr/bin/env python3

import contextlib
import base64
from collections import Counter
import hashlib
import io
import json
import os
from pathlib import Path
import re
import tempfile
import unittest
from unittest import mock

from postgis_exampletest import ExampleTester, QueryRows, parse_source_example


def xyz_vertices(points, ring=False):
    return [
        {"path": [1, index] if ring else [index], "point": point}
        for index, point in enumerate(points, 1)
    ]


class ExampleTestParserTest(unittest.TestCase):
    def setUp(self):
        self.tester = ExampleTester.__new__(ExampleTester)

    def test_empty_psql_table_is_empty_rows(self):
        rows = self.tester.expected_rows_from_psql_lines(
            [
                " name | default_version | installed_version",
                "------+-----------------+-------------------",
                "(0 rows)",
            ]
        )
        self.assertEqual([], rows)

    def test_non_empty_psql_table_rows_are_parsed(self):
        rows = self.tester.expected_rows_from_psql_lines(
            [
                " name    | default_version | installed_version",
                "---------+-----------------+-------------------",
                " postgis | 3.7.0dev        | null",
                "(1 row)",
            ]
        )
        self.assertEqual([["postgis", "3.7.0dev", "null"]], rows)

    def test_unicode_psql_table_rows_are_parsed(self):
        rows = self.tester.expected_rows_from_psql_lines(
            [
                "cube_surface_area │ solid_surface_area",
                "──────────────────┼────────────────────",
                "6                 │ 0",
            ]
        )
        self.assertEqual([["6", "0"]], rows)

    def test_psql_headers_are_preserved_for_visual_labels(self):
        lines = [
            " center | nearest ",
            "--------+---------",
            " POINT(0 0) | POINT(1 1)",
            "(1 row)",
        ]
        self.assertEqual(["center", "nearest"], self.tester.psql_headers_from_lines(lines))

    def test_psql_expanded_record_is_transposed_into_one_row(self):
        lines = [
            "-[ RECORD 1 ]-------------------",
            "auth_name | EPSG",
            "auth_srid | 3005",
            "srname    | NAD83 / BC Albers",
        ]
        self.assertEqual(
            [["EPSG", "3005", "NAD83 / BC Albers"]],
            self.tester.expected_rows_from_psql_lines(lines),
        )
        self.assertEqual(
            ["auth_name", "auth_srid", "srname"],
            self.tester.psql_headers_from_lines(lines),
        )

    def test_multiple_psql_expanded_records_become_multiple_rows(self):
        lines = [
            "-[ RECORD 1 ]---",
            "name | first",
            "srid | 1",
            "-[ RECORD 2 ]---",
            "name | second",
            "srid | 2",
        ]
        self.assertEqual(
            [["first", "1"], ["second", "2"]],
            self.tester.expected_rows_from_psql_lines(lines),
        )
        self.assertEqual(["name", "srid"], self.tester.psql_headers_from_lines(lines))

    def test_multiline_programlisting_is_trimmed(self):
        query, expected = parse_source_example(
            """
SELECT ST_AsText(
    ST_Point(1, 2));
""",
            """
POINT(1 2)
""",
        )
        self.assertEqual("SELECT ST_AsText(\n    ST_Point(1, 2))", query)
        self.assertEqual([["POINT(1 2)"]], expected)

    def test_function_whitespace_is_allowed(self):
        self.assertTrue(self.tester.query_is_auto_safe("SELECT ST_AsText ('POINT(1 2)'::geometry);"))

    def test_geometry_candidates_are_lexical_and_comment_aware(self):
        sql = """-- 'POINT(9 9)'
SELECT 'POINT(1 2)', $$LINESTRING(0 0,1 1)$$,
       'not geometry', /* 'POINT(8 8)' */ 'CURVEPOLYGON(CIRCULARSTRING(0 0,1 1,2 0,1 -1,0 0))';
"""
        self.assertEqual(
            [
                "POINT(1 2)",
                "LINESTRING(0 0,1 1)",
                "CURVEPOLYGON(CIRCULARSTRING(0 0,1 1,2 0,1 -1,0 0))",
            ],
            self.tester.geometry_candidates(sql, quoted_only=True),
        )

    def test_geometry_candidates_reject_unclosed_wkt(self):
        with self.assertRaisesRegex(RuntimeError, "Unclosed geometry candidate"):
            self.tester.geometry_candidates("POLYGON((0 0,1 0,0 0)")

    def test_hexwkb_geometry_recognizes_endian_flags_offsets_and_extended_types(self):
        def payload(type_word, byte_order=1):
            endian = "little" if byte_order == 1 else "big"
            encoded_type = type_word & 0x0FFFFFFF
            dimension_offset = encoded_type // 1000
            base_type = encoded_type % 1000 if dimension_offset in (1, 2, 3) else encoded_type
            dimensions = 2
            dimensions += int(bool(type_word & 0x80000000) or dimension_offset in (1, 3))
            dimensions += int(bool(type_word & 0x40000000) or dimension_offset in (2, 3))
            body = b""
            if type_word & 0x20000000:
                body += (4326).to_bytes(4, endian)
            if base_type == 1:
                body += b"\x00" * (dimensions * 8)
            elif base_type == 21:
                body += b"\x00" * 12
            else:
                body += b"\x00" * 4
            return (bytes([byte_order]) + type_word.to_bytes(4, endian) + body).hex()

        type_names = {
            1: "POINT", 2: "LINESTRING", 3: "POLYGON", 4: "MULTIPOINT",
            5: "MULTILINESTRING", 6: "MULTIPOLYGON", 7: "GEOMETRYCOLLECTION",
            8: "CIRCULARSTRING", 9: "COMPOUNDCURVE", 10: "CURVEPOLYGON",
            11: "MULTICURVE", 12: "MULTISURFACE", 15: "POLYHEDRALSURFACE",
            16: "TIN", 17: "TRIANGLE", 21: "NURBSCURVE",
        }
        cases = [
            (payload(type_code, byte_order), expected_type)
            for type_code, expected_type in type_names.items()
            for byte_order in (0, 1)
        ]
        cases.extend((
            (payload(0xE0000001), "POINT"),
            (payload(3003, byte_order=0), "POLYGON"),
            (payload(0x20000000 | 2021), "NURBSCURVE"),
            (payload(0x20000000 | 3021, byte_order=0), "NURBSCURVE"),
        ))
        for value, expected_type in cases:
            with self.subTest(value=value):
                self.assertEqual(
                    expected_type,
                    self.tester.hexwkb_geometry(value)["type"],
                )

    def test_hexwkb_geometry_rejects_false_positives_and_bbox_flag(self):
        self.assertIsNone(self.tester.hexwkb_geometry("0103"))
        self.assertIsNone(self.tester.hexwkb_geometry("0103000000"))
        self.assertIsNone(self.tester.hexwkb_geometry("010300000"))
        self.assertIsNone(self.tester.hexwkb_geometry("0203000000"))
        self.assertIsNone(self.tester.hexwkb_geometry("0100000000"))
        self.assertIsNone(self.tester.hexwkb_geometry("0112000000"))
        self.assertIsNone(self.tester.hexwkb_geometry("0103000010"))
        self.assertIsNone(self.tester.hexwkb_geometry("01E9030080" + "00" * 32))
        self.assertIsNone(self.tester.hexwkb_geometry("0115000080" + "00" * 24))
        self.assertIsNone(self.tester.hexwkb_geometry("01E9030020" + "00" * 32))
        self.assertIsNone(self.tester.hexwkb_geometry("0115000000" + "00" * 4))
        self.assertIsNone(self.tester.hexwkb_geometry("89504E470D0A1A0A"))
        self.assertIsNone(self.tester.hexwkb_geometry("not hex"))

    def test_hexwkb_geometry_accepts_bytea_prefix_only_when_requested(self):
        self.assertEqual(
            {"hexwkb": "010300000000000000", "type": "POLYGON"},
            self.tester.hexwkb_geometry("  \\x010300000000000000  "),
        )
        self.assertIsNone(
            self.tester.hexwkb_geometry("\\x010300000000000000", allow_bytea_prefix=False)
        )

    def test_geometry_output_hex_requires_geometry_type_metadata(self):
        value = "010300000000000000"
        self.assertEqual("POLYGON", self.tester.geometry_output_hex(value, "geometry")["type"])
        self.assertEqual("POLYGON", self.tester.geometry_output_hex(value, "geography")["type"])
        self.assertIsNone(self.tester.geometry_output_hex(value, "text"))
        self.assertIsNone(self.tester.geometry_output_hex("\\x" + value, "bytea"))

    def test_textual_surface_wkt_compares_canonically(self):
        actual = QueryRows(
            [[
                "POLYHEDRALSURFACE Z ("
                "((0 0 0,0 0 2,0 2 0,0 0 0)),"
                "((0 0 0,0 2 0,2 0 0,0 0 0))"
                ")"
            ]],
            types=["text"],
        )
        expected = [[
            "POLYHEDRALSURFACE Z ("
            "((0 0 0,0 2 0,2 0 0,0 0 0)),"
            "((0 0 0,0 0 2,0 2 0,0 0 0))"
            ")"
        ]]
        self.assertTrue(self.tester.typed_rows_equal("unused", actual, expected))

    def test_multiline_single_column_wkt_rows_are_coalesced(self):
        rows = QueryRows([
            ["POLYGON((234776.9 899563.7,234896.5 899456.7,"],
            ["234914 899436.4,234946.6 899356.9,"],
            ["234776.9 899563.7))"],
        ])
        self.assertEqual(
            [[
                "POLYGON((234776.9 899563.7,234896.5 899456.7,"
                "234914 899436.4,234946.6 899356.9,"
                "234776.9 899563.7))"
            ]],
            self.tester.coalesce_single_column_wkt_rows(rows),
        )

    def test_function_type_substring_does_not_swallow_argument_geometry(self):
        self.assertEqual(
            ["POLYGON((0 0,2 0,0 2,0 0))", "POINT(1 1)"],
            self.tester.geometry_candidates(
                "SELECT ST_ClosestPoint('POLYGON((0 0,2 0,0 2,0 0))', 'POINT(1 1)')",
                quoted_only=True,
            ),
        )

    def test_visual_skip_keeps_example_test_without_rendering(self):
        xml = """<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="example">
    <refsection>
      <programlisting role="visual-skip">SELECT ST_AsText('LINESTRING(0 0,1 1)'::geometry);</programlisting>
      <screen>LINESTRING(0 0,1 1)</screen>
    </refsection>
  </refentry>
</book>"""
        with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
            source.write(xml)
            source.flush()
            example = ExampleTester(source.name).examples()[0]
        self.assertTrue(example["valid"])
        self.assertIsNone(example["visual_id"])

    def test_explicit_visual_can_be_selected_from_binary_image_output(self):
        xml = """<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="image-example">
    <refsection>
      <programlisting>SELECT ST_AsPNG(ST_AsRaster(ST_Buffer(ST_Point(1, 5), 10), 150, 150)) AS image;</programlisting>
      <screen role="visual-primary"> image
-------
 PNG image, 150 x 150 pixels
      </screen>
    </refsection>
  </refentry>
</book>"""
        with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
            source.write(xml)
            source.flush()
            example = ExampleTester(source.name).examples()[0]
        self.assertEqual("explicit", example["visual_kind"])
        self.assertEqual("visual-image-example-01", example["visual_id"])

    def test_visual_separate_output_role_is_preserved(self):
        xml = """<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="comparison">
    <refsection>
      <programlisting>SELECT ST_AsText('LINESTRING(0 0,1 1)'), ST_AsText('POINT(0 0)');</programlisting>
      <screen role="visual-separate-output">LINESTRING(0 0,1 1) | POINT(0 0)</screen>
    </refsection>
  </refentry>
</book>"""
        with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
            source.write(xml)
            source.flush()
            example = ExampleTester(source.name).examples()[0]
        self.assertTrue(example["visual_separate_output"])

    def test_visual_overlay_role_is_preserved(self):
        xml = """<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="overlay">
    <refsection>
      <programlisting>SELECT ST_AsText(ST_Buffer('POINT(0 0)'::geometry, 1));</programlisting>
      <screen role="visual-overlay">POLYGON((0 0,1 0,0 1,0 0))</screen>
    </refsection>
  </refentry>
</book>"""
        with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
            source.write(xml)
            source.flush()
            example = ExampleTester(source.name).examples()[0]
        self.assertTrue(example["visual_overlay"])

    def test_visual_layout_roles_do_not_disable_self_contained_queries(self):
        for role, flag in (
            ("visual-overlay", "visual_overlay"),
            ("visual-direction", "visual_direction"),
            ("visual-separate-output", "visual_separate_output"),
        ):
            with self.subTest(role=role):
                xml = f"""<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="alpha-shape">
    <refsection>
      <programlisting>SELECT ST_AsText(CG_AlphaShape('MULTIPOINT((0 0),(1 0),(0 1))'::geometry, 1));</programlisting>
      <screen role="{role}">POLYGON((0 0,1 0,0 1,0 0))</screen>
    </refsection>
  </refentry>
</book>"""
                with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
                    source.write(xml)
                    source.flush()
                    example = ExampleTester(source.name).examples()[0]
                self.assertFalse(example["documented_only"])
                self.assertEqual("explicit", example["visual_kind"])
                self.assertTrue(example[flag])

    def test_documented_output_role_keeps_explicit_visual_without_execution(self):
        xml = """<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="preview">
    <refsection>
      <programlisting role="documented-output">SELECT ExpensiveVersionSensitiveGeometry() AS geom;</programlisting>
      <screen role="visual-primary">POLYGON((0 0,1 0,0 1,0 0))</screen>
    </refsection>
  </refentry>
</book>"""
        with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
            source.write(xml)
            source.flush()
            example = ExampleTester(source.name).examples()[0]
        self.assertTrue(example["documented_only"])
        self.assertEqual("explicit", example["visual_kind"])
        self.assertEqual("visual-preview-01", example["visual_id"])

    def test_documented_output_role_keeps_auto_visual_without_execution(self):
        xml = """<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="preview">
    <refsection>
      <programlisting role="documented-output">SELECT ST_Buffer('POINT(0 0)'::geometry, 1);</programlisting>
      <screen>POLYGON((1 0,0 1,-1 0,0 -1,1 0))</screen>
    </refsection>
  </refentry>
</book>"""
        with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
            source.write(xml)
            source.flush()
            example = ExampleTester(source.name).examples()[0]
        self.assertTrue(example["documented_only"])
        self.assertEqual("geometry-output", example["visual_kind"])
        self.assertEqual("visual-preview-01", example["visual_id"])

    def test_self_contained_functions_are_not_skipped_by_name(self):
        tester = ExampleTester.__new__(ExampleTester)
        for function in (
            "ST_MinimumSpanningTree",
            "ST_ClusterKMeans",
            "CG_3DConvexHull",
            "CG_AlphaShape",
            "CG_OptimalConvexPartition",
        ):
            with self.subTest(function=function):
                self.assertEqual("", tester.obvious_skip_reason(f"SELECT {function}(geom)"))

    def test_capability_roles_are_parsed_and_compared_generically(self):
        xml = """<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="capability"><refsection>
    <programlisting role="requires-geos-3.15 requires-proj-9.2 requires-cgal-6.1 requires-protobuf-1.1.0 requires-raster-1.0.0">SELECT 1;</programlisting>
    <screen>1</screen>
  </refsection></refentry>
</book>"""
        with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
            source.write(xml)
            source.flush()
            tester = ExampleTester(source.name)
            example = tester.examples()[0]
        self.assertEqual(
            ["geos", "proj", "cgal", "protobuf", "raster"],
            [item["name"] for item in example["requirements"]],
        )
        self.assertTrue(tester.requirements_satisfied(
            example["requirements"], {
                "geos": (3, 15, 1),
                "proj": (9, 2),
                "cgal": (6, 1),
                "protobuf": (1, 5, 2),
                "raster": (1, 0, 0),
            }
        ))
        self.assertFalse(tester.requirements_satisfied(
            example["requirements"], {
                "geos": (3, 14, 9),
                "proj": (9, 5),
                "cgal": (6, 2),
                "protobuf": (1, 5, 2),
                "raster": (1, 0, 0),
            }
        ))
        self.assertEqual(
            (6, 1, 0),
            tester.capability_version(
                "cgal",
                'SFCGAL="2.3.0" CGAL="6.1.0" Boost="1.83.0"',
            ),
        )
        self.assertEqual(
            (),
            tester.capability_version("cgal", 'SFCGAL="2.3.0" Boost="1.83.0"'),
        )

    def test_capability_requirements_are_inferred_from_known_functions(self):
        xml = """<book xmlns="http://docbook.org/ns/docbook">
  <refentry xml:id="capability"><refsection>
    <programlisting>SELECT ST_MakeEmptyRaster(1, 1, 0, 0, 1, -1, 0, 0, 0),
                           ST_CoverageEdges('MULTIPOLYGON EMPTY'::geometry),
                           CG_PolygonRepair('POLYGON EMPTY'::geometry);</programlisting>
    <screen>dummy</screen>
  </refsection></refentry>
</book>"""
        with tempfile.NamedTemporaryFile("w", suffix=".xml", encoding="utf-8") as source:
            source.write(xml)
            source.flush()
            example = ExampleTester(source.name).examples()[0]

        requirements = {item["name"]: item["minimum"] for item in example["requirements"]}
        self.assertEqual((1, 0, 0), requirements["raster"])
        self.assertEqual((3, 15, 0), requirements["geos"])
        self.assertEqual((2, 3, 0), requirements["sfcgal"])
        self.assertEqual((6, 0, 0), requirements["cgal"])

    def test_cg_3dbuffer_requires_sfcgal_2_0(self):
        tester = ExampleTester.__new__(ExampleTester)

        requirements = tester.inferred_capability_requirements(
            "SELECT CG_3DBuffer('POINT EMPTY'::geometry, 1, 8, 0)"
        )

        self.assertEqual([{
            "name": "sfcgal",
            "minimum": (2, 0, 0),
            "role": "auto-requires-sfcgal-2.0.0",
        }], requirements)

    def test_legacy_2d_wkt_output_compares_against_2d_projection(self):
        tester = ExampleTester.__new__(ExampleTester)
        query = tester.geometry_comparison_query(
            "0101000040000000000000F03F00000000000000400000000000000840",
            expected_wkt="POINT(1 2)",
            actual_type="POINT",
        )
        self.assertIn("ST_Force2D(ST_GeomFromEWKB", query)
        self.assertIn("ST_Force2D(ST_GeomFromEWKT", query)

    def test_implicit_3d_wkt_output_keeps_z_for_historical_postgis_spelling(self):
        tester = ExampleTester.__new__(ExampleTester)
        query = tester.geometry_comparison_query(
            "0101000080000000000000F03F00000000000000400000000000000840",
            expected_wkt="POINT(1 2 3)",
            actual_type="POINT",
        )
        self.assertNotIn("ST_Force2D", query)

    def test_explicit_geometry_output_precision_allows_hausdorff_tolerance(self):
        tester = ExampleTester.__new__(ExampleTester)
        query = tester.geometry_comparison_query(
            "0101000000000000000000F03F0000000000000040",
            expected_wkt="POINT(1.000000 2.000000)",
            actual_type="POINT",
            expected_wkt_digits=6,
        )
        self.assertIn("ST_HausdorffDistance(actual, expected) <= 9.9999999999999995e-07", query)

    def test_psql_environment_sets_default_query_timeout(self):
        tester = ExampleTester.__new__(ExampleTester)
        with mock.patch.dict(os.environ, {}, clear=True):
            env = tester.psql_environment()
        self.assertIn("postgis.gdal_enabled_drivers=PNG", env["PGOPTIONS"])
        self.assertIn("statement_timeout=60s", env["PGOPTIONS"])

    def test_psql_environment_allows_timeout_override(self):
        tester = ExampleTester.__new__(ExampleTester)
        with mock.patch.dict(
            os.environ,
            {"POSTGIS_EXAMPLETEST_STATEMENT_TIMEOUT": "5s"},
            clear=True,
        ):
            env = tester.psql_environment()
        self.assertIn("statement_timeout=5s", env["PGOPTIONS"])

    def test_psql_subprocess_timeout_follows_statement_timeout(self):
        tester = ExampleTester.__new__(ExampleTester)
        with mock.patch.dict(
            os.environ,
            {"POSTGIS_EXAMPLETEST_STATEMENT_TIMEOUT": "5s"},
            clear=True,
        ):
            self.assertEqual(20.0, tester.psql_subprocess_timeout())

    def test_psql_subprocess_timeout_can_be_disabled(self):
        tester = ExampleTester.__new__(ExampleTester)
        with mock.patch.dict(
            os.environ,
            {"POSTGIS_EXAMPLETEST_SUBPROCESS_TIMEOUT": "off"},
            clear=True,
        ):
            self.assertIsNone(tester.psql_subprocess_timeout())


class ExampleTestComparisonTest(unittest.TestCase):
    def setUp(self):
        self.tester = ExampleTester.__new__(ExampleTester)

    def test_polygon_ring_start_is_canonicalized(self):
        self.assertTrue(
            self.tester.values_equal(
                "POLYGON((0 0,1 0,1 1,0 0))",
                "POLYGON((1 0,1 1,0 0,1 0))",
            )
        )
        self.assertFalse(
            self.tester.values_equal(
                "POLYGON((0 0,1 0,1 1,0 0))",
                "POLYGON((0 0,2 0,1 1,0 0))",
            )
        )

    def test_multipoint_wrapped_point_spelling_is_canonicalized(self):
        self.assertTrue(
            self.tester.values_equal(
                "MULTIPOINT(0.5 0.5,2.5 2.5)",
                "MULTIPOINT((0.5 0.5),(2.5 2.5))",
            )
        )
        self.assertFalse(
            self.tester.values_equal(
                "MULTIPOINT(0.5 0.5,2.5 2.5)",
                "MULTIPOINT(2.5 2.5,0.5 0.5)",
            )
        )

    def test_png_bytea_is_summarized_without_losing_visual_payload(self):
        raw = b"\x89PNG\r\n\x1a\n" + b"\x00" * 8 + (150).to_bytes(4, "big") + (90).to_bytes(4, "big")
        actual = QueryRows([["\\x" + raw.hex()]], ["image"])
        prepared = self.tester.prepare_visual_rows(actual, ["bytea"])
        self.assertEqual([["PNG image, 150 x 90 pixels"]], prepared)
        self.assertEqual(1, len(prepared.visuals))
        self.assertEqual("Output", prepared.visuals[0]["label"])
        self.assertEqual("image/png", prepared.visuals[0]["media_type"])
        self.assertEqual(base64.b64encode(raw).decode("ascii"), prepared.visuals[0]["data"])

    def test_non_image_bytea_remains_text_output(self):
        actual = QueryRows([["\\x0102"]], ["payload"])
        prepared = self.tester.prepare_visual_rows(actual, ["bytea"])
        self.assertEqual([["\\x0102"]], prepared)
        self.assertEqual([], prepared.visuals)

    def test_scalar_row_value_labels_image_gallery_items(self):
        raw = b"\x89PNG\r\n\x1a\n" + b"\x00" * 8 + (4).to_bytes(4, "big") + (5).to_bytes(4, "big")
        actual = QueryRows([["weighted neighbors", "\\x" + raw.hex()]], ["title", "image"])
        prepared = self.tester.prepare_visual_rows(actual, ["text", "bytea"])
        self.assertEqual("weighted neighbors", prepared.visuals[0]["label"])

    def test_image_svg_embeds_buildtime_output_and_uses_alias_labels(self):
        svg = self.tester.visual_image_svg("raster-example", [{
            "source": "Output", "label": "grayscale", "media_type": "image/png",
            "width": 2, "height": 3, "data": "YWJj",
        }])
        self.assertIn('href="data:image/png;base64,YWJj"', svg)
        self.assertIn('preserveAspectRatio="none"', svg)
        self.assertIn('image-rendering:pixelated', svg)
        self.assertIn('data-postgis-geometry-id="raster-example-output-1"', svg)
        self.assertIn('>grayscale</text>', svg)

    def test_image_svg_uses_integer_upscale_for_pixel_art(self):
        svg = self.tester.visual_image_svg("pixels", [{
            "source": "Output", "label": "Output", "media_type": "image/png",
            "width": 150, "height": 90, "data": "YWJj",
        }])
        self.assertRegex(svg, r'<image[^>]+width="450"[^>]+height="270"')

    def test_image_gallery_uses_one_shared_pixel_scale(self):
        svg = self.tester.visual_image_svg("crop", [
            {"source": "Code", "label": "before", "media_type": "image/png",
             "width": 200, "height": 200, "data": "YQ=="},
            {"source": "Output", "label": "after", "media_type": "image/png",
             "width": 150, "height": 150, "data": "Yg=="},
        ])
        dimensions = re.findall(r'<image[^>]+width="([0-9.]+)"[^>]+height="([0-9.]+)"', svg)
        self.assertEqual([("200", "200"), ("150", "150")], dimensions)

    def test_multipolygon_ring_start_and_polygon_order_are_canonicalized(self):
        self.assertTrue(
            self.tester.values_equal(
                "MULTIPOLYGON(((0 0,1 0,1 1,0 0)),((10 10,11 10,11 11,10 10)))",
                "MULTIPOLYGON(((11 10,11 11,10 10,11 10)),((1 0,1 1,0 0,1 0)))",
            )
        )

    def test_geometrycollection_polygon_members_are_canonicalized(self):
        self.assertTrue(
            self.tester.values_equal(
                "GEOMETRYCOLLECTION(POLYGON((0 0,1 0,1 1,0 0)),POLYGON((10 10,11 10,11 11,10 10)))",
                "GEOMETRYCOLLECTION(POLYGON((11 10,11 11,10 10,11 10)),POLYGON((1 0,1 1,0 0,1 0)))",
            )
        )

    def test_polyhedral_surface_faces_and_ring_starts_are_canonicalized(self):
        self.assertTrue(
            self.tester.values_equal(
                "POLYHEDRALSURFACE(((0 0,1 0,0 1,0 0)),((10 10,11 10,10 11,10 10)))",
                "POLYHEDRALSURFACE(((11 10,10 11,10 10,11 10)),((1 0,0 1,0 0,1 0)))",
            )
        )

    def test_tin_faces_and_ring_starts_are_canonicalized(self):
        self.assertTrue(
            self.tester.values_equal(
                "TIN(((0 0,1 0,0 1,0 0)),((10 10,11 10,10 11,10 10)))",
                "TIN(((11 10,10 11,10 10,11 10)),((1 0,0 1,0 0,1 0)))",
            )
        )

    def test_multiline_expected_wkt_uses_canonical_comparison(self):
        actual = [["GEOMETRYCOLLECTION(POLYGON((1 0,1 1,0 0,1 0)),POLYGON((11 10,11 11,10 10,11 10)))"]]
        expected = [
            ["GEOMETRYCOLLECTION(POLYGON((0 0,1 0,1 1,0 0)),"],
            ["POLYGON((10 10,11 10,11 11,10 10)))"],
        ]
        self.assertTrue(self.tester.rows_equal(actual, expected))

    def test_version_comparison_checks_only_shape(self):
        self.assertTrue(self.tester.version_rows_equal([["3.7.0dev", "GEOS 3.13"]], [["3.6.0", "GEOS 3.12"]]))
        self.assertFalse(self.tester.version_rows_equal([["3.7.0dev"]], [["3.6.0", "GEOS 3.12"]]))

    def test_catalog_comparison_allows_empty_actual_catalog(self):
        expected = [["postgis", "3.7.0dev", "null"]]
        self.assertTrue(self.tester.catalog_rows_equal([], expected))
        self.assertTrue(self.tester.catalog_rows_equal([["postgis", "3.7.0dev", "null"]], expected))
        self.assertFalse(self.tester.catalog_rows_equal([["postgis", "3.7.0dev"]], expected))

    def test_typed_hexwkb_comparison_is_endian_independent(self):
        ndr = "0101000000000000000000F03F0000000000000040"
        xdr = "00000000013FF00000000000004000000000000000"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "t"
        actual = QueryRows([[xdr]], types=["geometry"])
        self.assertTrue(self.tester.typed_rows_equal("manual", actual, [[ndr]]))
        self.assertIn("ST_DumpPoints(actual)", queries[0])
        self.assertIn("greatest(1.0, abs(a.x), abs(e.x))", queries[0])
        self.assertIn("GeometryType(actual) = GeometryType(expected)", queries[0])
        self.assertIn("ST_CoordDim(actual) = ST_CoordDim(expected)", queries[0])
        self.assertIn("ST_Zmflag(actual) = ST_Zmflag(expected)", queries[0])
        self.assertIn("ST_SRID(actual) = ST_SRID(expected)", queries[0])

    def test_typed_geometry_comparison_canonicalizes_unordered_areal_rings(self):
        polygon = (
            "0103000000010000000400000000000000000000000000000000000000"
            "000000000000F03F00000000000000000000000000000000000000000000F03F"
            "00000000000000000000000000000000"
        )
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "t"
        actual = QueryRows([[polygon]], types=["geometry"])
        self.assertTrue(self.tester.typed_rows_equal("manual", actual, [[polygon]]))
        self.assertIn("GeometryType(actual) IN ('POLYGON', 'MULTIPOLYGON', 'TRIANGLE')", queries[0])
        self.assertIn("ST_Normalize(actual)", queries[0])
        self.assertIn("generate_subscripts(points, 1)", queries[0])
        self.assertIn("jsonb_agg(face_key ORDER BY face_key)", queries[0])
        self.assertNotIn("array_agg(face_key ORDER BY face_key)", queries[0])

    def test_surface_comparison_canonicalizes_face_order_without_refentry_rules(self):
        query = self.tester.geometry_comparison_query(
            "00", expected_hex="00", actual_type="POLYHEDRALSURFACE"
        )
        self.assertEqual(2, query.count("jsonb_agg(face_key ORDER BY face_key)"))
        self.assertEqual(2, query.count("ST_Dump("))
        self.assertIn("ST_Normalize", query)
        self.assertNotIn("ST_AsEWKT", query)
        self.assertNotIn("ST_GeomFromEWKT(ST_AsEWKT", query)
        self.assertNotIn("refentry", query.lower())

    def test_geometrycollection_comparison_normalizes_member_order(self):
        query = self.tester.geometry_comparison_query(
            "00", expected_wkt="GEOMETRYCOLLECTION EMPTY", actual_type="GEOMETRYCOLLECTION",
        )
        self.assertIn("ST_AsEWKT(ST_Normalize(actual))", query)
        self.assertIn("ST_AsEWKT(ST_Normalize(expected))", query)
        self.assertNotIn("actual_components AS", query)

    def test_areal_comparison_uses_rounded_coordinate_keys(self):
        query = self.tester.geometry_comparison_query(
            "00", expected_wkt="POLYGON Z EMPTY", actual_type="POLYGON",
        )
        self.assertEqual(2, query.count("generate_subscripts(points, 1)"))
        self.assertEqual(2, query.count("concat_ws(' ',"))
        self.assertIn("round((CASE WHEN abs(ST_X(point.geom))", query)
        self.assertIn("round((CASE WHEN abs(ST_Z(point.geom))", query)
        self.assertIn("points[rotation:array_length(points, 1)]", query)
        self.assertIn("IS NOT DISTINCT FROM", query)
        self.assertNotIn("regexp_replace", query)
        self.assertNotIn("CG_3DDifference", query)

    def test_typed_hexwkb_comparison_accepts_readable_ewkt_expectation(self):
        ndr = "0101000020E6100000000000000000F03F0000000000000040"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "t"
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertTrue(
            self.tester.typed_rows_equal(
                "manual", actual, [["SRID=4326;POINT(1 2)"]]
            )
        )
        self.assertIn("ST_GeomFromEWKB", queries[0])
        self.assertIn("ST_GeomFromEWKT('SRID=4326;POINT(1 2)')", queries[0])
        self.assertNotIn("ST_AsText", queries[0])
        self.assertIn("ST_DumpPoints(actual)", queries[0])
        self.assertNotIn("regexp_replace", queries[0])

    def test_typed_hexwkb_comparison_accepts_readable_wkt_without_srid(self):
        ndr = "0102000020E610000002000000713D0AD7A38051C08FC2F5285C6F4540D7A3703D0A8751C0713D0AD7A3704540"
        queries = []
        def scalar(database, query):
            queries.append(query)
            if "ST_AsEWKT" in query:
                return "LINESTRING(-70.01 42.87,-70.11 42.88)"
            return "t"
        self.tester.run_psql_scalar = scalar
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertTrue(
            self.tester.typed_rows_equal(
                "manual", actual, [["LINESTRING(-70.01 42.87,-70.11 42.88)"]]
            )
        )
        self.assertIn("ST_AsEWKT", queries[0])

    def test_typed_hexwkb_comparison_preserves_explicit_srid_zero(self):
        ndr = "0101000000000000000000F03F0000000000000040"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "t"
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertTrue(
            self.tester.typed_rows_equal(
                "manual", actual, [["SRID=0;POINT(1 2)"]]
            )
        )
        self.assertIn("FALSE AND ST_SRID(expected) = 0", queries[0])

    def test_typed_hexwkb_comparison_accepts_legacy_3d_wkt_without_z_token(self):
        ndr = "010200008002000000010000000000F0BFFFFFFFFFFFFFFFBF0000000000000840020000000000F0BF00000000000010C00000000000000840"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "t"
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertTrue(
            self.tester.typed_rows_equal(
                "manual", actual, [["LINESTRING(-1 -2 3,-1 -4 3)"]]
            )
        )
        self.assertIn("ST_GeomFromEWKT('LINESTRING(-1 -2 3,-1 -4 3)')", queries[0])

    def test_typed_hexwkb_comparison_accepts_explicit_z_token(self):
        ndr = "010200008002000000010000000000F0BFFFFFFFFFFFFFFFBF0000000000000840020000000000F0BF00000000000010C00000000000000840"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "t"
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertTrue(
            self.tester.typed_rows_equal(
                "manual", actual, [["LINESTRING Z (-1 -2 3,-1 -4 3)"]]
            )
        )
        self.assertIn("ST_GeomFromEWKT('LINESTRING Z (-1 -2 3,-1 -4 3)')", queries[0])

    def test_rounded_typed_hexwkb_comparison_keeps_explicit_z_spelling(self):
        ndr = "010200008002000000010000000000F0BFFFFFFFFFFFFFFFBF0000000000000840020000000000F0BF00000000000010C00000000000000840"
        queries = []

        def scalar(database, query):
            queries.append(query)
            if "ST_AsText" in query:
                return "LINESTRING Z (-1.000 -2.000 3.000,-1.000 -4.000 3.000)"
            return "f"

        self.tester.run_psql_scalar = scalar
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertTrue(
            self.tester.typed_rows_equal(
                "manual",
                actual,
                [["LINESTRING Z (-1.000 -2.000 3.000,-1.000 -4.000 3.000)"]],
            )
        )
        self.assertIn("ST_AsText", queries[0])
        self.assertNotIn("ST_AsEWKT", queries[0])

    def test_typed_hexwkb_comparison_accepts_compact_ewkt_dimension_marker(self):
        point_m = "0101000040000000000000144000000000000028400000000000002640"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "t"
        actual = QueryRows([[point_m]], types=["geometry"])
        self.assertTrue(
            self.tester.typed_rows_equal(
                "manual", actual, [["POINTM(5 12 11)"]]
            )
        )
        self.assertIn("ST_GeomFromEWKT('POINTM(5 12 11)')", queries[0])
        self.assertNotIn("regexp_replace", queries[0])

    def test_typed_geometry_comparison_rejects_coordinate_mismatch(self):
        ndr = "010200000002000000000000000000F03F000000000000004000000000000008400000000000001040"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "f"
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertFalse(
            self.tester.typed_rows_equal(
                "manual", actual, [["LINESTRING(1 2,3 4)"]]
            )
        )
        self.assertEqual(1, len(queries))
        self.assertIn("ST_DumpPoints(actual)", queries[0])
        self.assertIn("greatest(1.0, abs(a.x), abs(e.x))", queries[0])

    def test_typed_nurbs_comparison_stays_native(self):
        nurbs = "011500000002000000030000000000000000000000000000000000000000000000000014400000000000002440000000000000000000000000"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "f"
        actual = QueryRows([[nurbs]], types=["geometry"])
        self.assertFalse(
            self.tester.typed_rows_equal(
                "manual", actual, [[nurbs]]
            )
        )
        self.assertEqual(1, len(queries))
        self.assertIn("ST_AsHEXEWKB(actual, 'XDR')", queries[0])
        self.assertIn("ST_AsHEXEWKB(expected, 'XDR')", queries[0])
        self.assertNotIn("ST_AsEWKT", queries[0])
        self.assertNotIn("ST_CurveToLine", queries[0])
        self.assertNotIn("ST_DumpPoints", queries[0])

    def test_typed_linestring_comparison_does_not_normalize_direction(self):
        ndr = "010200000002000000000000000000F03F000000000000004000000000000008400000000000001040"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "f"
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertFalse(
            self.tester.typed_rows_equal(
                "manual", actual, [["LINESTRING(3 4,1 2)"]]
            )
        )
        self.assertEqual(1, len(queries))
        self.assertIn(
            "CASE WHEN GeometryType(actual) IN "
            "('POLYGON', 'MULTIPOLYGON', 'TRIANGLE') THEN ST_Normalize(actual)",
            queries[0],
        )
        self.assertNotIn("ST_Reverse", queries[0])

    def test_typed_geometry_comparison_rejects_semantic_mismatch(self):
        ndr = "0101000020E6100000000000000000F03F0000000000000040"
        self.tester.run_psql_scalar = lambda database, query: "f"
        actual = QueryRows([[ndr]], types=["geometry"])
        self.assertFalse(
            self.tester.typed_rows_equal(
                "manual", actual, [["SRID=3857;POINT(1 2)"]]
            )
        )

    def test_typed_hexwkb_comparison_accepts_wrapped_readable_ewkt(self):
        ndr = "0101000020E6100000000000000000F03F0000000000000040"
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or "t"
        actual = QueryRows([[ndr]], types=["geometry"])

        self.assertTrue(
            self.tester.typed_rows_equal(
                "manual",
                actual,
                [["SRID=4326;POINT(1"], [" 2)"]],
            )
        )
        self.assertIn("ST_GeomFromEWKB", queries[0])
        self.assertIn("ST_DumpPoints(actual)", queries[0])

    def test_readable_geometry_expectation_is_detected(self):
        self.assertTrue(self.tester.expected_has_geometry_text([["POINT(1 2)"]]))
        self.assertTrue(self.tester.expected_has_geometry_text([["POLYGON EMPTY"]]))
        self.assertFalse(self.tester.expected_has_geometry_text([["not geometry"]]))


class ExampleTestRunnerTest(unittest.TestCase):
    def make_runner(self, actual_rows):
        tester = ExampleTester.__new__(ExampleTester)
        examples = []
        for label, query, expected in (
            ("first:1", "SELECT 1", [["1"]]),
            ("second:2", "SELECT 2", [["2"]]),
            ("third:3", "SELECT 3", [["3"]]),
        ):
            examples.append(
                {
                    "label": label,
                    "query": query,
                    "expected": expected,
                    "valid": True,
                    "catalog": False,
                    "version": False,
                    "visual_id": None,
                }
            )
        tester.examples = lambda: examples
        tester.check_environment = lambda database: None
        tester.run_psql_query = lambda database, query: actual_rows[query]
        return tester

    def test_run_examples_keep_going_reports_all_failures(self):
        tester = self.make_runner(
            {
                "SELECT 1": [["bad"]],
                "SELECT 2": [["2"]],
                "SELECT 3": [["also bad"]],
            }
        )

        stderr = io.StringIO()
        with self.assertRaisesRegex(RuntimeError, r"FAILED 2 example\(s\): first:1, third:3"):
            with contextlib.redirect_stderr(stderr):
                tester.run_examples("exampletest", keep_going=True)

        output = stderr.getvalue()
        self.assertIn("Example test failed: first:1", output)
        self.assertIn("Example test failed: third:3", output)
        self.assertIn("Actual:\nbad", output)
        self.assertIn("Actual:\nalso bad", output)

    def test_run_examples_without_keep_going_still_fails_fast(self):
        tester = self.make_runner(
            {
                "SELECT 1": [["bad"]],
                "SELECT 2": [["wrong but not reached"]],
                "SELECT 3": [["also not reached"]],
            }
        )

        with self.assertRaisesRegex(RuntimeError, "Example test failed: first:1") as raised:
            tester.run_examples("exampletest")

        self.assertNotIn("second:2", str(raised.exception))

    def test_visual_only_skips_unmarked_examples_and_global_environment_checks(self):
        tester = self.make_runner({"SELECT 1": [["1"]], "SELECT 2": [["2"]], "SELECT 3": [["3"]]})
        examples = tester.examples()
        examples[1]["visual_id"] = "second-visual"
        calls = []
        tester.check_environment = lambda database: self.fail("visual-only checked unrelated environment")
        tester.run_one_example = lambda database, example: calls.append(example["label"]) or example["expected"]
        tester.render_visual_example = lambda database, example, actual: {
            "id": example["visual_id"], "source": example["label"], "svg": "<svg/>\n"
        }
        tester.publish_visual_examples = lambda render_dir, visuals: calls.append(
            (render_dir, [visual["id"] for visual in visuals])
        )

        tester.run_examples("exampletest", render_dir="visuals", visual_only=True)

        self.assertEqual(["second:2", ("visuals", ["second-visual"])], calls)

    def test_visual_only_renders_typed_documented_output_at_any_job_count(self):
        hexwkb = "010300000000000000"
        for jobs in (1, 2):
            with self.subTest(jobs=jobs):
                tester = self.make_runner({"SELECT 1": [[hexwkb]]})
                example = tester.examples()[0]
                example.update({
                    "expected": [[hexwkb]],
                    "documented_only": True,
                    "expected_headers": ["documented_geom"],
                    "visual_id": "documented-visual",
                    "visual_kind": "explicit",
                    "visual_preferred": True,
                })
                tester.examples = lambda: [example]
                calls = []
                tester.run_one_example = lambda database, selected: self.fail(
                    "documented output was executed"
                )
                tester.render_visual_example = lambda database, selected, actual: calls.append(
                    (selected["visual_id"], actual)
                ) or {"id": selected["visual_id"], "source": selected["label"], "svg": "<svg/>\n"}
                tester.publish_visual_examples = lambda render_dir, visuals: calls.append(
                    (render_dir, [visual["id"] for visual in visuals])
                )

                tester.run_examples("exampletest", render_dir="visuals", visual_only=True, jobs=jobs)

                self.assertEqual("documented-visual", calls[0][0])
                self.assertEqual(example["expected"], calls[0][1])
                self.assertEqual(["documented_geom"], calls[0][1].headers)
                self.assertEqual(["geometry"], calls[0][1].types)
                self.assertEqual(("visuals", ["documented-visual"]), calls[1])

    def test_native_geometry_output_is_described_when_input_uses_serializer(self):
        hexwkb = "010200000002000000000000000000F03F0000000000000040" \
                 "00000000000008400000000000001040"
        tester = ExampleTester.__new__(ExampleTester)
        example = {
            "label": "ST_LineFromWKB:1",
            "query": "SELECT ST_LineFromWKB(ST_AsBinary(" \
                     "ST_GeomFromText('LINESTRING(1 2,3 4)'))) AS aline",
            "expected": [["LINESTRING(1 2,3 4)"]],
            "valid": True,
            "catalog": False,
            "version": False,
            "visual_id": "st-linefromwkb-1",
            "visual_kind": "geometry-output",
        }
        described = []
        tester.run_psql_query = lambda database, query: QueryRows(
            [[hexwkb]], ["aline"]
        )
        tester.query_output_types = lambda database, query: described.append(query) or ["geometry"]
        tester.run_psql_scalar = lambda database, query: "t"

        actual = tester.run_one_example("manual", example)

        self.assertEqual([example["query"]], described)
        self.assertEqual(["geometry"], actual.types)


class VisualExampleTest(unittest.TestCase):
    def setUp(self):
        self.tester = ExampleTester.__new__(ExampleTester)

    def point_radius(self, svg):
        match = re.search(r'class="point" d="M [^"]+ A ([0-9.]+) ([0-9.]+) 0 1 0', svg)
        self.assertIsNotNone(match)
        return float(match.group(1))

    def vertex_radius(self, svg):
        match = re.search(r'class="vertex"[^>]* r="([0-9.]+)"', svg)
        self.assertIsNotNone(match)
        return float(match.group(1))

    def test_svg_uses_postgis_fragments_and_stable_source_colors(self):
        svg = self.tester.visual_svg(
            "buffer-example",
            {
                "bounds": [0, 0, 10, 10],
                "parts": [
                    {"ord": 1, "source": "Code", "label": "Code 1", "type": "POINT", "x": 2, "y": -3},
                    {"ord": 2, "source": "Output", "label": "Output 1", "type": "POLYGON", "svg": "M 0 0 L 10 0 10 -10 Z"},
                ],
            },
        )
        self.assertIn('viewBox="0 0 600 380"', svg)
        self.assertIn('class="plot-grid"', svg)
        self.assertIn('pointer-events:stroke', svg)
        self.assertIn('pointer-events:all', svg)
        self.assertIn('data-postgis-geometry-id="buffer-example-code-1"', svg)
        self.assertRegex(
            svg,
            r'<path class="point" d="M [^\"]+ A [^\"]+ Z" fill="#2878b8" '
            r'stroke="white" stroke-width="[0-9.]+"/>',
        )
        self.assertIn(
            'd="M 0 0 L 10 0 10 -10 Z" stroke="#a62c2b" fill="#a62c2b"',
            svg,
        )
        self.assertIn('fill-opacity="0.24"', svg)
        self.assertIn('stroke-opacity="1"', svg)
        self.assertRegex(svg, r'fill-rule="evenodd" stroke-width="[0-9.]+"')
        self.assertIn('.geometry-layer.active{filter:brightness(.72)}', svg)
        self.assertIn('svg:has(.geometry-layer.active) .geometry-layer:not(.active){opacity:.18}', svg)
        self.assertIn('stroke="#dce2e7" stroke-width="1"', svg)
        self.assertIn('<text class="grid-label grid-label-x"', svg)
        self.assertIn('<text class="grid-label grid-label-y"', svg)
        self.assertIn('>0</text>', svg)
        self.assertIn('>10</text>', svg)
        self.assertIn('<rect width="11" height="11" rx="2" fill="#2878b8"/>', svg)
        self.assertIn('<rect width="11" height="11" rx="2" fill="#a62c2b"/>', svg)
        self.assertLess(
            svg.index('data-postgis-geometry-id="buffer-example-output-1"'),
            svg.index('data-postgis-geometry-id="buffer-example-code-1"'),
        )

    def test_svg_matrix_3x3_keeps_context_separate_from_output_grid(self):
        frames = [
            {"id": frame_id, "bounds": [0, 0, 1, 1]}
            for frame_id in ["a", "b"] + [f"cell-{index}" for index in range(9)]
        ]
        parts = [
            {"ord": 1, "source": "Code", "label": "a", "frame": "a", "type": "POINT", "x": 0, "y": 0},
            {"ord": 2, "source": "Code", "label": "b", "frame": "b", "type": "POINT", "x": 1, "y": 1},
        ]
        parts.extend(
            {
                "ord": index + 3,
                "source": "Output",
                "label": f"cell {index}",
                "frame": f"cell-{index}",
                "type": "POINT",
                "x": 0.5,
                "y": 0.5,
            }
            for index in range(9)
        )

        svg = self.tester.visual_svg(
            "matrix-cells",
            {"bounds": [0, 0, 1, 1], "frames": frames, "parts": parts},
            matrix_3x3=True,
        )

        backgrounds = re.findall(
            r'<rect class="plot-background" x="([^"]+)" y="([^"]+)"',
            svg,
        )
        self.assertEqual(11, len(backgrounds))
        row_counts = Counter(y for _x, y in backgrounds)
        self.assertEqual([2, 3, 3, 3], sorted(row_counts.values()))
        self.assertIn('viewBox="0 0 900 ', svg)

    def test_visual_candidate_includes_spatial_context_but_skips_trivial_points(self):
        self.assertIsNone(self.tester.visual_candidate("SELECT 'POINT(1 2)'", [["1"]]))
        self.assertIsNone(self.tester.visual_candidate(
            "SELECT ST_IsCollection('GEOMETRYCOLLECTION(POINT(0 0))')", [["t"]]
        ))
        self.assertIsNone(self.tester.visual_candidate(
            "SELECT ST_IsCollection("
            "'GEOMETRYCOLLECTION(MULTIPOINT((0 0),(1 1)),GEOMETRYCOLLECTION(POINT(2 2)))')",
            [["t"]],
        ))
        context = self.tester.visual_candidate(
            "SELECT ST_Intersects('LINESTRING(0 0,1 1)', 'POLYGON((0 0,2 0,2 2,0 0))')",
            [["t"]],
        )
        self.assertEqual({"kind": "input-context", "preferred": False}, context)
        collection_context = self.tester.visual_candidate(
            "SELECT ST_IsCollection("
            "'GEOMETRYCOLLECTION(POINT(0 0),LINESTRING(0 0,1 1))')",
            [["t"]],
        )
        self.assertEqual({"kind": "input-context", "preferred": False}, collection_context)
        output = self.tester.visual_candidate(
            "SELECT ST_Buffer('LINESTRING(0 0,1 1)', 1)",
            [["POLYGON((0 0,2 0,2 2,0 0))"]],
        )
        self.assertEqual({"kind": "geometry-output", "preferred": True}, output)
        point_output = self.tester.visual_candidate(
            "SELECT ST_Buffer('POINT(0 0)', 1)",
            [["POLYGON((0 0,1 0,1 1,0 0))"]],
        )
        self.assertEqual({"kind": "geometry-output", "preferred": True}, point_output)
        self.assertIsNone(self.tester.visual_candidate(
            "SELECT ST_AsText('LINESTRING(0 0,1 1)')", [["LINESTRING(0 0,1 1)"]]
        ))

    def test_visual_candidate_recognizes_natural_geometry_hex_output(self):
        polygon_hex = "010300000000000000"
        self.assertEqual(
            {"kind": "geometry-output", "preferred": True},
            self.tester.visual_candidate(
                "SELECT ST_PolygonFromText('POLYGON EMPTY')",
                [[polygon_hex]],
            ),
        )
        self.assertIsNone(self.tester.visual_candidate(
            "SELECT ST_AsHEXEWKB('POLYGON EMPTY'::geometry)",
            [[polygon_hex]],
        ))
        self.assertIsNone(self.tester.visual_candidate(
            "SELECT ST_PolygonFromText('POINT(1 2)')",
            [["0101000000000000000000F03F0000000000000040"]],
        ))

    def test_visual_candidate_does_not_draw_geography_inputs_as_planar(self):
        geography_context = self.tester.visual_candidate(
            "SELECT ST_Distance('LINESTRING(-122 47,0 51)'::geography, "
            "'POINT(-21 64)'::geography)",
            [["122235"]],
        )
        self.assertIsNone(geography_context)

        geometry_context = self.tester.visual_candidate(
            "SELECT ST_Distance('LINESTRING(-122 47,0 51)'::geometry, "
            "'POINT(-21 64)'::geometry)",
            [["13.3"]],
        )
        self.assertEqual({"kind": "input-context", "preferred": False}, geometry_context)

        geography_output = self.tester.visual_candidate(
            "SELECT ST_AsText(ST_Segmentize('LINESTRING(0 0,60 60)'::geography, 2000000))",
            [["LINESTRING(0 0,10 12,60 60)"]],
        )
        self.assertEqual({"kind": "geometry-output", "preferred": True}, geography_output)

    def test_code_geometry_candidates_include_constructed_context_in_sql_order(self):
        candidates = self.tester.code_geometry_candidates(
            "SELECT f('LINESTRING(0 0,10 10)', ST_Point(1, 2), "
            "ST_MakeEnvelope(2, 3, 8, 9, 4326))"
        )
        self.assertEqual([None, "Point", "Envelope"], [candidate.get("label") for candidate in candidates])
        self.assertEqual("LINESTRING(0 0,10 10)", candidates[0]["wkt"])
        self.assertEqual("POINT(1 2)", candidates[1]["wkt"])
        self.assertEqual(
            "SRID=4326;POLYGON((2 3,8 3,8 9,2 9,2 3))",
            candidates[2]["wkt"],
        )

    def test_code_geometry_candidates_use_cast_expression_aliases(self):
        candidates = self.tester.code_geometry_candidates(
            "SELECT 'LINESTRING(0 0,1 1)'::geometry AS line_a, "
            "'POINT(2 2)'::geography point_b, "
            "'POLYGON((0 0,1 0,0 0))'::geometry AS \"lake \"\"inner\"\" shore\""
        )
        self.assertEqual(
            ["line_a", "point_b", 'lake "inner" shore'],
            [candidate.get("label") for candidate in candidates],
        )

    def test_query_output_headers_use_only_outer_select_aliases(self):
        headers = self.tester.query_output_headers(
            "WITH data AS (SELECT 'POINT(0 0)'::geometry AS inner_geom), "
            "projected AS (SELECT inner_geom AS other_inner FROM data) "
            "SELECT ST_AsText(other_inner) AS input_geometry, "
            "ST_AsText(ST_Buffer(other_inner, 1)) AS result_geometry FROM projected"
        )
        self.assertEqual(["input_geometry", "result_geometry"], headers)

    def test_documented_visual_uses_query_aliases_for_layer_roles(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": 0, "y": 0,
            } for layer in layers],
        }
        actual = QueryRows([["POINT(0 0)", "POINT(1 1)"]])
        self.tester.render_visual_example("manual", {
            "query": (
                "WITH data AS (SELECT 1) "
                "SELECT 'POINT(0 0)' AS input_center, 'POINT(1 1)' AS result FROM data"
            ),
            "visual_id": "documented-aliases", "label": "aliases:documented",
            "visual_refentry": "aliases", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "explicit",
        }, actual)
        self.assertEqual(["Code", "Output"], [layer["source"] for layer in captured])
        self.assertEqual(["center", "result"], [layer["label"] for layer in captured])

    def test_separate_output_visual_requests_one_frame_per_output_label(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 1, 1],
            "frames": [
                {"id": "Code", "bounds": [0, 0, 1, 1]},
                {"id": "cleaned", "bounds": [0, 0, 1, 1]},
                {"id": "invalid edges", "bounds": [0, 0, 1, 1]},
            ],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "frame": layer["requested_frame"], "type": "LINESTRING",
                "svg": "M 0 0 L 1 -1",
            } for layer in layers],
        }
        actual = QueryRows(
            [["LINESTRING(0 0,1 1)", "LINESTRING(0 1,1 0)"]],
            headers=["cleaned", "invalid_edges"],
        )
        self.tester.render_visual_example("manual", {
            "query": "SELECT 'POLYGON((0 0,1 0,1 1,0 0))'::geometry",
            "visual_id": "separate-output", "label": "comparison:documented",
            "visual_refentry": "comparison", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "explicit",
            "visual_separate_output": True,
        }, actual)
        self.assertEqual(
            ["Code", "cleaned", "invalid edges"],
            [layer["requested_frame"] for layer in captured],
        )

    def test_overlay_visual_requests_one_shared_frame(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": 0, "y": 0,
            } for layer in layers],
        }
        actual = QueryRows([["POINT(1 1)"]], headers=["center"])
        self.tester.render_visual_example("manual", {
            "query": "SELECT 'POLYGON((0 0,1 0,1 1,0 0))'::geometry",
            "visual_id": "overlay", "label": "overlay:documented",
            "visual_refentry": "overlay", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "explicit",
            "visual_overlay": True,
        }, actual)
        self.assertEqual(["Overlay", "Overlay"], [layer["requested_frame"] for layer in captured])

    def test_visual_payload_separates_same_dimension_or_cross_srid_sources(self):
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or '{"frames":[],"parts":[]}'
        self.tester.visual_payload("manual", [])
        self.assertIn("source_dimensions", queries[0])
        self.assertIn("count(DISTINCT ST_SRID(geom))", queries[0])
        self.assertIn("AS shared_coordinate_space", queries[0])
        self.assertIn("bool_and(shared_coordinate_space)", queries[0])
        self.assertIn("CASE WHEN separate_sources THEN source ELSE 'Overlay' END", queries[0])
        self.assertIn("COALESCE(requested_frame", queries[0])
        self.assertIn("ST_Translate(geom, -min_x, -min_y)", queries[0])
        self.assertNotIn("frame_bounds_geoms AS", queries[0])
        self.assertNotIn("ST_CurveToLine", queries[0])
        self.assertIn("ST_Extent(ST_Force2D(all_geometries.geom))", queries[0])
        self.assertIn("ELSE ST_Extent(ST_Force2D(geom))", queries[0])
        self.assertIn("ELSE ST_AsSVG(ST_Force2D(render_geom), 0, 12)", queries[0])
        self.assertIn("WHEN ST_Dimension(render_geom) = 0 THEN NULL", queries[0])
        self.assertIn("'points'", queries[0])
        self.assertIn("ST_DumpPoints(render_geom) AS point", queries[0])
        self.assertIn("ST_Zmflag(geom) IN (2, 3)", queries[0])
        self.assertIn("'has_z'", queries[0])
        self.assertIn("'points_xyz'", queries[0])
        self.assertIn("CROSS JOIN LATERAL", queries[0])
        self.assertIn("generate_series(1, ST_NumGeometries(framed.geom))", queries[0])
        self.assertIn("ST_GeometryN(framed.geom, part_num.n) AS geom", queries[0])
        self.assertIn("WHERE NOT ST_IsEmpty(part.geom)", queries[0])
        self.assertNotIn("CROSS JOIN LATERAL ST_Dump", queries[0])
        self.assertNotIn("WHERE GeometryType(framed.geom) = 'CURVEPOLYGON'", queries[0])
        self.assertNotIn("WHERE GeometryType(framed.geom) <> 'CURVEPOLYGON'", queries[0])
        self.assertIn("geom AS render_geom", queries[0])
        self.assertNotIn("ST_HasArc(geom) OR GeometryType(geom) IN ('CURVEPOLYGON', 'MULTISURFACE')", queries[0])
        self.assertIn("ST_GeomFromEWKB(decode(hexwkb, 'hex'))", queries[0])
        self.assertIn("ELSE ST_GeomFromEWKT(wkt)", queries[0])
        self.assertNotIn("ST_AsEWKB(result.geom) = ST_AsEWKB(candidate.geom)", queries[0])
        self.assertIn("COALESCE(ST_Z(vertex3d.geom), 0)", queries[0])
        self.assertIn("ST_Z(vertex3d.geom)", queries[0])
        self.assertIn("COALESCE(total_points, ST_NPoints(framed.geom)) AS total_points", queries[0])
        self.assertIn("GeometryType(framed.geom) AS root_type", queries[0])
        self.assertIn("'root_type', root_type", queries[0])
        self.assertIn("'source_point_count', source_point_count", queries[0])
        self.assertIn("'marker_scale', marker_scale", queries[0])
        self.assertIn("root_type IN ('LINESTRING', 'POLYGON', 'TRIANGLE')", queries[0])
        self.assertIn("AND total_points <= 64", queries[0])
        self.assertNotIn("AND ST_NPoints(geom) <= 64", queries[0])
        self.assertNotIn("ST_Scale", queries[0])

    def test_visual_payload_sends_unicode_labels_without_json_escapes(self):
        queries = []
        self.tester.run_psql_scalar = lambda database, query: queries.append(query) or '{"frames":[],"parts":[]}'
        self.tester.visual_payload("manual", [{
            "ord": 1,
            "source": "Output",
            "label": "2 I(a) ∩ I(b)",
            "row_num": 1,
            "column_num": 1,
            "requested_frame": None,
            "wkt": "POINT(0 0)",
            "hexwkb": None,
            "source_point_count": 1,
            "total_points": 1,
            "marker_scale": None,
        }])
        match = re.search(r"decode\('([^']+)', 'base64'\)", queries[0])
        self.assertIsNotNone(match)
        payload = base64.b64decode(match.group(1)).decode("utf-8")
        self.assertIn("∩", payload)
        self.assertNotIn(r"\u2229", payload)

    def test_ordinary_2d_svg_remains_byte_stable(self):
        svg = self.tester.visual_svg(
            "buffer-example",
            {
                "bounds": [0, 0, 10, 10],
                "parts": [
                    {"ord": 1, "source": "Code", "label": "Code 1",
                     "type": "POINT", "x": 2, "y": -3},
                    {"ord": 2, "source": "Output", "label": "Output 1",
                     "type": "POLYGON", "svg": "M 0 0 L 10 0 10 -10 Z",
                     "has_z": False,
                     "points_xyz": xyz_vertices(
                         [[0, 0, 0], [10, 0, 0], [10, 10, 0], [0, 0, 0]], ring=True
                     )},
                ],
            },
        )
        self.assertEqual(
            "ad9f03873ee58dbb970b5fab66598bf146db51f76331217814abeddbff05ae0b",
            hashlib.sha256(svg.encode()).hexdigest(),
        )

    def test_z_surface_faces_are_projected_depth_sorted_shaded_and_opaque(self):
        payload = {
            "bounds": [0, 0, 2, 1],
            "parts": [
                {"ord": 1, "source": "Output", "label": "solid",
                 "root_type": "POLYHEDRALSURFACE", "type": "POLYGON",
                 "svg": "unused",
                 "has_z": True,
                 "points_xyz": xyz_vertices(
                     [[2, 0, 0], [2, 1, 0], [2, 1, 1], [2, 0, 1], [2, 0, 0]], ring=True
                 )},
                {"ord": 1, "source": "Output", "label": "solid",
                 "root_type": "POLYHEDRALSURFACE", "type": "POLYGON",
                 "svg": "unused",
                 "has_z": True,
                 "points_xyz": xyz_vertices(
                     [[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0], [0, 0, 0]], ring=True
                 )},
            ],
        }
        svg = self.tester.visual_svg("solid-3d", payload)
        self.assertEqual(svg, self.tester.visual_svg("solid-3d", payload))
        self.assertIn(">3D view</text>", svg)
        depths = [float(value) for value in re.findall(r'data-postgis-depth="([^"]+)"', svg)]
        self.assertEqual(sorted(depths, reverse=True), depths)
        fills = re.findall(
            r'data-postgis-face="[12]"[^>]+fill="(#[0-9a-f]{6})"', svg
        )
        self.assertEqual(2, len(fills))
        self.assertEqual(2, len(set(fills)))
        face_colors = re.findall(
            r'data-postgis-face="[12]"[^>]+stroke="(#[0-9a-f]{6})" fill="(#[0-9a-f]{6})"',
            svg,
        )
        self.assertEqual(2, len(face_colors))
        self.assertTrue(all(stroke != fill for stroke, fill in face_colors))
        self.assertEqual(2, svg.count('fill-opacity="1"'))
        self.assertNotIn('fill-opacity="0.82"', svg)
        self.assertNotIn('stroke="#dce2e7" stroke-width="1"', svg)

    def test_wide_3d_scene_keeps_source_x_axis_readable(self):
        points = [
            (x, y, z)
            for x in (0, 280)
            for y in (0, 70)
            for z in (0, 10)
        ]
        view = self.tester.choose_3d_view(points)
        self.assertEqual((0.2, -1.5, 1.15), view)
        origin = self.tester.project_3d_point((0, 0, 0), view)
        x_axis = self.tester.project_3d_point((100, 0, 0), view)
        dx = x_axis[0] - origin[0]
        dy = x_axis[1] - origin[1]
        self.assertGreater(dx, 0)
        self.assertLess(abs(dy / dx), 0.1)

    def test_z_surface_draws_dashed_hidden_edges_beneath_opaque_faces(self):
        payload = {
            "bounds": [0, 0, 1, 1],
            "parts": [
                {"ord": 1, "source": "Output", "label": "solid",
                 "root_type": "POLYHEDRALSURFACE", "type": "POLYGON",
                 "svg": "unused", "has_z": True,
                 "points_xyz": xyz_vertices(points, ring=True)}
                for points in (
                    [[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 0, 0]],
                    [[0, 0, 0], [1, 1, 0], [0, 0, 1], [0, 0, 0]],
                )
            ],
        }
        svg = self.tester.visual_svg("hidden-edges", payload)
        self.assertEqual(2, svg.count('class="hidden-edge"'))
        self.assertIn('stroke-dasharray=', svg)
        self.assertLess(svg.index('class="hidden-edge-layer"'), svg.index('class="geometry-layer"'))

    def test_2d_surface_does_not_draw_hidden_edges(self):
        svg = self.tester.visual_svg(
            "flat-area",
            {"bounds": [0, 0, 1, 1], "parts": [{
                "ord": 1, "source": "Output", "label": "area",
                "root_type": "POLYGON", "type": "POLYGON",
                "svg": "M 0 0 L 1 0 L 0 -1 Z", "has_z": False,
                "vertices": [[0, 0], [1, 0], [0, -1], [0, 0]],
            }]},
        )
        self.assertNotIn('class="hidden-edge"', svg)

    def test_tin_faces_do_not_alpha_stack_when_their_projections_overlap(self):
        payload = {
            "bounds": [0, 0, 1, 1],
            "parts": [
                {"ord": 1, "source": "Code", "label": "TIN",
                 "root_type": "TIN", "type": "TRIANGLE", "svg": "unused",
                 "has_z": True, "points_xyz": xyz_vertices(points, ring=True)}
                for points in (
                    [[0, 0, 0], [0, 0, 1], [0, 1, 0], [0, 0, 0]],
                    [[0, 0, 0], [0, 1, 0], [1, 0, 0], [0, 0, 0]],
                    [[0, 0, 0], [1, 0, 0], [0, 0, 1], [0, 0, 0]],
                    [[1, 0, 0], [0, 1, 0], [0, 0, 1], [1, 0, 0]],
                )
            ],
        }
        svg = self.tester.visual_svg("tetrahedron", payload)
        self.assertEqual(4, svg.count('data-postgis-face='))
        self.assertEqual(4, svg.count('fill-opacity="1"'))
        self.assertNotIn('fill-opacity="0.82"', svg)

    def test_mixed_frame_projects_2d_footprint_with_3d_roof(self):
        payload = {
            "frames": [{"id": "Overlay", "bounds": [0, 0, 4, 3]}],
            "parts": [
                {"ord": 1, "source": "Output", "label": "footprint",
                 "root_type": "POLYGON", "type": "POLYGON",
                 "svg": "M 0 0 L 4 0 4 -3 0 -3 Z", "has_z": False,
                 "points_xyz": xyz_vertices(
                     [[0, 0, 0], [4, 0, 0], [4, 3, 0], [0, 3, 0], [0, 0, 0]], ring=True
                 )},
                {"ord": 2, "source": "Output", "label": "roof",
                 "root_type": "POLYHEDRALSURFACE", "type": "POLYGON",
                 "svg": "unused", "has_z": True,
                 "points_xyz": xyz_vertices(
                     [[0, 0, 0], [4, 0, 0], [2, 1.5, 2], [0, 0, 0]], ring=True
                 )},
            ],
        }
        projected = self.tester.project_3d_payload(payload)
        self.assertTrue(projected["frames"][0]["three_dimensional"])
        self.assertTrue(all(part.get("is_3d_face") for part in projected["parts"]))
        self.assertNotEqual(payload["parts"][0]["svg"], projected["parts"][0]["svg"])
        svg = self.tester.visual_svg("mixed-roof", payload)
        self.assertEqual(2, svg.count("data-postgis-face="))
        self.assertNotIn('stroke="#dce2e7" stroke-width="1"', svg)

    def test_mixed_frame_projects_3d_line_with_3d_polygon(self):
        line_points = [[0, 0, 0], [4, 0, 1], [4, 3, 2]]
        payload = {
            "frames": [{"id": "Overlay", "bounds": [0, 0, 4, 3]}],
            "parts": [
                {"ord": 1, "source": "Code", "label": "ring",
                 "root_type": "LINESTRING", "type": "LINESTRING",
                 "svg": "M 0 0 L 4 0 4 -3", "closed": False, "has_z": True,
                 "points_xyz": xyz_vertices(line_points)},
                {"ord": 2, "source": "Output", "label": "polygon",
                 "root_type": "POLYGON", "type": "POLYGON",
                 "svg": "unused", "has_z": True,
                 "points_xyz": xyz_vertices(line_points + [line_points[0]], ring=True)},
            ],
        }
        projected = self.tester.project_3d_payload(payload)
        line = projected["parts"][0]
        self.assertNotEqual(payload["parts"][0]["svg"], line["svg"])
        self.assertFalse(line.get("is_3d_face", False))
        min_x, min_y, max_x, max_y = projected["frames"][0]["bounds"]
        for x, svg_y in line["vertices"]:
            self.assertGreaterEqual(x, min_x)
            self.assertLessEqual(x, max_x)
            self.assertGreaterEqual(-svg_y, min_y)
            self.assertLessEqual(-svg_y, max_y)
        svg = self.tester.visual_svg("makepolygon", payload)
        self.assertIn('data-postgis-geometry-id="makepolygon-code-1"', svg)
        self.assertIn('data-postgis-geometry-id="makepolygon-output-1"', svg)

    def test_separate_frames_share_3d_projection_policy(self):
        ring_points = [[0, 0, 1], [4, 0, 2], [4, 3, 2], [0, 3, 1], [0, 0, 1]]
        payload = {
            "frames": [
                {"id": "Code", "bounds": [0, 0, 4, 3]},
                {"id": "Output", "bounds": [0, 0, 4, 3]},
            ],
            "parts": [
                {"ord": 1, "source": "Code", "label": "ring", "frame": "Code",
                 "root_type": "LINESTRING", "type": "LINESTRING",
                 "svg": "M 0 0 L 4 0 4 -3 0 -3 0 0", "closed": True,
                 "has_z": True, "points_xyz": xyz_vertices(ring_points)},
                {"ord": 2, "source": "Output", "label": "polygon", "frame": "Output",
                 "root_type": "POLYGON", "type": "POLYGON", "svg": "unused",
                 "has_z": True, "points_xyz": xyz_vertices(ring_points, ring=True)},
            ],
        }
        projected = self.tester.project_3d_payload(payload)
        self.assertTrue(all(frame["three_dimensional"] for frame in projected["frames"]))
        self.assertNotEqual(payload["parts"][0]["svg"], projected["parts"][0]["svg"])
        self.assertEqual(
            projected["parts"][0]["svg"] + " Z",
            projected["parts"][1]["svg"],
        )
        svg = self.tester.visual_svg("separate-3d", payload)
        self.assertNotIn('stroke="#dce2e7" stroke-width="1"', svg)
        self.assertIn(">Code (3D view)</text>", svg)
        self.assertIn(">Output (3D view)</text>", svg)

    def test_separate_2d_output_is_not_projected_with_3d_input(self):
        ring_3d = [[0, 0, 2], [0, 5, 2], [5, 0, 2], [0, 0, 2]]
        ring_2d = [[0, 0, 0], [0, 5, 0], [5, 0, 0], [0, 0, 0]]
        payload = {
            "frames": [
                {"id": "Code", "bounds": [0, 0, 5, 5]},
                {"id": "Output", "bounds": [0, 0, 5, 5]},
            ],
            "parts": [
                {"ord": 1, "source": "Code", "label": "input", "frame": "Code",
                 "root_type": "POLYGON", "type": "POLYGON", "svg": "input-2d-placeholder",
                 "has_z": True, "points_xyz": xyz_vertices(ring_3d, ring=True)},
                {"ord": 2, "source": "Output", "label": "output", "frame": "Output",
                 "root_type": "POLYGON", "type": "POLYGON", "svg": "M 0 0 L 0 -5 5 0 Z",
                 "has_z": False, "points_xyz": xyz_vertices(ring_2d, ring=True)},
            ],
        }
        projected = self.tester.project_3d_payload(payload)
        self.assertTrue(projected["frames"][0]["three_dimensional"])
        self.assertNotIn("three_dimensional", projected["frames"][1])
        self.assertNotEqual(payload["parts"][0]["svg"], projected["parts"][0]["svg"])
        self.assertEqual(payload["parts"][1]["svg"], projected["parts"][1]["svg"])
        svg = self.tester.visual_svg("force2d", payload)
        self.assertIn(">Code (3D view)</text>", svg)
        self.assertIn(">Output</text>", svg)
        self.assertNotIn(">Output (3D view)</text>", svg)
        self.assertIn('stroke="#dce2e7" stroke-width="1"', svg)

    def test_3d_frame_projects_zero_z_point_and_line(self):
        payload = {
            "frames": [{"id": "Overlay", "bounds": [0, 0, 3, 2]}],
            "parts": [
                {"ord": 1, "source": "Output", "label": "surface",
                 "root_type": "POLYGON", "type": "POLYGON", "svg": "unused",
                 "has_z": True, "points_xyz": xyz_vertices(
                     [[0, 0, 0], [3, 0, 0], [0, 2, 2], [0, 0, 0]], ring=True
                 )},
                {"ord": 2, "source": "Code", "label": "origin",
                 "root_type": "POINT", "type": "POINT", "x": 2, "y": -1,
                 "has_z": False, "points_xyz": xyz_vertices([[2, 1, 0]])},
                {"ord": 3, "source": "Code", "label": "edge",
                 "root_type": "LINESTRING", "type": "LINESTRING",
                 "svg": "M 0 0 L 3 -2", "closed": False, "has_z": False,
                 "points_xyz": xyz_vertices([[0, 0, 0], [3, 2, 0]])},
            ],
        }
        projected = self.tester.project_3d_payload(payload)
        frame_points = [
            vertex["point"]
            for part in payload["parts"]
            for vertex in (part.get("points_xyz") or [])
        ]
        view = self.tester.choose_3d_view(frame_points)
        expected_x, expected_y, _ = self.tester.project_3d_point([2, 1, 0], view)
        self.assertEqual(expected_x, projected["parts"][1]["x"])
        self.assertEqual(-expected_y, projected["parts"][1]["y"])
        self.assertNotEqual(payload["parts"][2]["svg"], projected["parts"][2]["svg"])
        svg = self.tester.visual_svg("mixed-zero-z", payload)
        self.assertIn('data-postgis-geometry-id="mixed-zero-z-code-1"', svg)
        self.assertIn('data-postgis-geometry-id="mixed-zero-z-code-2"', svg)

    def test_3d_projection_chooses_non_edge_on_view_for_skinny_footprints(self):
        payload = {
            "frames": [{"id": "Overlay", "bounds": [1, 10, 175, 155]}],
            "parts": [
                {"ord": 1, "source": "Code", "label": "poly",
                 "root_type": "POLYGON", "type": "POLYGON", "svg": "unused",
                 "has_z": True, "points_xyz": xyz_vertices(
                     [[175, 150, 5], [20, 40, 5], [35, 45, 5],
                      [50, 60, 5], [100, 100, 5], [175, 150, 5]],
                     ring=True,
                 )},
                {"ord": 2, "source": "Code", "label": "mline",
                 "root_type": "MULTILINESTRING", "type": "LINESTRING",
                 "svg": "unused", "closed": False, "has_z": True,
                 "points_xyz": xyz_vertices(
                     [[175, 155, 2], [20, 40, 20], [50, 60, -2],
                      [125, 100, 1], [175, 155, 1]]
                 )},
                {"ord": 3, "source": "Output", "label": "lol3d",
                 "root_type": "LINESTRING", "type": "LINESTRING",
                 "svg": "unused", "closed": False, "has_z": True,
                 "points_xyz": xyz_vertices([[175, 150, 5], [1, 10, 2]])},
                {"ord": 4, "source": "Output", "label": "lol2d",
                 "root_type": "LINESTRING", "type": "LINESTRING",
                 "svg": "unused", "closed": False, "has_z": False,
                 "points_xyz": xyz_vertices([[175, 150, 0], [1, 10, 0]])},
            ],
        }
        projected = self.tester.project_3d_payload(payload)
        min_x, min_y, max_x, max_y = projected["frames"][0]["bounds"]
        self.assertGreater(max_x - min_x, 0)
        self.assertGreater(max_y - min_y, 0)
        self.assertGreaterEqual(
            min(max_x - min_x, max_y - min_y) / max(max_x - min_x, max_y - min_y),
            0.55,
        )

    def test_3d_projection_preserves_vertical_separation(self):
        ground = self.tester.project_3d_point([4, 7, 0])
        raised = self.tester.project_3d_point([4, 7, 3])
        self.assertEqual(ground[0], raised[0])
        self.assertGreater(raised[1], ground[1])
        self.assertGreater(raised[2], ground[2])

    def test_3d_projection_does_not_collapse_opposite_cube_corners(self):
        origin = self.tester.project_3d_point([0, 0, 0])
        opposite = self.tester.project_3d_point([1, 1, 1])
        self.assertNotEqual(origin[:2], opposite[:2])

    def test_svg_renders_separate_source_frames(self):
        svg = self.tester.visual_svg(
            "comparison",
            {
                "frames": [
                    {"id": "Code", "label": "Code (SRID 2249)", "bounds": [0, 0, 10, 10]},
                    {"id": "Output", "label": "Output (SRID 4326)", "bounds": [0, 0, 10, 10]},
                ],
                "parts": [
                    {"ord": 1, "source": "Code", "label": "Code", "frame": "Code",
                     "type": "LINESTRING", "svg": "M 0 0 L 10 -10", "closed": False},
                    {"ord": 2, "source": "Output", "label": "Output", "frame": "Output",
                     "type": "LINESTRING", "svg": "M 0 0 L 10 -10", "closed": False},
                ],
            },
        )
        self.assertIn('class="frame-label"', svg)
        self.assertIn('>Code (SRID 2249)</text>', svg)
        self.assertIn('>Output (SRID 4326)</text>', svg)
        self.assertIn('data-postgis-frame="Code"', svg)
        self.assertIn('data-postgis-frame="Output"', svg)
        self.assertEqual(2, svg.count('class="plot-background"'))
        self.assertNotIn("font:", svg)

    def test_svg_renders_four_frames_in_two_column_grid(self):
        frames = [
            {"id": label, "label": label, "bounds": [0, 0, 10, 10]}
            for label in ("(a) simple", "(b) not simple", "(c) simple", "(d) not simple")
        ]
        svg = self.tester.visual_svg(
            "four-frames",
            {
                "frames": frames,
                "parts": [
                    {"ord": index, "source": "Code", "label": frame["label"],
                     "frame": frame["id"], "type": "LINESTRING",
                     "svg": "M 0 0 L 10 -10", "closed": False}
                    for index, frame in enumerate(frames, 1)
                ],
            },
        )
        self.assertEqual(4, svg.count('class="plot-background"'))
        self.assertEqual(2, svg.count('y="30" width="270" height="220"'))
        self.assertEqual(2, svg.count('y="270" width="270" height="220"'))
        self.assertIn('x="155" y="20"', svg)
        self.assertIn('x="445" y="260"', svg)
        self.assertIn('height="548" viewBox="0 0 600 548"', svg)

    def test_svg_renders_three_frames_in_one_wide_row(self):
        frames = [
            {"id": label, "label": label, "bounds": [0, 0, 10, 10]}
            for label in ("Code", "linework", "structure")
        ]
        svg = self.tester.visual_svg(
            "three-frames",
            {
                "frames": frames,
                "parts": [
                    {"ord": index, "source": "Output" if index > 1 else "Code",
                     "label": frame["label"], "frame": frame["id"],
                     "type": "LINESTRING", "svg": "M 0 0 L 10 -10", "closed": False}
                    for index, frame in enumerate(frames, 1)
                ],
            },
        )
        self.assertIn('width="900"', svg)
        self.assertIn('viewBox="0 0 900 380"', svg)
        self.assertEqual(3, svg.count('class="plot-background"'))
        self.assertEqual(3, svg.count('y="30" width="273.333333333" height="292"'))
        self.assertIn('>Code</text>', svg)
        self.assertIn('>linework</text>', svg)
        self.assertIn('>structure</text>', svg)

    def test_separate_explicit_inputs_use_their_labels_as_frames(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "frames": [
                {"id": layer["requested_frame"], "label": layer["requested_frame"],
                 "bounds": [0, 0, 1, 1]}
                for layer in layers
            ],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "frame": layer["requested_frame"], "type": "POINT", "x": 0, "y": 0,
            } for layer in layers],
        }
        actual = QueryRows(
            [["POINT(0 0)", "POINT(1 1)"]],
            ["input_(a) simple", "input_(b) not simple"],
        )
        self.tester.render_visual_example("manual", {
            "query": "SELECT 'POINT(9 9)'",
            "visual_id": "labeled-inputs", "label": "labels:labeled-inputs",
            "visual_refentry": "labels", "visual_screen": 3,
            "visual_preferred": True, "visual_kind": "geometry-output",
            "visual_separate_output": True,
        }, actual)
        self.assertEqual(["(a) simple", "(b) not simple"], [
            layer["requested_frame"] for layer in captured
        ])

    def test_svg_uses_one_color_for_polyhedral_surface_faces(self):
        svg = self.tester.visual_svg(
            "solid",
            {"bounds": [0, 0, 2, 1], "parts": [
                {"ord": 1, "source": "Output", "label": "solid",
                 "root_type": "POLYHEDRALSURFACE", "type": "POLYGON",
                 "svg": "M 0 0 L 1 0 0 -1 Z"},
                {"ord": 1, "source": "Output", "label": "solid",
                 "root_type": "POLYHEDRALSURFACE", "type": "POLYGON",
                 "svg": "M 1 0 L 2 0 2 -1 Z"},
            ]},
        )
        self.assertEqual(2, svg.count('stroke="#a62c2b" fill="#a62c2b"'))
        self.assertNotIn('stroke="#d95f3d"', svg)

    def test_svg_draws_bounded_vertices_for_linear_geometries(self):
        svg = self.tester.visual_svg(
            "vertices",
            {"bounds": [0, 0, 1, 1], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "POLYGON",
                "svg": "M 0 0 L 1 0 0 -1 Z",
                "vertices": [[0, 0], [1, 0], [0, -1], [0, 0]],
            }]},
        )
        self.assertEqual(3, svg.count('class="vertex"'))

    def test_svg_can_hide_output_area_vertices_for_visual_clarity(self):
        payload = {"bounds": [0, 0, 1, 1], "parts": [{
            "ord": 1, "source": "Output", "label": "circle", "type": "POLYGON",
            "svg": "M 0 0 L 1 0 0 -1 Z",
            "vertices": [[0, 0], [1, 0], [0, -1], [0, 0]],
        }]}
        default_svg = self.tester.visual_svg("vertices-default", payload)
        hidden_svg = self.tester.visual_svg(
            "vertices-hidden", payload, hide_output_area_vertices=True
        )
        self.assertEqual(3, default_svg.count('class="vertex"'))
        self.assertNotIn('class="vertex"', hidden_svg)

    def test_svg_suppresses_vertices_for_dense_output_areas(self):
        vertices = [[index, index % 2] for index in range(17)]
        vertices.append(vertices[0])
        svg = self.tester.visual_svg(
            "dense-output-area",
            {"bounds": [0, 0, 16, 1], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "POLYGON",
                "svg": "M 0 0 L 1 -1 L 2 0 L 3 -1 L 16 0 Z",
                "vertices": vertices,
            }]},
        )
        self.assertNotIn('class="vertex"', svg)

    def test_svg_keeps_vertices_for_dense_input_areas(self):
        vertices = [[index, index % 2] for index in range(17)]
        vertices.append(vertices[0])
        svg = self.tester.visual_svg(
            "dense-input-area",
            {"bounds": [0, 0, 16, 1], "parts": [{
                "ord": 1, "source": "Code", "label": "Code", "type": "POLYGON",
                "svg": "M 0 0 L 1 -1 L 2 0 L 3 -1 L 16 0 Z",
                "vertices": vertices,
            }]},
        )
        self.assertEqual(17, svg.count('class="vertex"'))

    def test_svg_uses_input_underlay_and_direction_arrows_for_open_lines(self):
        svg = self.tester.visual_svg(
            "directions",
            {"bounds": [0, 0, 2, 1], "parts": [
                {"ord": 1, "source": "Code", "label": "Code", "type": "LINESTRING",
                 "svg": "M 0 0 L 2 -1", "closed": False, "vertices": [[0, 0], [2, -1]]},
                {"ord": 2, "source": "Output", "label": "Output", "type": "LINESTRING",
                 "svg": "M 0 0 L 2 -1", "closed": False, "vertices": [[0, 0], [2, -1]]},
            ]},
        )
        self.assertEqual(2, svg.count('class="direction-arrow"'))
        self.assertIn('fill="none" fill-opacity="0"', svg)
        self.assertRegex(svg, r'class="direction-arrow"[^>]*fill="#2878b8"')
        self.assertRegex(svg, r'class="direction-arrow"[^>]*fill="#a62c2b"')
        widths = re.findall(r'class="line"[^>]*stroke-width="([0-9.]+)"', svg)
        self.assertGreater(float(widths[0]), float(widths[1]))
        self.assertNotIn('class="start-vertex"', svg)

    def test_svg_uses_direction_arrows_for_area_rings_when_requested(self):
        payload = {"bounds": [0, 0, 4, 4], "parts": [{
            "ord": 1, "source": "Code", "label": "Code", "type": "POLYGON",
            "svg": "M 0 0 L 4 0 L 4 -4 L 0 -4 Z M 1 -1 L 1 -3 L 3 -3 L 3 -1 Z",
            "vertices": [
                [0, 0], [4, 0], [4, -4], [0, -4], [0, 0],
                [1, -1], [1, -3], [3, -3], [3, -1], [1, -1],
            ],
            "direction_paths": [
                [[0, 0], [4, 0], [4, -4], [0, -4], [0, 0]],
                [[1, -1], [1, -3], [3, -3], [3, -1], [1, -1]],
            ],
        }]}
        default_svg = self.tester.visual_svg("area-directions", payload)
        directed_svg = self.tester.visual_svg(
            "area-directions", payload, show_direction=True
        )
        self.assertNotIn("ring-direction-arrow", default_svg)
        self.assertEqual(4, directed_svg.count("ring-direction-arrow"))
        self.assertRegex(
            directed_svg,
            r'class="direction-arrow ring-direction-arrow"[^>]*fill="#2878b8"',
        )

    def test_svg_marks_start_vertex_for_closed_lines(self):
        svg = self.tester.visual_svg(
            "scroll",
            {"bounds": [0, 0, 2, 1], "parts": [
                {"ord": 1, "source": "Code", "label": "input", "type": "LINESTRING",
                 "svg": "M 0 0 L 2 0 1 -1 0 0", "closed": True,
                 "vertices": [[0, 0], [2, 0], [1, -1], [0, 0]]},
                {"ord": 2, "source": "Output", "label": "scrolled", "type": "LINESTRING",
                 "svg": "M 1 -1 L 0 0 2 0 1 -1", "closed": True,
                 "vertices": [[1, -1], [0, 0], [2, 0], [1, -1]]},
            ]},
        )
        self.assertEqual(2, svg.count('class="start-vertex"'))
        self.assertIn("<title>Start vertex for input</title>", svg)
        self.assertIn("<title>Start vertex for scrolled</title>", svg)
        self.assertIn('data-postgis-start-vertex="true"', svg)
        self.assertNotIn('class="direction-arrow"', svg)

    def test_svg_points_have_contrasting_halo(self):
        svg = self.tester.visual_svg(
            "point-halo",
            {"bounds": [0, 0, 1, 1], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "POINT",
                "x": 0.5, "y": -0.5,
            }]},
        )
        self.assertRegex(svg, r'class="point"[^>]*stroke="white" stroke-width="[0-9.]+"')

    def test_svg_renders_multipoint_coordinates_as_point_markers(self):
        svg = self.tester.visual_svg(
            "multipoint-markers",
            {"bounds": [0, 0, 10, 10], "parts": [{
                "ord": 1, "source": "Code", "label": "Code", "type": "MULTIPOINT",
                "dimension": 0, "points": [[1, -1], [2, -2], [3, -3]],
                "svg": 'cx="1" cy="-1",cx="2" cy="-2"',
            }]},
        )
        self.assertEqual(3, svg.count('class="point"'))
        self.assertNotIn('d="cx=', svg)

    def test_svg_scales_point_markers_down_for_dense_payloads(self):
        sparse = self.tester.visual_svg(
            "sparse-points",
            {"bounds": [0, 0, 10, 10], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "POINT",
                "x": 5, "y": -5, "total_points": 8, "source_point_count": 8,
            }]},
        )
        dense = self.tester.visual_svg(
            "dense-points",
            {"bounds": [0, 0, 10, 10], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "POINT",
                "x": 5, "y": -5, "total_points": 512, "source_point_count": 512,
            }]},
        )
        self.assertLess(self.point_radius(dense), self.point_radius(sparse))

    def test_svg_keeps_medium_density_code_points_below_four_pixel_radius(self):
        svg = self.tester.visual_svg(
            "delaunay-points",
            {"bounds": [0, 0, 200, 200], "parts": [{
                "ord": 1, "source": "Code", "label": "Code", "type": "POINT",
                "x": 5, "y": -5, "total_points": 42, "source_point_count": 42,
            }]},
        )
        self.assertLess(self.point_radius(svg), 4.0)

    def test_svg_scales_vertices_down_for_dense_payloads(self):
        sparse = self.tester.visual_svg(
            "sparse-vertices",
            {"bounds": [0, 0, 4, 1], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "LINESTRING",
                "svg": "M 0 0 L 4 -1", "closed": False,
                "vertices": [[0, 0], [4, -1]],
                "total_points": 2, "source_point_count": 2,
            }]},
        )
        dense = self.tester.visual_svg(
            "dense-vertices",
            {"bounds": [0, 0, 4, 1], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "LINESTRING",
                "svg": "M 0 0 L 1 -1 L 2 0 L 3 -1 L 4 0", "closed": False,
                "vertices": [[0, 0], [1, -1], [2, 0], [3, -1], [4, 0]],
                "total_points": 512, "source_point_count": 512,
            }]},
        )
        self.assertLess(self.vertex_radius(dense), self.vertex_radius(sparse))

    def test_svg_scales_point_markers_with_marker_scale_but_caps_them(self):
        smaller = self.tester.visual_svg(
            "marker-scale-small",
            {"bounds": [0, 0, 10, 10], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "POINT",
                "x": 5, "y": -5, "marker_scale": 0.5, "total_points": 1, "source_point_count": 1,
            }]},
        )
        larger = self.tester.visual_svg(
            "marker-scale-large",
            {"bounds": [0, 0, 10, 10], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "POINT",
                "x": 5, "y": -5, "marker_scale": 999, "total_points": 1, "source_point_count": 1,
            }]},
        )
        self.assertLess(self.point_radius(smaller), self.point_radius(larger))
        self.assertLess(self.point_radius(larger), self.point_radius(smaller) * 4)

    def test_svg_does_not_arrow_closed_curves(self):
        svg = self.tester.visual_svg(
            "closed-curve",
            {"bounds": [0, 0, 1, 1], "parts": [{
                "ord": 1, "source": "Output", "label": "Output", "type": "CIRCULARSTRING",
                "svg": "M 0 0 C 0 1 1 1 0 0", "closed": True,
            }]},
        )
        self.assertNotIn('class="direction-arrow"', svg)

    def test_render_visual_example_uses_geometry_column_names(self):
        self.tester.visual_payload = lambda database, layers: {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": 0, "y": 0,
            } for layer in layers],
        }
        actual = QueryRows([["POINT(0 0)", "POINT(1 1)"]], ["center", "nearest"])
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT 1", "visual_id": "labels", "label": "labels:1",
            "visual_refentry": "labels", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertIn(">center</text>", visual["svg"])
        self.assertIn(">nearest</text>", visual["svg"])

    def test_render_visual_example_groups_multirow_outputs_by_cluster_column(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 2, 2],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": layer["row_num"] or 0, "y": -(layer["row_num"] or 0),
            } for layer in layers],
        }
        actual = QueryRows(
            [["POINT(0 0)", "A"], ["POINT(1 1)", "A"], ["POINT(2 2)", "B"]],
            ["geom", "cluster"],
        )
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT 1", "visual_id": "clustered", "label": "clustered:1",
            "visual_refentry": "clustered", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertEqual(["cluster A", "cluster A", "cluster B"], [layer["label"] for layer in captured])
        self.assertEqual([1, 1, 2], [layer["ord"] for layer in captured])
        self.assertEqual(2, len(visual["layers"]))
        self.assertEqual(2, visual["svg"].count('class="legend-row"'))

    def test_render_visual_example_uses_text_companion_for_cluster_labels(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 2, 2],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": layer["row_num"] or 0, "y": -(layer["row_num"] or 0),
            } for layer in layers],
        }
        actual = QueryRows(
            [
                ["0", "Melbourne, Sydney", "MULTIPOINT((144.96 -37.81),(151.21 -33.87))"],
                ["1", "Boston, New York", "MULTIPOINT((-71.06 42.36),(-74.01 40.71))"],
            ],
            ["cid", "places", "cluster_geom"],
        )
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT 1", "visual_id": "clustered-places", "label": "clustered:2",
            "visual_refentry": "clustered", "visual_screen": 2,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertEqual(
            ["cid 0: Melbourne, Sydney", "cid 1: Boston, New York"],
            [layer["label"] for layer in captured],
        )
        self.assertIn(">cid 0: Melbourne, Sydney</text>", visual["svg"])

    def test_render_visual_example_reads_marker_scale_companion_column(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": 0, "y": 0, "marker_scale": layer["marker_scale"],
            } for layer in layers],
        }
        actual = QueryRows([["POINT(0 0)", "3.5"]], ["geom", "marker_scale"])
        self.tester.render_visual_example("manual", {
            "query": "SELECT 1", "visual_id": "scaled", "label": "scaled:1",
            "visual_refentry": "scaled", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertEqual(3.5, captured[0]["marker_scale"])

    def test_render_visual_example_defers_geometry_point_counts_to_postgis(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "MULTIPOINT", "svg": "M 0 0",
            } for layer in layers],
        }
        actual = QueryRows([["MULTIPOINT((0 0),(1 1))"]], ["points"])
        self.tester.render_visual_example("manual", {
            "query": "SELECT 'MULTIPOINT((0 0),(1 1))'::geometry",
            "visual_id": "point-counts", "label": "point-counts:1",
            "visual_refentry": "point-counts", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertTrue(captured)
        self.assertTrue(all(layer["total_points"] is None for layer in captured))
        self.assertTrue(all(layer["source_point_count"] == 1 for layer in captured))

    def test_render_visual_example_uses_typed_hexwkb_without_text_roundtrip(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POLYGON", "svg": "M 0 0 L 1 0 L 0 -1 Z",
            } for layer in layers],
        }
        actual = QueryRows(
            [["010300000000000000"]],
            ["st_polygonfromtext"],
            ["geometry"],
        )
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT ST_PolygonFromText('POLYGON EMPTY')",
            "visual_id": "hexwkb", "label": "ST_PolygonFromText:1",
            "visual_refentry": "ST_PolygonFromText", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertEqual("010300000000000000", captured[-1]["hexwkb"])
        self.assertIsNone(captured[-1]["wkt"])
        self.assertEqual("Output", captured[-1]["label"])
        self.assertIn(">Output</text>", visual["svg"])

    def test_native_geometry_visual_exposes_raw_hex_alongside_readable_ewkt(self):
        self.tester.visual_payload = lambda database, layers: {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": 1, "y": 2,
            } for layer in layers],
        }
        hexwkb = "0101000000000000000000F03F0000000000000040"
        actual = QueryRows([[hexwkb]], ["point"], ["geometry"])
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT 'POINT(1 2)'::geometry",
            "expected": [["POINT(1 2)"]],
            "visual_id": "native-output", "label": "native-output:1",
            "visual_refentry": "native-output", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertEqual(hexwkb, visual["native_output"])

    def test_render_visual_example_skips_provisional_hex_text(self):
        hexwkb = "010300000000000000"
        actual = QueryRows([[hexwkb]], ["payload"], ["text"])
        visual = self.tester.render_visual_example("manual", {
            "query": f"SELECT '{hexwkb}'::text",
            "expected": [[hexwkb]],
            "visual_id": "hex-text", "label": "hex-text:1",
            "visual_refentry": "hex-text", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertIsNone(visual)

    def test_render_visual_example_skips_empty_only_geometry(self):
        self.tester.visual_payload = lambda database, layers: {
            "bounds": None, "frames": None, "parts": None,
        }
        hexwkb = "010300000000000000"
        actual = QueryRows([[hexwkb]], ["empty_polygon"], ["geometry"])
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT 'POLYGON EMPTY'::geometry",
            "expected": [[hexwkb]],
            "visual_id": "empty", "label": "empty:1",
            "visual_refentry": "empty", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertIsNone(visual)

    def test_render_visual_example_exposes_text_when_empty_output_leaves_input_context(self):
        def payload(database, layers):
            code = next(layer for layer in layers if layer["source"] == "Code")
            return {
                "bounds": [0, 0, 1, 1],
                "parts": [{
                    "ord": code["ord"], "source": "Code", "label": code["label"],
                    "type": "LINESTRING", "svg": "M 0 0 L 1 -1",
                }],
            }

        self.tester.visual_payload = payload
        hexwkb = "010300000000000000"
        actual = QueryRows([[hexwkb]], ["empty_result"], ["geometry"])
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT f('LINESTRING(0 0,1 1)'::geometry)",
            "expected": [[hexwkb]],
            "visual_id": "empty-result", "label": "empty-result:1",
            "visual_refentry": "empty-result", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertTrue(visual["output_omitted"])
        self.assertFalse(visual["preferred"])
        self.assertEqual("input-context-fallback", visual["kind"])
        self.assertEqual(["Code"], [layer["source"] for layer in visual["layers"]])

    def test_explicit_input_layers_replace_inferred_code_context(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": 0, "y": 0,
            } for layer in layers],
        }
        actual = QueryRows([[
            "POINT(0 0)", "LINESTRING(0 0,1 1)", "POLYGON((0 0,1 0,0 1,0 0))",
        ]], ["input_point", "input_line", "result"])

        self.tester.render_visual_example("manual", {
            "query": "SELECT 'POINT(0 0)', 'LINESTRING(0 0,1 1)'",
            "visual_id": "complete-layers", "label": "labels:complete",
            "visual_refentry": "labels", "visual_screen": 2,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)

        self.assertEqual(["Code", "Code", "Output"], [layer["source"] for layer in captured])
        self.assertEqual(["point", "line", "result"], [layer["label"] for layer in captured])

    def test_named_outputs_retain_inferred_code_context(self):
        captured = []
        self.tester.visual_payload = lambda database, layers: captured.extend(layers) or {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "POINT", "x": 0, "y": 0,
            } for layer in layers],
        }
        actual = QueryRows(
            [["POINT(160 40)", "POINT(125.75 115.34)"]],
            ["cp_pt_line", "cp_line_pt"],
        )

        self.tester.render_visual_example("manual", {
            "query": (
                "SELECT ST_ClosestPoint(pt, line), ST_ClosestPoint(line, pt) "
                "FROM (SELECT 'POINT (160 40)'::geometry AS pt, "
                "'LINESTRING (10 30,50 50,30 110,70 90,180 140,130 190)'::geometry AS line) t"
            ),
            "visual_id": "closest-point", "label": "ST_ClosestPoint:1",
            "visual_refentry": "ST_ClosestPoint", "visual_screen": 1,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)

        self.assertEqual(["Code", "Code", "Output", "Output"], [
            layer["source"] for layer in captured
        ])
        self.assertEqual(["pt", "line", "cp pt line", "cp line pt"], [
            layer["label"] for layer in captured
        ])

    def test_render_visual_example_hides_single_output_column_header(self):
        self.tester.visual_payload = lambda database, layers: {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "LINESTRING", "svg": "M 0 0 L 1 -1", "closed": False,
            } for layer in layers],
        }
        actual = QueryRows([["LINESTRING(0 0,1 1)"]], ["polysnapped"])
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT 1", "visual_id": "default-label", "label": "labels:2",
            "visual_refentry": "labels", "visual_screen": 2,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertIn(">Output</text>", visual["svg"])
        self.assertNotIn(">polysnapped</text>", visual["svg"])

    def test_render_visual_example_keeps_single_output_alias_in_comparison(self):
        self.tester.visual_payload = lambda database, layers: {
            "bounds": [0, 0, 1, 1],
            "parts": [{
                "ord": layer["ord"], "source": layer["source"], "label": layer["label"],
                "type": "LINESTRING", "svg": "M 0 0 L 1 -1", "closed": False,
            } for layer in layers],
        }
        actual = QueryRows([["LINESTRING(0 0,1 1)"]], ["distance_segment"])
        visual = self.tester.render_visual_example("manual", {
            "query": "SELECT 'LINESTRING(0 0,1 1)'::geometry AS input",
            "visual_id": "comparison-label", "label": "labels:3",
            "visual_refentry": "labels", "visual_screen": 3,
            "visual_preferred": True, "visual_kind": "geometry-output",
        }, actual)
        self.assertIn(">input</text>", visual["svg"])
        self.assertIn(">distance segment</text>", visual["svg"])

    def test_svg_distinguishes_parts_of_multipart_areas(self):
        svg = self.tester.visual_svg(
            "triangulation",
            {"bounds": [0, 0, 2, 1], "parts": [
                {"ord": 1, "source": "Output", "label": "Output", "type": "POLYGON",
                 "svg": "M 0 0 L 1 0 0 -1 Z"},
                {"ord": 1, "source": "Output", "label": "Output", "type": "POLYGON",
                 "svg": "M 1 0 L 2 0 2 -1 Z"},
            ]},
        )
        self.assertIn('stroke="#a62c2b" fill="#a62c2b"', svg)
        self.assertIn('stroke="#d95f3d" fill="#d95f3d"', svg)

    def test_svg_draws_lines_above_area_fills(self):
        svg = self.tester.visual_svg(
            "line-over-area",
            {"bounds": [0, 0, 2, 1], "parts": [
                {"ord": 1, "source": "Code", "label": "observer", "type": "LINESTRING",
                 "svg": "M 0 0 L 2 0", "closed": False},
                {"ord": 2, "source": "Output", "label": "visibility", "type": "POLYGON",
                 "svg": "M 0 0 L 2 0 1 -1 Z"},
            ]},
        )
        self.assertLess(
            svg.index('data-postgis-geometry-id="line-over-area-output-1"'),
            svg.index('data-postgis-geometry-id="line-over-area-code-1"'),
        )

    def test_svg_rejects_empty_non_point_paths(self):
        with self.assertRaisesRegex(RuntimeError, "empty SVG path"):
            self.tester.visual_svg(
                "broken-curve",
                {"bounds": [0, 0, 1, 1], "parts": [
                    {"ord": 1, "source": "Output", "label": "Output", "type": "MULTICURVE", "svg": ""}
                ]},
            )

    def test_svg_rejects_negative_arc_radii(self):
        with self.assertRaisesRegex(RuntimeError, "invalid SVG path"):
            self.tester.visual_svg(
                "bad-arc",
                {"bounds": [0, 0, 1, 1], "parts": [{
                    "ord": 1, "source": "Output", "label": "Output",
                    "type": "CIRCULARSTRING", "svg": "M 0 0 A -1 -1 0 0 0 1 -1",
                    "closed": False,
                }]},
            )

    def test_grid_is_bounded_for_large_coordinate_offsets(self):
        svg = self.tester.visual_svg(
            "large-offset",
            {"bounds": [1e16, 1e16, 1e16 + 2, 1e16 + 2], "parts": [
                {"ord": 1, "source": "Code", "label": "Code", "type": "POINT", "x": 1e16, "y": -1e16}
            ]},
        )
        self.assertLessEqual(svg.count("<line "), 64)

    def test_multi_frame_3d_payload_uses_shared_projected_bounds(self):
        def triangle_part(ord_, label, frame, size):
            points = [
                [0, 0, 0],
                [size, 0, 0],
                [0, size, 0],
                [0, 0, size],
            ]
            return {
                "ord": ord_,
                "source": "Output",
                "label": label,
                "frame": frame,
                "type": "POLYHEDRALSURFACE",
                "has_z": True,
                "points_xyz": [
                    {"path": [face, index], "point": point}
                    for face, ring in enumerate((
                        [points[0], points[1], points[2], points[0]],
                        [points[0], points[1], points[3], points[0]],
                        [points[0], points[2], points[3], points[0]],
                        [points[1], points[2], points[3], points[1]],
                    ), 1)
                    for index, point in enumerate(ring, 1)
                ],
            }

        projected = self.tester.project_3d_payload({
            "frames": [
                {"id": "outer", "bounds": [0, 0, 4, 4]},
                {"id": "inner", "bounds": [0, 0, 2, 2]},
            ],
            "parts": [
                triangle_part(1, "outer", "outer", 4),
                triangle_part(2, "inner", "inner", 2),
            ],
        })
        self.assertEqual(
            projected["frames"][0]["bounds"],
            projected["frames"][1]["bounds"],
        )

    def test_unparseable_documented_output_falls_back_to_input_context(self):
        calls = []
        def payload(_database, layers):
            calls.append(layers)
            if any(layer["source"] == "Output" for layer in layers):
                raise RuntimeError("geometry contains non-closed rings; parse error")
            return {"bounds": [0, 0, 1, 1], "parts": [
                {"ord": 1, "source": "Code", "label": "Code", "type": "LINESTRING", "svg": "M 0 0 L 1 -1"}
            ]}
        self.tester.visual_payload = payload
        visual = self.tester.render_visual_example("db", {
            "query": "SELECT ST_AsText(f('LINESTRING(0 0,1 1)'))",
            "label": "example:1", "visual_id": "example", "visual_refentry": "example",
            "visual_screen": 1, "visual_preferred": True, "visual_kind": "geometry-output",
        }, [["POLYGON((0 0,1 0,1 1))"]])
        self.assertEqual(2, len(calls))
        self.assertTrue(visual["output_omitted"])
        self.assertFalse(visual["preferred"])
        self.assertEqual("input-context-fallback", visual["kind"])

    def test_publish_visual_examples_replaces_stale_assets(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / "visual-examples"
            target.mkdir()
            (target / "stale.svg").write_text("stale", encoding="utf-8")
            self.tester.publish_visual_examples(
                target,
                [{
                    "id": "one", "source": "ST_Buffer:1", "svg": "<svg/>\n",
                    "kind": "geometry-output", "preferred": True,
                    "refentry": "ST_Buffer", "screen": 1, "output_omitted": False,
                    "native_output": "01010000000000000000000000000000000000000000",
                    "layers": [{
                        "id": "one-code-1", "source": "Code", "label": "center",
                        "row": None, "column": None, "dimension": 0,
                        "closed": None, "srid": 0, "frame": "Overlay",
                    }],
                }],
            )
            self.assertFalse((target / "stale.svg").exists())
            self.assertEqual("<svg/>\n", (target / "one.svg").read_text(encoding="utf-8"))
            self.assertEqual(
                [{
                    "id": "one", "kind": "geometry-output", "preferred": True,
                    "refentry": "ST_Buffer", "screen": 1, "source": "ST_Buffer:1",
                    "output_omitted": False,
                    "native_output": "01010000000000000000000000000000000000000000",
                    "layers": [{
                        "id": "one-code-1", "source": "Code", "label": "center",
                        "row": None, "column": None, "dimension": 0,
                        "closed": None, "srid": 0, "frame": "Overlay",
                    }],
                }],
                json.loads((target / "manifest.json").read_text(encoding="utf-8")),
            )
            manifest_xml = (target / "manifest.xml").read_text(encoding="utf-8")
            self.assertIn('id="one"', manifest_xml)
            self.assertIn('preferred="true"', manifest_xml)
            self.assertIn('<native-output format="hexewkb">01010000', manifest_xml)
            self.assertIn('<layer id="one-code-1" source="Code" label="center"', manifest_xml)
            self.assertIn('dimension="0" srid="0" frame="Overlay"', manifest_xml)


if __name__ == "__main__":
    unittest.main()
