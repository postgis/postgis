#!/usr/bin/env python3

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


if __name__ == "__main__":
    unittest.main()
