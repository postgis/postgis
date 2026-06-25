# Trac Wiki Migration Map

This is a draft-only migration workspace for ticket #5638. Do not treat it as
the final developer-documentation structure or publish it as a maintained
chapter. Once the remaining imported wiki pages are either folded into the
topic pages under `doc/development/`, left as historical Trac context, or
retired in the cleanup ledger, remove this map and keep only the maintained
topic pages plus any necessary archive pointer.

The PostGIS Trac wiki remains useful for tickets and live project discussion,
but maintained developer documentation should live in this repository. This
page records the wiki pages pulled while consolidating ticket #5638 and where
their maintained content belongs now.

The Trac `TitleIndex` was checked during the migration by reading the rendered
page list rather than the unexpanded Trac macro. All 306 linked wiki page
bodies were then pulled with `?format=txt` before deciding where they belong.
The index includes developer workflow pages, user cookbook pages, old install
notes, historical GSoC and meeting pages, topology and raster design notes,
Trac built-in documentation, and obsolete release planning pages. The Trac
roadmap and ticket reports were also checked as active planning surfaces,
together with the published development pages on
<https://postgis.net/development/>.

The `postgis.net` website source repository was also checked at
<https://gitea.osgeo.org/postgis/postgis.net>. Development, RFC, community,
documentation, release-link, and website-maintenance pages were compared with
the repository docs so the maintained content in `doc/development/` does not
depend on the website as the only source.

This ticket moves maintained developer workflow material into
`doc/development/` and imports the PostGIS-specific Trac wiki bodies into
[`doc/trac-wiki/`](../trac-wiki/) as an archival source corpus. Trac engine
help and administration pages are not copied because they document Trac itself
rather than PostGIS. Pages inspected during migration but intentionally left on
Trac or retired from the repository import are tracked in the
[Trac cleanup ledger](wiki/trac-cleanup.md).

| Trac wiki page | Repository home |
|----------------|-----------------|
| `WikiStart` | `README.md`, `CONTRIBUTING.md`, this directory, and `doc/trac-wiki/project-history/WikiStart.tracwiki` |
| `DevWikiGettingABackTrace` | `doc/development/testing.md`, `doc/development/tools.md` |
| `DevWikiPGRegress` | `doc/development/testing.md` |
| `DevWikiCUnit` | `doc/development/testing.md` |
| `DevWikiDocNewFeature` | `doc/development/contributing.md`, `doc/development/manual.md` |
| `DevWikiPatch` | `doc/development/style.md`, `doc/development/contributing.md`, `doc/development/testing.md` |
| `DevWikiMain/DevForDummy` | `doc/development/contributing.md`, `doc/development/style.md` |
| `DevWikiDockerTesting` | `doc/development/environment/docker.md` |
| `DevWikiComitGuidelines` | `doc/development/maintenance/commit-guidelines.md`, `doc/development/maintenance.md`, `doc/development/governance.md`, plus published RFC-5 at <https://postgis.net/development/rfcs/rfc05/> |
| `DevWikiPostGISCoding` | `doc/development/internals/postgresql.md`, `doc/development/internals/memory.md`; original source in `doc/trac-wiki/development/DevWikiPostGISCoding.tracwiki` |
| `DevWikiEmptyGeometry` | `doc/development/internals/empty-geometry.md`; current behavior was reconciled against code and regression tests |
| `DevWikiAffineParameters` | `doc/development/internals/raster-affine.md`; original images recovered under `doc/development/internals/images/raster-affine/` |
| `DevWikiRealParameters` | `doc/development/internals/raster-physical-parameters.md`; original images recovered under `doc/development/internals/images/raster-physical-parameters/` |
| `DevWikiSpatialCollectionTutorial` | `doc/development/internals/spatial-collections.md`; current code no longer has the old `SPATIAL_COLLECTION` API, so the maintained page records current geometry collection and raster/vector crossing paths |
| `WKTRaster_GDALDriverSpecificationWorking` | `doc/development/internals/raster-gdal-driver.md`; the old GDAL driver working spec was compressed into the maintained boundary between PostGIS raster storage/catalog contracts and the external GDAL `PostGISRaster` driver |
| `DevGUC` | `doc/development/release/api.md` |
| `ToleranceDiscussion` | `doc/development/internals/precision-tolerance.md`; current behavior was reconciled against GEOS-backed overlay `gridSize`, `ST_Snap`, topology precision/tolerance APIs, predicate distance arguments, coverage/triangulation tolerances, and MVT quantization, while the still-open global precision-model question remains in `doc/development/doc-wip.md` |
| `DevWikiGardenTest` | `doc/development/testing.md` |
| `UsersWikiMakeCheckConsiderations` | `doc/development/environment/ubuntu.md`, `doc/development/testing.md` |
| `ContinuousIntegration` | `ci/`, `doc/development/testing/ci.md`, `doc/development/environment/ubuntu.md`, and `doc/development/environment/docker.md` |
| `HOWTO_RELEASE` | `doc/development/release-process.md`, `doc/development/release/versioning.md`, `doc/development/release/deprecation.md` |
| `STYLE` | `doc/development/style.md` |
| Trac `roadmap` and ticket reports | `doc/development/maintenance.md`, `doc/development/release.md`, and Trac itself for live planning |
| Published RFC-5 contributor guidelines | `doc/development/contributing.md`, `doc/development/maintenance/commit-guidelines.md`, `doc/development/maintenance.md`, `doc/development/governance.md`, and <https://postgis.net/development/rfcs/> |
| `postgis.net/development/source_code` | `doc/development/contributing.md`, `doc/development/maintenance.md` |
| `postgis.net/development/getting_involved` | `doc/development/contributing.md` |
| `postgis.net/development/bug_reporting` | `doc/development/contributing.md` |
| `postgis.net/development/developer_docs` | `doc/development/manual.md`, `doc/development/website.md` |
| `postgis.net/development/versions_eol` | `doc/development/release-process.md`, `doc/development/release.md` |
| `postgis.net/development/rfcs/rfc01` | `doc/development/governance.md` |
| `postgis.net/community/mailinglists`, `postgis.net/community/chat` | `doc/development/contributing.md` |
| `postgis.net/community/conduct` | `doc/development/governance.md` |
| `postgis.net/documentation/manual` | `doc/development/manual.md` |
| `postgis.net` Hugo repository README, Makefile, config, and release-check script | `doc/development/website.md`, `doc/development/release-process.md` |

Imported corpus:

| Category | Pages | Location |
|----------|-------|----------|
| Development | 15 | `doc/trac-wiki/development/` |
| User wiki | 133 | `doc/trac-wiki/user/` |
| Raster | 20 | `doc/trac-wiki/raster/` |
| Project history | 17 | `doc/trac-wiki/project-history/` |

TitleIndex groups intentionally not imported into the corpus:

* `Trac*`, `Inter*`, `Wiki*`, `CamelCase`, `BadContent`, `PageTemplates`,
  `RecentChanges`, `SandBox`, `TitleIndex`, and `TicketQuery` are Trac system,
  help, or administration pages.
* `pgRouting` is an external project pointer rather than PostGIS documentation.

Do not add new maintained developer-process pages to Trac. Add them under
`doc/development/` and link from this index when the topic is useful for future
contributors.
