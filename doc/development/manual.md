---
title: "Manual Documentation Workflow"
date: 2026-06-26
weight: 100
geekdocHidden: false
---

PostGIS documentation is written in DocBook XML under `doc/`. The root file is
`doc/postgis.xml`, which includes chapter and reference XML files. The same XML
also generates SQL comments for functions.

Published manuals for released branches are linked from
<https://postgis.net/documentation/>. The development manual is published
separately and changes frequently while work is in progress.

Common targets:

```sh
make -C doc check
make -C doc comments
make -C doc html
make -C doc chunked-html
make -C doc images
make -C doc doxygen
```

The markup checker treats `programlisting` as input and `screen` as output.
Keep SQL and psql backslash commands in `programlisting`; a following `screen`
contains only the captured result.

To review a manual-wide visual change, build the old revision in a separate
worktree, build the current revision, and point the opt-in comparison target at
the old HTML directory:

```sh
make -C /path/to/old-postgis/doc html
make -C doc review-html REVIEW_HTML_BASE=/path/to/old-postgis/doc/html
python3 -m http.server --directory . 8000
```

Open `http://127.0.0.1:8000/doc/html/review/`. The page keeps the old manual on
the left and the current manual on the right. Scrolling either pane aligns the
other pane to the same vertical offset from the current function heading. For
example, `ST_Buffer +1200px` on one side maps to `ST_Buffer +1200px` on the
other, even though the manuals have different total heights. Use the search
field or a URL fragment such as `#ST_Buffer` to jump both panes to a common
function. The comparison is an uninstalled review artifact and
`make -C doc html-clean` removes it.

Use `make -C doc check-localized` after translation-related changes. Use
`make -C doc update-po` to refresh translation files when source XML changes
need to be propagated.

Manual translations are coordinated in Weblate at
<https://weblate.osgeo.org/projects/postgis/>. The website currently advertises
work in progress for languages including Italian, French, Portuguese,
Japanese, German, Korean, Spanish, and Polish.

Substantive manual figures are generated from executable examples at build
time. Do not add screenshots, authored geometry PNGs, or a second geometry
renderer. Static raster files under `doc/html/images/static` are limited to
branding and interface icons; the DocBook checker rejects other raster image
references. Use DocBook markup for equations and tables.

Run the example renderer before an HTML or PDF build:

```sh
make -C regress visual-examples
make -C doc html
make -C doc pdf
```

The renderer writes SVG and a manifest under
`doc/html/images/visual-examples`. HTML embeds those SVG files directly. The
PDF build rasterizes the same SVG source at high resolution only as a backend
compatibility fallback, so HTML and PDF do not have separate authored figures.
Raster-returning SQL examples are also executed at build time; their exact
pixels are embedded in the generated SVG container rather than checked in as a
manual screenshot.

Garden checks combine documentation examples with behavior validation. Keep the
command and review guidance in [Testing and debugging](testing/_index.md#garden-checks)
and link back here when manual examples need attention.

Generated Doxygen documentation for supported branches is linked from the
public developer documentation page. Build local Doxygen output with:

```sh
make -C doc doxygen
```

## Documenting New SQL Features

New SQL functions and visible behavior changes need documentation in the same
patch as the implementation.

For a new function:

1. Copy `doc/template.xml` or a nearby reference XML file.
2. Remove sections that do not apply, such as SQL/MM, curve, 3D, geography,
   raster, or SFCGAL notes.
3. Add an `Availability:` line with the target PostGIS version and mention any
   minimum dependency version, such as GEOS or SFCGAL, needed for the feature.
4. Keep `refpurpose` short and useful; it appears in generated indexes and in
   SQL comments visible from clients.
5. Name the XML file after the function or feature, then include the refentry
   from the appropriate `doc/reference_*.xml` file so `doc/postgis.xml`,
   generated comments, and HTML builds can see it.
6. Add executable examples when a figure materially improves the
   documentation. Return the relevant input and output geometry columns from
   one self-contained query and let the visual-example manifest supply the
   figure to HTML and PDF.

Run `make -C regress visual-examples` before building HTML or PDF. It verifies
every runnable example and automatically generates a figure for geometry found
in the result or in SQL literals. The staged PostGIS library supplies the SVG
paths and the PDF fallbacks are rasterized from the same SVG, so no second
geometry implementation or checked-in image is needed. Use
`role="visual-primary"` with a stable `xml:id` only when an authored example
needs an explicit figure identity or preference. Use `role="visual-skip"` on a
`programlisting` or its adjacent `screen` only when a figure would be
misleading; three-dimensional geometries are rendered with projected,
depth-sorted faces and hidden-edge cues and are not a reason to skip a figure.
Use `role="text-primary"` on a `screen`
when the exact textual type or coordinates carry meaning that the accompanying
figure cannot show; the figure is still generated, but HTML leaves Output open.
Use `role="documented-output"` on a `programlisting` only when the adjacent
`screen` is an authored preview for a figure and must not be treated as the
query's exact current text output, for example when dependency-version details
make the full geometry text unstable.
Use `role="visual-overlay"` when input and output geometries must share one
coordinate frame. Use `role="visual-separate-output"` when independent output
columns should be compared in separate frames at a common scale. Geometry
columns named with an `input_` prefix are treated as authored input layers; in
separate-output mode their remaining column names also label their frames.
Use `role="psql-expanded"` on a `screen` when a wide result is clearer in
psql's expanded display: one `-[ RECORD n ]-` block per row and one
`column | value` line per field. The example runner transposes this display
back into rows for comparison, so expanded examples remain executable.
Aliases on cast WKT literals and geometry result columns become figure legend
labels.  The renderer normalizes underscores in aliases to spaces, so SQL-safe
aliases such as `invalid_edges` display as `invalid edges`.  Quote an alias
when the label needs punctuation or capitalization that cannot be expressed as
a simple identifier.
Return native `geometry`/`geography` values when an `ST_AsText` or `ST_AsEWKT`
wrapper would exist only to make the documented output readable. Put the
readable WKT or EWKT in the adjacent `screen`; the example runner recognizes
the server's typed HEXEWKB result, compares it to that text through canonical
EWKB, and sends the native value directly to the figure renderer. HTML keeps
the readable WKT/EWKT visible by default and offers `Show HEXEWKB` when the
native transport value is useful for debugging. Keep an
explicit text conversion only when the function's text representation is the
behavior being documented.
Keep a short plain paragraph immediately before the associated
`programlisting` when an example needs an introduction; HTML joins that prose
to the Code, Output, and Figure card without requiring a wrapper or empty label.

Before committing a visual change, inspect the generated SVG rather than only
checking that a manifest entry exists. Verify that inputs and outputs share a
frame when overlap, direction, or scale is the point of the example; use
separate frames only when comparison benefits from them. Confirm that labels
name the actual semantic layers, dense points do not obscure edges, important
text remains expanded, and 3D faces are readable as solids. Then inspect the
same example in chunked HTML and in a PDF build.

When documenting optional arguments, follow nearby DocBook examples so generated
signatures and comments remain stable.

Keep deprecated compatibility names as compact refentries with
`role="deprecated-alias"`. Their warning must contain exactly one `xref` to the
canonical replacement, and their page must not duplicate examples. Put these
entries in a separate compatibility chapter. The source checker rejects links
and examples that use deprecated aliases elsewhere, while the reference search
index resolves the legacy spelling directly to the canonical page.

The public website also links training material, including the Introduction to
PostGIS workshop. The workshop has its own upstream repository under the PostGIS
umbrella at <https://github.com/postgis/postgis-workshops>; route workshop
changes there instead of adding them to the core documentation tree.

## Review Style

When reviewing manual changes, check prose, spelling, function names, argument
names, and examples together. Keep parameter names consistent with the SQL
signature and nearby reference entries, because `refpurpose`, signatures, and
selected reference text are reused in generated indexes and SQL comments.

Prefer focused examples that show the function contract directly. Put
tutorial-style or workbook-style exercises in the workshop repository unless
the material documents core function behavior that belongs in the manual.

Separate SQL input from query output when practical. Use `programlisting` for
commands or SQL fragments, and follow existing DocBook examples such as
`screen` blocks when the rendered output needs to be distinguished from the
query being run.

Keep DocBook source readable in diffs by following the local shape of nearby
XML: wrap long prose and SQL examples sensibly, preserve intentional indentation
inside listings, and avoid reflowing unrelated entries in the same patch.
