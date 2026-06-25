# Trac Wiki Migration Map

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
rather than PostGIS.

| Trac wiki page | Repository home |
|----------------|-----------------|
| `WikiStart` | `README.md`, `CONTRIBUTING.md`, this directory, and `doc/trac-wiki/project-history/WikiStart.tracwiki` |
| `DevWikiMain` | `doc/development/README.md`, `workflow/`, `environment/`, `testing/`, `manual/`, `style/`, `internals/`, `release/`, `release-process/`, `wiki/`; original source in `doc/trac-wiki/development/DevWikiMain.tracwiki` |
| `DevWikiGettingABackTrace` | `doc/development/testing/`, `doc/development/tools/` |
| `DevWikiPGRegress` | `doc/development/testing/` |
| `DevWikiCUnit` | `doc/development/testing/` |
| `DevWikiDocNewFeature` | `doc/development/manual/` |
| `DevWikiPatch` | `doc/development/style/`, `doc/development/workflow/`, `doc/development/testing/` |
| `DevWikiMain/DevForDummy` | `doc/development/workflow/`, `environment/ubuntu/`, `testing/`, `manual/` |
| `DevWikiDockerTesting` | `doc/development/environment/docker/` |
| `DevWikiRFC` | Retired from this repository import. Current governance material belongs in `doc/development/governance/`; historical RFC text remains in `doc/rfc/` and the published RFC pages. |
| `DevWikiComitGuidelines` | `doc/development/maintenance/commit-guidelines/`, `doc/development/maintenance/`, `doc/development/governance/`, plus published RFC-5 at <https://postgis.net/development/rfcs/rfc05/> |
| `DevWikiEvent` | Archived in `doc/trac-wiki/development/DevWikiEvent.tracwiki`; convert only if it becomes current process documentation |
| `DevWikiPostGISCoding` | `doc/development/internals/postgresql/`, `doc/development/internals/memory/`; original source in `doc/trac-wiki/development/DevWikiPostGISCoding.tracwiki` |
| `DevWikiEmptyGeometry` | Archived in `doc/trac-wiki/development/DevWikiEmptyGeometry.tracwiki`; reconcile with current behavior and tests before moving into maintained internals docs |
| `NewDistCalc`, `NewDistCalcSubGeom`, `NewDistCalcGeom2Geom`, `3DDistancecalc` | Archived in `doc/trac-wiki/development/`; convert into maintained internals docs only after reconciling with current distance code |
| `DevWikiAffineParameters`, `DevWikiRealParameters` | Archived in `doc/trac-wiki/development/`; convert into maintained algorithm docs only after checking current code |
| `DevWikiSpatialCollectionTutorial` | Archived in `doc/trac-wiki/development/`; convert into user-facing documentation only after updating examples |
| `DevelopmentDiscussion`, `DevWikiISO19125`, `DevGUC`, `ToleranceDiscussion` | Archived in `doc/trac-wiki/development/` as historical design discussions |
| `DevClusteringFunctions` | Retired from this repository import; current clustering functions are implemented and documented in the user manual. |
| `DevCleanFromGEOS` | Retired from this repository import; the source page only contained a placeholder for a clean function API. |
| `DevWikiProvenanceReview` | Archived in `doc/trac-wiki/development/DevWikiProvenanceReview.tracwiki` as historical OSGeo incubation/provenance record |
| `DevFOSS4GCodeSprintNotes` | Archived in `doc/trac-wiki/project-history/DevFOSS4GCodeSprintNotes.tracwiki` |
| `DevWikiGardenTest` | `doc/development/manual/` |
| `DevWikiMingW64_Setup`, `DevWikiWinMingW64`, `DevWikiWinMingW64_21`, `DevWikiWinMingWSys_14_15`, `DevWikiWinMingWSys_20`, `DevWikiWinMingWSys_20_MSVC`, `DevWikiWinVC_15`, `DevWikiWinNSIS` | Archived in `doc/trac-wiki/development/`; current CI/build notes belong in `doc/development/environment/ubuntu/` and `ci/` |
| `DevCleanFromGEOS` | Archived in `doc/trac-wiki/development/DevCleanFromGEOS.tracwiki` |
| `UsersWikiMakeCheckConsiderations` | `doc/development/environment/ubuntu/`, `doc/development/testing/` |
| `ContinuousIntegration` | `ci/`, `doc/development/environment/ubuntu/`, and `doc/development/environment/docker/` |
| `CodeRepository`, `CodeMirrors` | Consolidated into `doc/development/workflow/` and `doc/development/maintenance/`; the tiny Trac source pages are not kept separately. |
| `PostGISDevelopment2021`, `PostGISDevelopment2022-1`, `PostGISDevelopment2023-1`, `PostGISDevelopment2026-1` | Archived in `doc/trac-wiki/project-history/`; current release-process actions belong in `doc/development/release-process/` |
| `PostGIS3`, `PostGIS20ReleaseAnnouncement`, `PostGISObsoleteVersionsMatrix`, `RCDownloads` | Archived in `doc/trac-wiki/project-history/`; maintained release workflow belongs in `doc/development/release-process/` |
| `PostGISCodeSprint2018`, `PostGISCodeSprint2020` | Archived in `doc/trac-wiki/project-history/` |
| `CodeSprintParis2012`, `GeospatialDataViewerInPgAdmin4ForPostGIS`, `GeospatialDataViewerInPgAdmin4ForPostGIS_FinalReport` | Left on Trac rather than imported; these are event or project-history records rather than maintained repository documentation. |
| `ImplementSortingMethodsBeforeGistIndexBuilding`, `MoveToGit`, `UsersWikiCascadeUnion` | Retired from this repository import because the work is done or superseded by current built-in behavior. |
| `PostGISExtensionPaths` | Archived in `doc/trac-wiki/project-history/PostGISExtensionPaths.tracwiki`; convert into current release or packaging docs only if it becomes maintained policy |
| `HOWTO_RELEASE` | `doc/development/release-process/`, `doc/development/release/versioning/`, `doc/development/release/deprecation/` |
| `STYLE` | `doc/development/style/` |
| `TODO` | Removed; live planning belongs in Trac roadmap, Trac tickets, and `NEWS` |
| Trac `roadmap` and ticket reports | `doc/development/maintenance/`, `doc/development/release/`, and Trac itself for live planning |
| Published RFC-5 contributor guidelines | `doc/development/workflow/`, `doc/development/maintenance/commit-guidelines/`, `doc/development/maintenance/`, `doc/development/governance/`, and <https://postgis.net/development/rfcs/> |
| `postgis.net/development/source_code` | `doc/development/workflow/`, `doc/development/maintenance/` |
| `postgis.net/development/getting_involved` | `doc/development/workflow/` |
| `postgis.net/development/bug_reporting` | `doc/development/workflow/` |
| `postgis.net/development/developer_docs` | `doc/development/manual/`, `doc/development/website/` |
| `postgis.net/development/versions_eol` | `doc/development/release-process/`, `doc/development/release/` |
| `postgis.net/development/rfcs/rfc01` | `doc/development/governance/` |
| `postgis.net/community/mailinglists`, `postgis.net/community/chat` | `doc/development/workflow/` |
| `postgis.net/community/conduct` | `doc/development/governance/` |
| `postgis.net/documentation/manual` | `doc/development/manual/` |
| `postgis.net` Hugo repository README, Makefile, config, and release-check script | `doc/development/website/`, `doc/development/release-process/` |
| `UsersWikiPostgreSQLPostGIS` | Archived in `doc/trac-wiki/user/UsersWikiPostgreSQLPostGIS.tracwiki`; convert into release-support documentation only after reconciling with current supported versions |
| `UsersWikiCreateFishnet` | Retired from this repository import; current grid generation belongs to built-in functions such as `ST_SquareGrid` and `ST_HexagonGrid` in the user manual. |
| `UsersWikiCleanPolygons` | Retired from this repository import; current invalid-geometry repair guidance belongs with `ST_MakeValid` in the user manual. |
| `WKTRaster_MapAlgebra` | Retired from this repository import; current raster map algebra functions are implemented and documented in the raster manual. |

Imported corpus:

| Category | Pages | Location |
|----------|-------|----------|
| Development | 28 | `doc/trac-wiki/development/` |
| User wiki | 139 | `doc/trac-wiki/user/` |
| Raster | 20 | `doc/trac-wiki/raster/` |
| Topology | 10 | `doc/trac-wiki/topology/` |
| Project history | 21 | `doc/trac-wiki/project-history/` |
| Source hosts | 5 | `doc/trac-wiki/source-hosts/` |

TitleIndex groups intentionally not imported into the corpus:

* `Trac*`, `Inter*`, `Wiki*`, `CamelCase`, `BadContent`, `PageTemplates`,
  `RecentChanges`, `SandBox`, `TitleIndex`, and `TicketQuery` are Trac system,
  help, or administration pages.
* `pgRouting` is an external project pointer rather than PostGIS documentation.

Do not add new maintained developer-process pages to Trac. Add them under
`doc/development/` and link from this index when the topic is useful for future
contributors.
