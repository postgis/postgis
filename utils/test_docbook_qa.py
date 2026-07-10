#!/usr/bin/env python3

from __future__ import annotations

import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

from docbook_qa import (
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
            '<refentry xml:id="f"><programlisting>-- first comment\nSELECT 1;\n-- second comment</programlisting></refentry>',
            set(),
        )
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>---- decorative preface\nSELECT 1;</programlisting></refentry>',
            {"mixed-programlisting-output"},
        )

    def test_screen_contains_sql_violation_and_clean_case(self):
        self.assertCategories('<refentry xml:id="f"><screen>SELECT 1;</screen></refentry>', {"screen-contains-sql"})
        self.assertCategories('<refentry xml:id="f"><screen>POINT(1 2)</screen></refentry>', set())

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

    def test_tab_in_verbatim_violation_and_clean_case(self):
        self.assertInfoCategory('<refentry xml:id="f"><programlisting>SELECT\t1;</programlisting></refentry>', "tab-in-verbatim")
        self.assertCategories('<refentry xml:id="f"><screen>1    2</screen></refentry>', set())

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
            '<programlisting>-- query\n SELECT 1;</programlisting><para>No output</para></refsection></refentry>',
            "example-missing-output",
        )
        self.assertCategories(
            '<refentry xml:id="f"><refsection><title>Examples</title>'
            '<programlisting>SELECT 1;</programlisting><!-- gap --><screen>1</screen></refsection></refentry>',
            set(),
        )

    def test_info_findings_do_not_fail_warning_gate_or_count_as_warnings(self):
        path = write_tmp(".xml", DOCBOOK_OPEN + '<refentry xml:id="f"><screen>1\t2</screen></refentry>' + DOCBOOK_CLOSE)
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.assertEqual(0, main(["lint-source", "--fail-on", "warning", "--max-warnings", "0", str(path)]))
        self.assertIn("info=1", output.getvalue())
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


class DocBookRefIndexTest(unittest.TestCase):
    def test_ref_index_function_operator_and_clean_empty_case(self):
        path = write_tmp(
            ".xml",
            DOCBOOK_OPEN
            + '<refentry xml:id="ST_Point"><refnamediv><refname>ST_Point</refname><refpurpose>Makes a point</refpurpose></refnamediv></refentry>'
            + '<refentry xml:id="geometry_overlaps"><refnamediv><refname>&amp;&amp;</refname><refpurpose>Overlaps</refpurpose></refnamediv></refentry>'
            + DOCBOOK_CLOSE,
        )
        index = build_index(path)
        self.assertEqual("ST_Point", index["functions"]["ST_POINT"]["label"])
        self.assertEqual("geometry_overlaps", index["operators"]["&&"][0]["id"])
        self.assertEqual(list(SQL_HIGHLIGHT_KEYWORDS), index["keywords"])
        self.assertIn("SELECT", index["keywords"])
        empty_path = write_tmp(".xml", DOCBOOK_OPEN + DOCBOOK_CLOSE)
        self.assertEqual({"functions": {}, "operators": {}, "keywords": list(SQL_HIGHLIGHT_KEYWORDS)}, build_index(empty_path))

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


if __name__ == "__main__":
    unittest.main()
