#!/usr/bin/env python3

import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

from postgis_exampletest import ExampleTester, parse_source_example


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


class VisualExampleTest(unittest.TestCase):
    def setUp(self):
        self.tester = ExampleTester.__new__(ExampleTester)

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
        self.assertRegex(svg, r'<circle class="point" cx="2" cy="-3" r="[0-9.]+" fill="#2878b8"/>')
        self.assertIn(
            'd="M 0 0 L 10 0 10 -10 Z" stroke="#a62c2b" fill="#a62c2b"',
            svg,
        )
        self.assertIn('<rect width="11" height="11" rx="2" fill="#2878b8"/>', svg)
        self.assertIn('<rect width="11" height="11" rx="2" fill="#a62c2b"/>', svg)

    def test_visual_candidate_includes_spatial_context_but_skips_trivial_points(self):
        self.assertIsNone(self.tester.visual_candidate("SELECT 'POINT(1 2)'", [["1"]]))
        context = self.tester.visual_candidate(
            "SELECT ST_Intersects('LINESTRING(0 0,1 1)', 'POLYGON((0 0,2 0,2 2,0 0))')",
            [["t"]],
        )
        self.assertEqual({"kind": "input-context", "preferred": False}, context)
        output = self.tester.visual_candidate(
            "SELECT ST_Buffer('LINESTRING(0 0,1 1)', 1)",
            [["POLYGON((0 0,2 0,2 2,0 0))"]],
        )
        self.assertEqual({"kind": "geometry-output", "preferred": True}, output)
        self.assertIsNone(self.tester.visual_candidate(
            "SELECT ST_AsText('LINESTRING(0 0,1 1)')", [["LINESTRING(0 0,1 1)"]]
        ))

    def test_code_geometry_candidates_include_view_envelopes_in_sql_order(self):
        candidates = self.tester.code_geometry_candidates(
            "SELECT f('LINESTRING(0 0,10 10)', ST_MakeEnvelope(2, 3, 8, 9, 4326))"
        )
        self.assertEqual([None, "Envelope"], [candidate.get("label") for candidate in candidates])
        self.assertEqual("LINESTRING(0 0,10 10)", candidates[0]["wkt"])
        self.assertEqual(
            "SRID=4326;POLYGON((2 3,8 3,8 9,2 9,2 3))",
            candidates[1]["wkt"],
        )

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

    def test_svg_rejects_empty_non_point_paths(self):
        with self.assertRaisesRegex(RuntimeError, "empty SVG path"):
            self.tester.visual_svg(
                "broken-curve",
                {"bounds": [0, 0, 1, 1], "parts": [
                    {"ord": 1, "source": "Output", "label": "Output", "type": "MULTICURVE", "svg": ""}
                ]},
            )

    def test_grid_is_bounded_for_large_coordinate_offsets(self):
        svg = self.tester.visual_svg(
            "large-offset",
            {"bounds": [1e16, 1e16, 1e16 + 2, 1e16 + 2], "parts": [
                {"ord": 1, "source": "Code", "label": "Code", "type": "POINT", "x": 1e16, "y": -1e16}
            ]},
        )
        self.assertLessEqual(svg.count("<line "), 64)

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
                }],
            )
            self.assertFalse((target / "stale.svg").exists())
            self.assertEqual("<svg/>\n", (target / "one.svg").read_text(encoding="utf-8"))
            self.assertEqual(
                [{
                    "id": "one", "kind": "geometry-output", "preferred": True,
                    "refentry": "ST_Buffer", "screen": 1, "source": "ST_Buffer:1",
                    "output_omitted": False,
                }],
                json.loads((target / "manifest.json").read_text(encoding="utf-8")),
            )
            manifest_xml = (target / "manifest.xml").read_text(encoding="utf-8")
            self.assertIn('id="one"', manifest_xml)
            self.assertIn('preferred="true"', manifest_xml)


if __name__ == "__main__":
    unittest.main()
