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
[`doc/trac-wiki/`](../../trac-wiki/) as an archival source corpus. Trac engine
help and administration pages are not copied because they document Trac itself
rather than PostGIS. Pages inspected during migration but intentionally left on
Trac or retired from the repository import are tracked in the
[Trac cleanup ledger](trac-cleanup.md).

| Trac wiki page | Repository home |
|----------------|-----------------|
| `WikiStart` | `README.md`, `CONTRIBUTING.md`, this directory, and `doc/trac-wiki/project-history/WikiStart.tracwiki` |
| `DevWikiMain` | Retired from this repository import; replaced by `doc/development/README.md`, `doc/development/contributing.md`, and the topic indexes under `environment/`, `testing/`, `manual/`, `style/`, `internals/`, `release/`, `release-process/`, `governance/`, `maintenance/`, `website/`, and `wiki/` |
| `DevWikiGettingABackTrace` | `doc/development/testing/`, `doc/development/tools/` |
| `DevWikiPGRegress` | `doc/development/testing/` |
| `DevWikiCUnit` | `doc/development/testing/` |
| `DevWikiDocNewFeature` | `doc/development/contributing.md`, `doc/development/manual/` |
| `DevWikiPatch` | `doc/development/style/`, `doc/development/contributing.md`, `doc/development/testing/` |
| `DevWikiMain/DevForDummy` | `doc/development/contributing.md`, `doc/development/style/` |
| `DevWikiDockerTesting` | `doc/development/environment/docker/` |
| `DevWikiRFC` | Retired from this repository import. Current governance material belongs in `doc/development/governance/`; historical RFC text remains in `doc/rfc/` and the published RFC pages. |
| `DevWikiComitGuidelines` | `doc/development/maintenance/commit-guidelines/`, `doc/development/maintenance/`, `doc/development/governance/`, plus published RFC-5 at <https://postgis.net/development/rfcs/rfc05/> |
| `DevWikiEvent` | Left on Trac; recorded in `doc/development/wiki/trac-cleanup.md` as historical meeting/sprint navigation |
| `DevWikiPostGISCoding` | `doc/development/internals/postgresql/`, `doc/development/internals/memory/`; original source in `doc/trac-wiki/development/DevWikiPostGISCoding.tracwiki` |
| `DevWikiEmptyGeometry` | `doc/development/internals/empty-geometry.md`; current behavior was reconciled against code and regression tests |
| `DevWikiAffineParameters` | `doc/development/internals/raster-affine.md`; original images recovered under `doc/development/internals/images/raster-affine/` |
| `DevWikiRealParameters` | `doc/development/internals/raster-physical-parameters.md`; original images recovered under `doc/development/internals/images/raster-physical-parameters/` |
| `DevWikiSpatialCollectionTutorial` | `doc/development/internals/spatial-collections.md`; current code no longer has the old `SPATIAL_COLLECTION` API, so the maintained page records current geometry collection and raster/vector crossing paths |
| `DevGUC` | `doc/development/release/api/` |
| `DevelopmentDiscussion` | Left on Trac; still-open PostGIS 2.0 design themes were extracted into `doc/development/doc-wip.md` and archive-handling policy is recorded in `doc/development/wiki/trac-cleanup.md` |
| `ToleranceDiscussion` | Archived in `doc/trac-wiki/development/` as a historical design discussion |
| `DevWikiISO19125` | Left on Trac; still-open standards-conformance themes were extracted into `doc/development/doc-wip.md` |
| `DevClusteringFunctions` | Retired from this repository import; current clustering functions are implemented and documented in the user manual. |
| `DevCleanFromGEOS` | Retired from this repository import; the source page only contained a placeholder for a clean function API. |
| `DevWikiProvenanceReview` | Left on Trac; recorded in `doc/development/wiki/trac-cleanup.md` as a historical OSGeo incubation/provenance record rather than current contributor guidance |
| `DevFOSS4GCodeSprintNotes` | Left on Trac; recorded in `doc/development/wiki/trac-cleanup.md` as historical sprint agenda/release-planning notes |
| `DevWikiGardenTest` | `doc/development/testing/` |
| `DevWikiMingW64_Setup`, `DevWikiWinMingW64`, `DevWikiWinMingW64_21` | Historical MinGW-w64/MSYS and prepared-bundle packaging context folded into `doc/development/environment/windows/`; current behavior should be checked against `.github/workflows/msys.yml` and `ci/winnie/` |
| `DevWikiWinMingWSys_20`, `DevWikiWinMingWSys_20_MSVC`, `DevWikiWinVC_15`, `DevWikiWinNSIS` | Archived in `doc/trac-wiki/development/`; current Windows CI/build notes belong in `doc/development/environment/windows/` and `ci/` |
| `DevWikiWinMingWSys_14_15` | Retired from this repository import; obsolete Windows XP/PostGIS 1.4 and 1.5 build notes are recorded in `doc/development/wiki/trac-cleanup.md` |
| `DevCleanFromGEOS` | Archived in `doc/trac-wiki/development/DevCleanFromGEOS.tracwiki` |
| `UsersWikiMakeCheckConsiderations` | `doc/development/environment/ubuntu/`, `doc/development/testing/` |
| `ContinuousIntegration` | `ci/`, `doc/development/testing/ci.md`, `doc/development/environment/ubuntu/`, and `doc/development/environment/docker/` |
| `CodeRepository`, `CodeMirrors` | Consolidated into `doc/development/contributing.md` and `doc/development/maintenance/`; the tiny Trac source pages are not kept separately. |
| `PostGISDevelopment2021`, `PostGISDevelopment2022-1`, `PostGISDevelopment2023-1`, `PostGISDevelopment2026-1` | Archived in `doc/trac-wiki/project-history/`; current release-process actions belong in `doc/development/release-process/` |
| `GoogleSeasonDocs2019` | Left on Trac; not-yet-done documentation ideas were extracted into `doc/development/doc-wip.md` |
| `PostGIS3` | Retired from this repository import; open major-version research themes were extracted into `doc/development/doc-wip.md` |
| `PostGISObsoleteVersionsMatrix` | Retired from this repository import; support-matrix maintenance rules were folded into `doc/development/release/dependencies/` |
| `PostGIS20ReleaseAnnouncement`, `RCDownloads` | Archived in `doc/trac-wiki/project-history/`; maintained release workflow belongs in `doc/development/release-process/` |
| `PostGISCodeSprint2018`, `PostGISCodeSprint2020` | Archived in `doc/trac-wiki/project-history/` |
| `CodeSprintParis2012`, `GeospatialDataViewerInPgAdmin4ForPostGIS`, `GeospatialDataViewerInPgAdmin4ForPostGIS_FinalReport` | Left on Trac rather than imported; these are event or project-history records rather than maintained repository documentation. |
| `ImplementSortingMethodsBeforeGistIndexBuilding`, `MoveToGit`, `UsersWikiCascadeUnion` | Retired from this repository import because the work is done or superseded by current built-in behavior. |
| `PostGISExtensionPaths` | Archived in `doc/trac-wiki/project-history/PostGISExtensionPaths.tracwiki`; convert into current release or packaging docs only if it becomes maintained policy |
| `HOWTO_RELEASE` | `doc/development/release-process/`, `doc/development/release/versioning/`, `doc/development/release/deprecation/` |
| `STYLE` | `doc/development/style/` |
| `TODO` | Removed; live planning belongs in Trac roadmap, Trac tickets, and `NEWS` |
| Trac `roadmap` and ticket reports | `doc/development/maintenance/`, `doc/development/release/`, and Trac itself for live planning |
| Published RFC-5 contributor guidelines | `doc/development/contributing.md`, `doc/development/maintenance/commit-guidelines/`, `doc/development/maintenance/`, `doc/development/governance/`, and <https://postgis.net/development/rfcs/> |
| `postgis.net/development/source_code` | `doc/development/contributing.md`, `doc/development/maintenance/` |
| `postgis.net/development/getting_involved` | `doc/development/contributing.md` |
| `postgis.net/development/bug_reporting` | `doc/development/contributing.md` |
| `postgis.net/development/developer_docs` | `doc/development/manual/`, `doc/development/website/` |
| `postgis.net/development/versions_eol` | `doc/development/release-process/`, `doc/development/release/` |
| `postgis.net/development/rfcs/rfc01` | `doc/development/governance/` |
| `postgis.net/community/mailinglists`, `postgis.net/community/chat` | `doc/development/contributing.md` |
| `postgis.net/community/conduct` | `doc/development/governance/` |
| `postgis.net/documentation/manual` | `doc/development/manual/` |
| `postgis.net` Hugo repository README, Makefile, config, and release-check script | `doc/development/website/`, `doc/development/release-process/` |
| `UsersWikiPostgreSQLPostGIS` | Archived in `doc/trac-wiki/user/UsersWikiPostgreSQLPostGIS.tracwiki`; convert into release-support documentation only after reconciling with current supported versions |
| `UsersWikiPostGIS15Ubuntu1110pkg` | Retired from this repository import; obsolete Ubuntu 11.10/PostGIS 1.5 package setup is recorded in `doc/development/wiki/trac-cleanup.md` |
| `UsersWikiCreateFishnet` | Retired from this repository import; current grid generation belongs to built-in functions such as `ST_SquareGrid` and `ST_HexagonGrid` in the user manual. |
| `UsersWikiGenerateHexagonalGrid` | Retired from this repository import; current hexagon grid generation belongs to `ST_HexagonGrid` in the user manual, with H3 tiling routed to `h3-pg` |
| `UsersWikiCheckInvalidGeometriesFromGeometryColumns` | Retired from this repository import; current registered-column validity scanning is documented in the `ST_IsValid` manual examples |
| `UsersWikiExamplesInterpolateWithOffset` | Retired from this repository import; current offset interpolation is documented with `ST_LineInterpolatePoint` and `ST_OffsetCurve` in the manual |
| `UsersWikiExamplesSpikeRemover` | Retired from this repository import; `ST_SimplifyVW` is documented as a simplification tool rather than a dedicated spike-repair replacement |
| `FAQ` | Retired from this repository import; current manual pages cover the surviving SRID, WKB, function naming, and overlay precision topics, while the old Windows/PostGIS 1.x entries are obsolete. |
| `UsersWikiCleanPolygons` | Retired from this repository import; current invalid-geometry repair guidance belongs with `ST_MakeValid` in the user manual. |
| `WKTRaster_MapAlgebra` | Retired from this repository import; current raster map algebra functions are implemented and documented in the raster manual. |

Imported corpus:

| Category | Pages | Location |
|----------|-------|----------|
| Development | 16 | `doc/trac-wiki/development/` |
| User wiki | 133 | `doc/trac-wiki/user/` |
| Raster | 20 | `doc/trac-wiki/raster/` |
| Topology | 10 | `doc/trac-wiki/topology/` |
| Project history | 17 | `doc/trac-wiki/project-history/` |
| Source hosts | 5 | `doc/trac-wiki/source-hosts/` |

TitleIndex groups intentionally not imported into the corpus:

* `Trac*`, `Inter*`, `Wiki*`, `CamelCase`, `BadContent`, `PageTemplates`,
  `RecentChanges`, `SandBox`, `TitleIndex`, and `TicketQuery` are Trac system,
  help, or administration pages.
* `pgRouting` is an external project pointer rather than PostGIS documentation.

Do not add new maintained developer-process pages to Trac. Add them under
`doc/development/` and link from this index when the topic is useful for future
contributors.
