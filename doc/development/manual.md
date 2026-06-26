---
title: "Manual Documentation Workflow"
date: 2026-06-26
weight: 100
geekdocHidden: false
---

# Manual Documentation Workflow

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

Use `make -C doc check-localized` after translation-related changes. Use
`make -C doc update-po` to refresh translation files when source XML changes
need to be propagated.

Manual translations are coordinated in Weblate at
<https://weblate.osgeo.org/projects/postgis/>. The website currently advertises
work in progress for languages including Italian, French, Portuguese,
Japanese, German, Korean, Spanish, and Polish.

Manual images live in `doc/html/images`. Each `.wkt` file stores one drawing,
optionally split into named layers by prefixing a line with `StyleName;WKT`.
The generator associates each layer with `styles.conf` and streams the
GraphicsMagick command without temporary image files, so parallel image builds
are safe.

To regenerate images:

```sh
make -C doc/html/images generator
make -C doc/html/images images -j"$(nproc)"
```

Invoke `doc/html/images/generator` directly to spot-check artwork. The `-v`
flag prints the exact GraphicsMagick or ImageMagick command, `-s` overrides the
canvas size, and passing a second filename overrides the default `.png` output.
Set `POSTGIS_DOC_CONVERTER` when autodetection cannot find `gm`, `magick`, or
`convert`.

Garden checks combine documentation examples with behavior validation. Keep the
command and review guidance in [Testing and debugging](testing.md#garden-checks)
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
6. Add examples and images when they materially improve the documentation. For
   generated `.wkt` images, also register the expected PNG in the appropriate
   `GENERATED_IMAGES`, `ALL_IMAGES`, or related list in
   `doc/html/images/Makefile.in`; the image build uses explicit lists rather
   than globbing every `.wkt` file.

When documenting optional arguments, follow nearby DocBook examples so generated
signatures and comments remain stable.

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
