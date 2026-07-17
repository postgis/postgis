#!/usr/bin/env python3

from __future__ import annotations

import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

from docbook_qa import (
    SQL_BLOCK_STARTER_KEYWORDS,
    SQL_BLOCK_STARTER_EXTRAS,
    SQL_HIGHLIGHT_KEYWORDS,
    SQL_STARTERS,
    SQL_STATEMENT_START_KEYWORDS,
    add_source_findings,
    build_index,
    check_html_file,
    load_xml,
    main,
)

DOCBOOK_OPEN = '<book xmlns="http://docbook.org/ns/docbook" xmlns:xml="http://www.w3.org/XML/1998/namespace">'
DOCBOOK_CLOSE = '</book>'


def write_tmp(suffix: str, text: str) -> Path:
    path = Path(tempfile.mkdtemp()) / f"fixture{suffix}"
    path.write_text(text, encoding="utf-8")
    return path


class DocBookSourceLintTest(unittest.TestCase):
    def source_findings(self, body: str):
        path = write_tmp(".xml", DOCBOOK_OPEN + body + DOCBOOK_CLOSE)
        return add_source_findings(load_xml(path).getroot(), path)

    def assertCategories(self, body: str, categories: set[str]):
        self.assertEqual(categories, {finding.category for finding in self.source_findings(body)})

    def assertInfoCategory(self, body: str, category: str):
        findings = self.source_findings(body)
        self.assertEqual([(category, "info")], [(finding.category, finding.severity) for finding in findings])

    def test_mixed_programlisting_output_violation_and_clean_case(self):
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>SELECT 1;\n ?column?\n----------\n 1\n(1 row)</programlisting></refentry>',
            {"mixed-programlisting-output"},
        )
        self.assertCategories('<refentry xml:id="f"><programlisting>SELECT 1;</programlisting></refentry>', set())

    def test_mixed_programlisting_dash_run_markers_and_sql_comments(self):
        for output in ("left | right\n----|----\n1 | 2", "----RESULT output ---\n1"):
            with self.subTest(output=output):
                self.assertCategories(
                    f'<refentry xml:id="f"><programlisting>SELECT 1;\n{output}</programlisting></refentry>',
                    {"mixed-programlisting-output"},
                )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>-- first comment\n-- second caption line\nSELECT 1;\n-- internal comment</programlisting></refentry>',
            {"leading-sql-comment"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>---- decorative preface\nSELECT 1;</programlisting></refentry>',
            {"mixed-programlisting-output"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>SELECT 1;\n-- explain the next step\nSELECT 2;</programlisting></refentry>',
            set(),
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>-- Manifest.txt --\nAssembly-Version: 1.0</programlisting></refentry>',
            set(),
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>-- export this file\n\\lo_export 42 /tmp/result.png</programlisting></refentry>',
            {"leading-sql-comment"},
        )

    def test_screen_contains_sql_violation_and_clean_case(self):
        self.assertCategories('<refentry xml:id="f"><screen>SELECT 1;</screen></refentry>', {"screen-contains-sql"})
        self.assertCategories('<refentry xml:id="f"><screen>POINT(1 2)</screen></refentry>', set())

    def test_screen_contains_psql_command_violation_and_clean_cases(self):
        for command in (r"\d roads", r"  \pset tuples_only on", r"\?", r"\! echo ready"):
            with self.subTest(command=command):
                self.assertCategories(
                    f'<refentry xml:id="f"><screen>output\n{command}\nmore output</screen></refentry>',
                    {"screen-contains-psql-command"},
                )
        self.assertCategories(
            r'<refentry xml:id="f"><programlisting>\d roads</programlisting><screen>Table "roads"</screen></refentry>',
            set(),
        )
        self.assertCategories(
            r'<refentry xml:id="f"><screen>C:\temp\roads.sql</screen></refentry>',
            set(),
        )
        self.assertCategories(
            r'<refentry xml:id="f"><screen>\x0103000020e6100000</screen></refentry>',
            set(),
        )

    def test_substantive_raster_image_violation_and_branding_allowlist(self):
        self.assertCategories(
            '<mediaobject><imageobject><imagedata fileref="images/example.png"/>'
            '</imageobject></mediaobject>',
            {"legacy-raster-image"},
        )
        self.assertCategories(
            '<mediaobject><imageobject><imagedata fileref="images/PostGIS_logo.png"/>'
            '</imageobject></mediaobject>',
            set(),
        )
        self.assertCategories(
            '<mediaobject><imageobject><imagedata fileref="images/matrix_sfcgal_enhanced.png"/>'
            '</imageobject></mediaobject>',
            set(),
        )
        self.assertCategories(
            '<mediaobject><imageobject><imagedata fileref="images/st_clusterkmeans03.png"/>'
            '</imageobject></mediaobject>',
            set(),
        )
        self.assertCategories(
            '<mediaobject><imageobject><imagedata fileref="images/example.svg"/>'
            '</imageobject></mediaobject>',
            set(),
        )

    def test_redundant_output_caption_violation_and_real_prose_clean_case(self):
        self.assertCategories(
            '<refentry xml:id="f"><para> The output: </para><!-- separator --><screen>1</screen></refentry>',
            {"redundant-output-caption"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><para>The output preserves the input SRID.</para><screen>1</screen></refentry>',
            set(),
        )

    def test_duplicate_refsection_title_violation_and_clean_case(self):
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Description</title></refsection>'
            '<refsection><title>Description</title></refsection></refentry>',
            {"duplicate-refsection-title"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Description</title></refsection>'
            '<refsection><title>Examples</title></refsection></refentry>',
            set(),
        )

    def test_repeated_example_section_variants_violation_and_single_section_clean_case(self):
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Example: Geometry</title></refsection>'
            '<refsection><title>Examples - Geography</title></refsection></refentry>',
            {"repeated-example-sections"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Examples</title>'
            '<para>Geometry.</para><programlisting>SELECT 1;</programlisting><screen>1</screen>'
            '<para>Geography.</para><programlisting>SELECT 2;</programlisting><screen>2</screen>'
            '</refsection></refentry>',
            set(),
        )

    def test_metadata_role_in_admonition_violation_and_clean_case(self):
        self.assertCategories(
            '<refentry xml:id="f"><note><para role="availability">Availability: 3.7</para></note></refentry>',
            {"metadata-role-in-admonition"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Description</title>'
            '<para role="availability">Availability: 3.7</para></refsection></refentry>',
            set(),
        )

    def test_support_only_description_violation_and_semantic_prose_clean_case(self):
        badge = (
            '<para><inlinemediaobject><imageobject/></inlinemediaobject>'
            'This function supports 3d.</para>'
        )
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Description</title>'
            '<para role="availability">Availability: 3.7</para>' + badge + '</refsection></refentry>',
            {"support-only-description"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Description</title>'
            '<para>Converts curves to linear geometry.</para>' + badge + '</refsection></refentry>',
            set(),
        )

    def test_documented_reference_requires_description_section(self):
        entry = (
            '<refentry xml:id="ST_Foo"><refnamediv><refname>ST_Foo</refname>'
            '<refpurpose>Returns a value.</refpurpose></refnamediv>'
            '<refsynopsisdiv><funcsynopsis><funcprototype><funcdef>integer '
            '<function>ST_Foo</function></funcdef></funcprototype></funcsynopsis></refsynopsisdiv>'
        )
        self.assertCategories(entry + '</refentry>', {"missing-description"})
        self.assertCategories(
            entry + '<refsection><title>Description</title><para>Returns one.</para></refsection></refentry>',
            set(),
        )

    def test_tab_in_verbatim_violation_and_clean_case(self):
        self.assertInfoCategory('<refentry xml:id="f"><programlisting>SELECT\t1;</programlisting></refentry>', "tab-in-verbatim")
        self.assertCategories('<refentry xml:id="f"><screen>1    2</screen></refentry>', set())

    def test_admonition_must_not_contain_code_or_output_blocks(self):
        self.assertCategories(
            '<note><para>Run this command:</para><programlisting>make check</programlisting></note>',
            {"admonition-contains-verbatim"},
        )
        self.assertCategories(
            '<note><para>Run this command:</para></note><programlisting>make check</programlisting>',
            set(),
        )

    def test_wide_verbatim_line_violation_and_recommendation(self):
        long_sql = (
            "SELECT col_a, col_b, col_c, col_d, col_e, col_f, col_g, col_h, col_i, col_j, col_k, "
            "col_l, col_m, col_n FROM huge_schema.huge_table_with_very_wide_columns WHERE id > 0;"
        )
        findings = self.source_findings(f'<refentry xml:id="f"><programlisting>{long_sql}</programlisting></refentry>')
        self.assertEqual({"wide-verbatim-line"}, {finding.category for finding in findings})
        self.assertTrue(any("wrap" in finding.message for finding in findings))

    def test_wide_verbatim_table_row_in_screen_recommends_psql_expanded_output(self):
        table_row = (
            " id | name | description | created_at | updated_at | extra_comment | status_comment | geometry_wkt | source_id | user_name | note | flag | tag\n"
            "----+------+-------------+------------+------------+---------------+---------------+------------+----------+----------+------|----+------+"
        )
        findings = self.source_findings(f'<refentry xml:id="f"><screen>{table_row}</screen></refentry>')
        self.assertEqual({"wide-output-table"}, {finding.category for finding in findings})
        self.assertEqual({"info"}, {finding.severity for finding in findings})
        self.assertIn("2 wide line(s)", findings[0].message)
        self.assertTrue(any("psql expanded output" in finding.message for finding in findings))

    def test_ragged_scalar_output_indent_violation_and_clean_cases(self):
        self.assertCategories(
            '<refentry xml:id="f"><screen> 10156\n10157\n10158</screen></refentry>',
            {"ragged-scalar-output-indent"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><screen>10156\n10157\n10158</screen></refentry>',
            set(),
        )
        self.assertCategories(
            '<refentry xml:id="f"><screen> t\n(1 row)</screen></refentry>',
            set(),
        )
        self.assertCategories(
            '<refentry xml:id="f"><screen> name | value\n------+------\n a    | b</screen></refentry>',
            set(),
        )
        self.assertCategories(
            '<refentry xml:id="f"><screen>                    geography\n'
            '----------------------------------------------------\n'
            ' 0101000020AD1000000000000000C05EC00000000000004140</screen></refentry>',
            set(),
        )
        self.assertCategories(
            '<refentry xml:id="f"><screen> LINESTRING(0 0,\n1 1)</screen></refentry>',
            set(),
        )

    def test_psql_table_at_manual_width_is_not_forced_to_expanded_output(self):
        table_row = (
            " digits |                   encode                   |                  st_astext                   \n"
            "--------+--------------------------------------------+----------------------------------------------\n"
            "     13 | 01010000005e9a72083cdd5e405e9a72083cdd5e40 | POINT(123.45678912345599 123.45678912345599)"
        )
        self.assertCategories(f'<refentry xml:id="f"><screen>{table_row}</screen></refentry>', set())

    def test_misaligned_psql_table_violation_and_unicode_clean_case(self):
        findings = self.source_findings(
            '<refentry xml:id="f"><screen>            left | right\n'
            '-----+------\n'
            '1 |      2</screen></refentry>',
        )
        self.assertEqual({"misaligned-psql-table"}, {finding.category for finding in findings})
        self.assertEqual({"info"}, {finding.severity for finding in findings})
        self.assertCategories(
            '<refentry xml:id="f"><screen>left │ right\n'
            '─────┼──────\n'
            '1    │ 2</screen></refentry>',
            set(),
        )

    def test_wide_verbatim_line_negative_payload_and_opt_out(self):
        payload = "\\x" + "0123456789abcdef" * 10
        self.assertCategories(
            f'<refentry xml:id="f"><programlisting>{payload}</programlisting></refentry>',
            set(),
        )
        self.assertCategories(
            f'<refentry xml:id="f"><programlisting role="docbook-qa-ignore-wide-lines">{payload * 2}</programlisting></refentry>',
            set(),
        )

    def test_sql_staircase_indent_violation_and_opt_out(self):
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>SELECT x\n'
            'FROM (SELECT y\n'
            '                FROM\n'
            '                table_name) AS q;</programlisting></refentry>',
            {"sql-staircase-indent"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>SELECT gps.track_id, ST_MakeLine(gps.geom) AS geom\n'
            '    FROM ( SELECT track_id, gps_time, geom\n'
            '            FROM gps_points ORDER BY track_id, gps_time ) AS gps\n'
            '    GROUP BY track_id;</programlisting></refentry>',
            {"sql-staircase-indent"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>SELECT p.gid,\n'
            '    CASE WHEN EXISTS( SELECT w.geom\n'
            '        FROM waterlines w\n'
            '        WHERE ST_Within(w.geom, p.geom))\n'
            '    THEN ARRAY( SELECT w.geom\n'
            '        FROM waterlines w)\n'
            '    END AS geom\n'
            'FROM provinces p;</programlisting></refentry>',
            {"sql-staircase-indent"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>SELECT COUNT(*), squares.geom\n'
            'FROM\n'
            'pointtable AS pts\n'
            'INNER JOIN\n'
            'ST_SquareGrid(1000, pts.geom) AS squares\n'
            'ON ST_Intersects(pts.geom, squares.geom)</programlisting></refentry>',
            {"sql-staircase-indent"},
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>SELECT x\n'
            'FROM (\n'
            '  SELECT y\n'
            '  FROM table_name\n'
            ') AS q;</programlisting></refentry>',
            set(),
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting role="docbook-qa-ignore-sql-indent">SELECT x\n'
            '                FROM\n'
            '                table_name;</programlisting></refentry>',
            set(),
        )

    def test_stacked_admonitions_violation_and_clean_case(self):
        self.assertInfoCategory(
            '<refentry xml:id="f"><note><para>First</para></note><!-- gap --><tip><para>Second</para></tip></refentry>',
            "stacked-admonitions",
        )
        self.assertCategories(
            '<refentry xml:id="f"><note><para>First</para></note><para>Bridge</para><tip><para>Second</para></tip></refentry>',
            set(),
        )

    def test_ancient_version_note_violation_and_clean_case(self):
        self.assertInfoCategory(
            '<refentry xml:id="f"><warning><para>Changed in PostGIS 1.5.3.</para></warning></refentry>',
            "ancient-version-note",
        )
        self.assertCategories(
            '<refentry xml:id="f"><warning><para>Changed in PostGIS 2.0.</para></warning></refentry>',
            set(),
        )

    def test_example_missing_output_violation_and_clean_case(self):
        self.assertInfoCategory(
            '<refentry xml:id="f"><refsection><title>Standard Examples</title>'
            '<programlisting>SELECT 1;</programlisting><para>No output</para></refsection></refentry>',
            "example-missing-output",
        )
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Examples</title>'
            '<programlisting>SELECT 1;</programlisting><!-- gap --><screen>1</screen></refsection></refentry>',
            set(),
        )

    def test_current_function_in_examples_for_functions_and_operators(self):
        self.assertCategories(
            '<refentry xml:id="ST_Foo"><refnamediv><refname>ST_Foo</refname></refnamediv>'
            '<refsynopsisdiv><funcsynopsis><funcprototype><funcdef><type>integer</type> '
            '<function>ST_Foo</function></funcdef></funcprototype></funcsynopsis></refsynopsisdiv>'
            '<refsection><title>Examples</title><programlisting>SELECT ST_Foo(1);</programlisting>'
            '<screen>1</screen></refsection></refentry>',
            set(),
        )
        self.assertInfoCategory(
            '<refentry xml:id="ST_Foo"><refnamediv><refname>ST_Foo</refname></refnamediv>'
            '<refsynopsisdiv><funcsynopsis><funcprototype><funcdef><type>integer</type> '
            '<function>ST_Foo</function></funcdef></funcprototype></funcsynopsis></refsynopsisdiv>'
            '<refsection><title>Examples</title><programlisting>SELECT ST_Bar(1);</programlisting>'
            '<screen>1</screen></refsection></refentry>',
            "current-function-in-examples",
        )
        self.assertCategories(
            '<refentry xml:id="geometry_overlaps"><refnamediv>'
            '<refname>&amp;&amp;(geometry,box2df)</refname></refnamediv>'
            '<refsynopsisdiv><funcsynopsis><funcprototype><funcdef><type>boolean</type> '
            '<function>&amp;&amp;</function></funcdef></funcprototype></funcsynopsis></refsynopsisdiv>'
            '<refsection><title>Examples</title><programlisting>SELECT a &amp;&amp; b;</programlisting>'
            '<screen>t</screen></refsection></refentry>',
            set(),
        )
        self.assertInfoCategory(
            '<refentry xml:id="geometry_overlaps"><refnamediv>'
            '<refname>&amp;&amp;(geometry,box2df)</refname></refnamediv>'
            '<refsynopsisdiv><funcsynopsis><funcprototype><funcdef><type>boolean</type> '
            '<function>&amp;&amp;</function></funcdef></funcprototype></funcsynopsis></refsynopsisdiv>'
            '<refsection><title>Examples</title><programlisting>SELECT a @ b;</programlisting>'
            '<screen>t</screen></refsection></refentry>',
            "current-function-in-examples",
        )

    def test_deprecated_alias_requires_canonical_links_and_no_examples(self):
        canonical = (
            '<refentry xml:id="CG_Foo"><refnamediv><refname>CG_Foo</refname>'
            '<refpurpose>Canonical</refpurpose></refnamediv></refentry>'
        )
        alias = (
            '<refentry xml:id="ST_Foo" role="deprecated-alias"><refnamediv><refname>ST_Foo</refname>'
            '<refpurpose>Deprecated alias</refpurpose></refnamediv><refsection><title>Description</title>'
            '<warning><para>Use <xref linkend="CG_Foo"/>.</para></warning></refsection></refentry>'
        )
        self.assertCategories(canonical + alias, set())
        self.assertCategories(
            canonical + alias + '<para>See <xref linkend="ST_Foo"/>.</para>',
            {"deprecated-alias-link"},
        )
        self.assertCategories(
            canonical + alias.replace(
                '</refentry>',
                '<refsection><title>Examples</title><programlisting>SELECT ST_Foo();</programlisting>'
                '<screen>1</screen></refsection></refentry>',
            ),
            {"deprecated-alias-examples"},
        )
        self.assertCategories(
            canonical + alias + '<refentry xml:id="Other"><refsection><title>Examples</title>'
            '<programlisting>SELECT ST_Foo();</programlisting></refsection></refentry>',
            {"deprecated-alias-reference", "example-missing-output"},
        )

    def test_info_findings_do_not_fail_warning_gate_or_count_as_warnings(self):
        path = write_tmp(".xml", DOCBOOK_OPEN + '<refentry xml:id="f"><screen>1\t2</screen></refentry>' + DOCBOOK_CLOSE)
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.assertEqual(0, main(["lint-source", "--fail-on", "warning", "--max-warnings", "0", str(path)]))
        self.assertIn("info=1", output.getvalue())
        self.assertNotIn("warning=", output.getvalue())

    def test_table_polish_findings_do_not_fail_warning_gate(self):
        table = (
            "left | right | a_very_long_column_name_that_should_be_expanded_in_polished_docs\n"
            "-----+-------+--------------------------------------------------------------------------\n"
            "1 |      2 | " + "x" * 90
        )
        path = write_tmp(".xml", DOCBOOK_OPEN + f'<refentry xml:id="f"><screen>{table}</screen></refentry>' + DOCBOOK_CLOSE)
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.assertEqual(0, main(["lint-source", "--fail-on", "warning", "--max-warnings", "0", str(path)]))
        self.assertIn("info=", output.getvalue())
        self.assertNotIn("warning=", output.getvalue())

    def test_sql_token_data_drives_all_source_detectors(self):
        self.assertTrue(set(SQL_STATEMENT_START_KEYWORDS).issubset(SQL_HIGHLIGHT_KEYWORDS))
        self.assertTrue(set(SQL_BLOCK_STARTER_EXTRAS).issubset(SQL_HIGHLIGHT_KEYWORDS))
        for keyword in SQL_STATEMENT_START_KEYWORDS:
            self.assertCategories(
                f'<refentry xml:id="f"><programlisting>{keyword} 1;\n(1 row)</programlisting></refentry>',
                {"mixed-programlisting-output"},
            )
            self.assertCategories(
                f'<refentry xml:id="f"><screen>{keyword} 1;</screen></refentry>',
                {"screen-contains-sql"},
            )
        for starter in SQL_STARTERS:
            body = starter if starter.endswith(";") else starter + "1;"
            findings = self.html_findings_for_sql_probe(body)
            self.assertTrue(any("lacks data-postgis-language='sql'" in finding.location for finding in findings), body)

    def test_xsl_sql_detection_covers_all_block_starters(self):
        xsl = (
            Path(__file__).resolve().parents[1]
            / "doc"
            / "xsl"
            / "postgis-html-block-labels.xsl"
        ).read_text(encoding="utf-8")
        for keyword in SQL_BLOCK_STARTER_KEYWORDS:
            suffix = ";" if keyword in {"BEGIN", "COMMIT"} else " "
            self.assertIn(
                f"starts-with($code.text.upper, '{keyword}{suffix}')",
                xsl,
                f"XSL SQL detection is missing {keyword}",
            )

    def html_findings_for_sql_probe(self, text: str):
        path = write_tmp(
            "-en.html",
            '<html><body>'
            '<div class="postgis-example-block postgis-example-code" role="group" data-postgis-block="code" aria-labelledby="l1">'
            '<span id="l1" class="postgis-example-label">Code</span>'
            '<button class="postgis-copy-button" type="button" aria-label="Copy" title="Copy"></button>'
            f'<pre id="p1" class="programlisting">{text}</pre></div>'
            '</body></html>',
        )
        return check_html_file(path, set())


class DocBookHtmlLintTest(unittest.TestCase):
    def html_findings(self, body: str, required_kinds: set[str] | None = None):
        path = write_tmp("-en.html", f"<html><body>{body}</body></html>")
        return check_html_file(path, required_kinds or set())

    def test_programlisting_wrapper_violation_and_clean_case(self):
        findings = self.html_findings('<pre id="p1" class="programlisting">SELECT 1;</pre>')
        self.assertIn("html-block-label", {finding.category for finding in findings})
        self.assertEqual(
            [],
            self.html_findings(
                '<div class="postgis-example-block postgis-example-code postgis-example-language-sql" '
                'role="group" data-postgis-block="code" data-postgis-language="sql" aria-labelledby="l1">'
                '<span id="l1" class="postgis-example-label">Code</span>'
                '<button class="postgis-copy-button" type="button" aria-label="Copy" title="Copy"></button>'
                '<pre id="p1" class="programlisting language-sql" data-postgis-language="sql">SELECT 1;</pre>'
                '</div>'
            ),
        )

    def test_sql_code_language_violation_and_clean_case(self):
        findings = self.html_findings(
            '<div class="postgis-example-block postgis-example-code" role="group" data-postgis-block="code" aria-labelledby="l1">'
            '<span id="l1" class="postgis-example-label">Code</span>'
            '<button class="postgis-copy-button" type="button" aria-label="Copy" title="Copy"></button>'
            '<pre id="p1" class="programlisting">SELECT 1;</pre></div>'
        )
        self.assertTrue(any("lacks data-postgis-language='sql'" in finding.location for finding in findings))
        self.assertEqual(
            [],
            self.html_findings(
                '<div class="postgis-example-block postgis-example-code postgis-example-language-sql" role="group" '
                'data-postgis-block="code" data-postgis-language="sql" aria-labelledby="l1">'
                '<span id="l1" class="postgis-example-label">Code</span>'
                '<button class="postgis-copy-button" type="button" aria-label="Copy" title="Copy"></button>'
                '<pre id="p1" class="programlisting language-sql" data-postgis-language="sql">SELECT 1;</pre></div>'
            ),
        )

    def test_screen_code_class_and_copy_button_violation_and_clean_case(self):
        findings = self.html_findings(
            '<div class="postgis-example-block postgis-example-output" role="group" data-postgis-block="output" aria-labelledby="l1">'
            '<span id="l1" class="postgis-example-label">Output</span>'
            '<button class="postgis-copy-button" type="button" aria-label="Copy" title="Copy"></button>'
            '<pre id="s1" class="screen language-sql">1</pre></div>'
        )
        messages = "\n".join(finding.location for finding in findings)
        self.assertIn("code/highlight class tokens", messages)
        self.assertIn("unexpectedly has a copy button", messages)
        self.assertEqual(
            [],
            self.html_findings(
                '<div class="postgis-example-block postgis-example-output" role="group" data-postgis-block="output" aria-labelledby="l1">'
                '<span id="l1" class="postgis-example-label">Output</span>'
                '<pre id="s1" class="screen">1</pre></div>'
            ),
        )

    def test_expect_from_requires_source_programlisting_on_refentry_page(self):
        xml = write_tmp(
            ".xml",
            DOCBOOK_OPEN
            + '<refentry xml:id="ST_Foo"><refsection><title>Examples</title><programlisting>SELECT 1;</programlisting></refsection></refentry>'
            + DOCBOOK_CLOSE,
        )
        html = Path(xml.parent) / "ST_Foo.html"
        html.write_text("<html><body><p>No generated code block here.</p></body></html>", encoding="utf-8")
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(1, main(["lint-html", "--expect-from", str(xml), str(html)]))

    def test_expect_from_does_not_require_screen_when_source_has_no_screen(self):
        xml = write_tmp(
            ".xml",
            DOCBOOK_OPEN
            + '<refentry xml:id="ST_Foo"><refsection><title>Examples</title><programlisting>SELECT 1;</programlisting></refsection></refentry>'
            + DOCBOOK_CLOSE,
        )
        html = Path(xml.parent) / "ST_Foo.html"
        html.write_text(
            '<html><body><div class="postgis-example-block postgis-example-code postgis-example-language-sql" '
            'role="group" data-postgis-block="code" data-postgis-language="sql" aria-labelledby="l1">'
            '<span id="l1" class="postgis-example-label">Code</span>'
            '<button class="postgis-copy-button" type="button" aria-label="Copy" title="Copy"></button>'
            '<pre id="p1" class="programlisting language-sql" data-postgis-language="sql">SELECT 1;</pre>'
            '</div></body></html>',
            encoding="utf-8",
        )
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(0, main(["lint-html", "--expect-from", str(xml), str(html)]))

    def test_required_kinds_violation_and_clean_case(self):
        self.assertTrue(self.html_findings("", {"programlisting"}))
        self.assertEqual(
            [],
            self.html_findings(
                '<div class="postgis-example-block postgis-example-output" role="group" data-postgis-block="output" aria-labelledby="l1">'
                '<span id="l1" class="postgis-example-label">Output</span>'
                '<pre id="s1" class="screen">1</pre></div>',
                {"screen"},
            ),
        )

    def test_geometry_figure_requires_adjacent_shared_visual_stack(self):
        broken = self.html_findings(
            '<div class="postgis-example-block postgis-example-code" data-postgis-visual-id="v1"></div>'
            '<div class="postgis-example-block postgis-example-output"></div>'
            '<div class="postgis-geometry-figure" data-postgis-visual-id="v1"></div>'
        )
        self.assertTrue(any("do not share visual id" in finding.location for finding in broken))
        self.assertEqual([], self.html_findings(
            '<div class="postgis-example-block postgis-example-code" data-postgis-visual-id="v1"></div>'
            '<div class="postgis-example-block postgis-example-output" data-postgis-visual-id="v1"></div>'
            '<div class="postgis-geometry-figure" data-postgis-visual-id="v1"></div>'
        ))


class DocBookRefIndexTest(unittest.TestCase):
    def test_ref_index_command_function_operator_and_clean_empty_case(self):
        path = write_tmp(
            ".xml",
            DOCBOOK_OPEN
            + '<chapter xml:id="admin"><title>Administration</title><section xml:id="hard-upgrade">'
            '<title>Hard upgrade</title><para><command role="reference-target">postgis_restore</command>'
            '</para></section></chapter>'
            + '<refentry xml:id="ST_Point"><refnamediv><refname>ST_Point</refname><refpurpose>Makes a point</refpurpose></refnamediv></refentry>'
            + '<refentry xml:id="geometry_overlaps"><refnamediv><refname>&amp;&amp;</refname><refpurpose>Overlaps</refpurpose></refnamediv></refentry>'
            + DOCBOOK_CLOSE,
        )
        index = build_index(path)
        self.assertEqual(
            {
                "id": "hard-upgrade",
                "label": "postgis_restore",
                "href": "admin.html#hard-upgrade",
                "title": "Hard upgrade",
            },
            index["commands"]["postgis_restore"],
        )
        self.assertEqual("ST_Point", index["functions"]["ST_POINT"]["label"])
        self.assertEqual("geometry_overlaps", index["operators"]["&&"][0]["id"])
        self.assertEqual(list(SQL_HIGHLIGHT_KEYWORDS), index["keywords"])
        self.assertIn("SELECT", index["keywords"])
        empty_path = write_tmp(".xml", DOCBOOK_OPEN + DOCBOOK_CLOSE)
        self.assertEqual(
            {"commands": {}, "functions": {}, "operators": {}, "keywords": list(SQL_HIGHLIGHT_KEYWORDS)},
            build_index(empty_path),
        )

    def test_ref_index_command_writes_json_and_script_with_same_data(self):
        path = write_tmp(
            ".xml",
            DOCBOOK_OPEN
            + '<refentry xml:id="ST_Point"><refnamediv><refname>ST_Point</refname><refpurpose>Makes a point</refpurpose></refnamediv></refentry>'
            + DOCBOOK_CLOSE,
        )
        json_output = path.parent / "postgis-ref-index.json"
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(0, main(["ref-index", str(path), str(json_output)]))

        expected = build_index(path)
        self.assertEqual(expected, json.loads(json_output.read_text(encoding="utf-8")))
        script = (path.parent / "postgis-ref-index.js").read_text(encoding="utf-8")
        self.assertTrue(script.startswith("window.POSTGIS_REF_INDEX = "))
        self.assertTrue(script.endswith(";\n"))
        self.assertEqual(expected, json.loads(script.removeprefix("window.POSTGIS_REF_INDEX = ").removesuffix(";\n")))

    def test_deprecated_alias_search_opens_canonical_page(self):
        path = write_tmp(
            ".xml",
            DOCBOOK_OPEN
            + '<refentry xml:id="CG_Foo"><refnamediv><refname>CG_Foo</refname>'
            '<refpurpose>Canonical function</refpurpose></refnamediv></refentry>'
            + '<refentry xml:id="ST_Foo" role="deprecated-alias"><refnamediv><refname>ST_Foo</refname>'
            '<refpurpose>Deprecated alias for CG_Foo</refpurpose></refnamediv>'
            '<refsection><warning><para>Use <xref linkend="CG_Foo"/>.</para></warning></refsection></refentry>'
            + DOCBOOK_CLOSE,
        )
        index = build_index(path)
        self.assertEqual("CG_Foo", index["functions"]["ST_FOO"]["id"])
        self.assertEqual("CG_Foo.html", index["functions"]["ST_FOO"]["href"])


if __name__ == "__main__":
    unittest.main()
