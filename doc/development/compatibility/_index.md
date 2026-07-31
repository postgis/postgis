---
title: "Compatibility Data Maintenance"
date: 2026-07-31
weight: 35
geekdocCollapseSection: true
---

The compatibility browser is generated from versioned PostGIS data. It replaces
the old Trac compatibility tables without turning the website repository into a
second source of support policy.

## Ownership

The PostGIS repository owns:

* reviewed compatibility and lifecycle statements;
* source refresh and last-known-good fallback behavior;
* conflict detection and payload validation;
* the generated browser data model.

The `postgis.net` repository owns:

* the compatibility page, CSS, and JavaScript renderer;
* Hugo integration and publication;
* scrolling, hover, responsive, and accessibility behavior.

The website invokes the PostGIS generator during its build. It must not carry a
copy of the source matrix or commit the generated `compatibility.json` payload.

## Data Layers

The inputs have deliberately different review and refresh rules:

| File | Contents | Update rule |
| ---- | -------- | ----------- |
| `data/matrix.json` | Reviewed PostGIS series, compatibility edges, exceptions, dependency catalog, source citations, status definitions, and upgrade policy | Edit by hand and review as project policy |
| `data/cache.json` | Compact last-known-good release, lifecycle, packaging, vendored-source, and compile-time feature metadata | Regenerate with `update` and commit |
| `compatibility.json` | Resolved matrix, inventory, maintenance notices, provenance, controls, labels, and warnings for the website renderer | Build when needed; never commit |

`matrix.json` must not contain updater-owned sections. `cache.json` must remain
tracked so a clean checkout can build usable compatibility data while an
upstream service is unavailable.

## Normal Workflow

Run commands from the repository root:

```sh
python3 -m utils.docs.support_matrix update
python3 -m utils.docs.support_matrix check
python3 -m utils.docs.support_matrix build /tmp/compatibility.json
```

`update` is the only command that mutates repository data. It refreshes
`cache.json`, validates the resolved model before writing, and prints a warning
for every source that used cached data. Review the cache diff before committing
it.

`check` is read-only. It combines `matrix.json` with `cache.json`, validates the
source model, resolves minor and patch-level cells, builds the browser model,
and validates that model.

`build` performs the same validation and writes a disposable JSON payload only
after all checks pass. The output path may be outside the repository.

## Refreshed Sources

The updater combines independent sources rather than treating an old matrix as
authority. They currently include:

* PostgreSQL, GEOS, PROJ, GDAL, and SFCGAL release and lifecycle pages;
* Repology package versions;
* upstream GitHub or GitLab release tags where appropriate;
* PostGIS `configure.ac`, release branches, release documentation, and feature
  guards;
* CI dependency pins, including Winnie package and build defaults;
* vendored library versions found in the PostGIS source tree.

Every generated source has a stable source family and a human-facing label and
URL in the browser payload. When a remote request fails or a parser no longer
matches, the updater retains that source's cached section and records an
`update_warnings` entry. It must not silently replace valid cached facts with an
empty result.

Treat a new warning as a maintenance failure to investigate, even though the
fallback allows the website build to continue. If the upstream format changed,
fix the parser, rerun `update`, and confirm the warning disappears. Do not erase
the warning or hand-edit generated cache fields to make the page look current.

## Editing Compatibility Claims

Add project-reviewed claims to `matrix.json` as compatibility edges. Use exact
released versions where possible. Patch overlays belong on the affected
PostGIS patch release when support appeared or changed after the initial minor
release.

Keep these distinctions explicit:

* `supported`: intended to work without a known feature reduction;
* `tested`: directly exercised by current repository CI;
* `feature-limited`: builds, but a named feature is unavailable or degraded;
* `known-compatible`: known to work outside the currently supported window;
* `packaged`: observed in a package repository, without implying project
  support;
* `historical`: supported or known to work in its historical release context;
* `unsupported`: known not to work or outside an explicit supported range;
* `not-applicable`: the dependency or feature did not participate in that
  PostGIS release;
* `unknown`: no defensible statement is available.

Do not use EOL as a cell status. End of life belongs to a PostGIS, PostgreSQL,
or dependency version; the resolved cell still says whether the pair was
compatible. The browser converts historical `supported` cells to its compact
historical presentation when an axis is EOL.

For a new PostGIS or dependency release:

1. Refresh source-derived release and lifecycle metadata.
2. Add or adjust reviewed compatibility edges only where source guards, CI,
   release notes, or upstream documentation support the claim.
3. Add patch-level exceptions when compatibility changed within a PostGIS
   series.
4. Run `check` and inspect every conflict instead of weakening the validator.
5. Build the website from the companion `postgis.net` checkout and inspect the
   rendered matrix at narrow and desktop widths.

## Validation Contract

The checker rejects, among other invalid states:

* duplicate IDs, versions, controls, sources, or maintenance actions;
* overlapping claims of equal authority that resolve the same cell to different
  statuses;
* invalid patch overlays or source references;
* dependency dimensions that never expose a fully supported result;
* source matrix sections that should have been generated into the cache;
* browser rows, cells, inventory items, labels, controls, or source indices that
  do not cover the resolved model exactly;
* internal machine source IDs leaking into user-facing labels.

These assertions belong in generation because every consumer needs the same
resolved truth. Website JavaScript should render the payload and must not carry
dependency lists, compatibility exceptions, or project-policy fallbacks.

## Website Verification

In the `postgis.net` checkout, point `POSTGIS` at the PostGIS worktree and run:

```sh
make check POSTGIS=/path/to/postgis
make site POSTGIS=/path/to/postgis
```

The site build generates `compatibility.json` into the publish tree, then the
browser renderer consumes `browser.matrix`, `browser.inventory`,
`browser.maintenance`, `browser.provenance`, and `browser.warnings`. A payload
schema change therefore requires coordinated PostGIS and website changes.

Do not validate only the JSON. Open the built page and verify the default,
supported, EOL, full-history, and patch-detail controls; sticky headers; pointer
and touch scrolling; row/column highlighting; tooltips; and dark theme.
