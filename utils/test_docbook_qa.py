#!/usr/bin/env python3

from __future__ import annotations

import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

from docbook_qa import add_source_findings, build_index, check_html_file, load_xml, main

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

    def test_mixed_programlisting_output_violation_and_clean_case(self):
        self.assertCategories(
            '<refentry xml:id="f"><programlisting>SELECT 1;\n ?column?\n----------\n 1\n(1 row)</programlisting></refentry>',
            {"mixed-programlisting-output"},
        )
        self.assertCategories('<refentry xml:id="f"><programlisting>SELECT 1;</programlisting></refentry>', set())

    def test_screen_contains_sql_violation_and_clean_case(self):
        self.assertCategories('<refentry xml:id="f"><screen>SELECT 1;</screen></refentry>', {"screen-contains-sql"})
        self.assertCategories('<refentry xml:id="f"><screen>POINT(1 2)</screen></refentry>', set())

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
        empty_path = write_tmp(".xml", DOCBOOK_OPEN + DOCBOOK_CLOSE)
        self.assertEqual({"functions": {}, "operators": {}}, build_index(empty_path))


if __name__ == "__main__":
    unittest.main()
