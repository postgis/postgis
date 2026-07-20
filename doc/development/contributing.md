---
title: "Contributing Workflow"
date: 2026-06-26
weight: 20
geekdocHidden: false
---

PostGIS development planning happens in the
[PostGIS Trac instance](https://trac.osgeo.org/postgis/). For non-trivial
changes, file or find a Trac ticket and link your patch, pull request, or public
branch from that ticket. This keeps release planning visible even when patches
arrive through a forge mirror.

Filing tickets requires an
[OSGeo account](https://id.osgeo.org/ldap/create). The same account is used by
other OSGeo services.

Before filing a bug, decide whether you have found a defect or whether you have
a usage question. Questions belong on the user mailing list, chat, or another
community support forum. Bug reports should be reduced to the smallest
reproducible example, ideally a single SQL statement, and should include:

```sql
SELECT version();
SELECT postgis_full_version();
```

Search Trac first. If the problem is already reported, add the extra context to
the existing ticket rather than opening a duplicate. Test across PostGIS
versions when practical so maintainers can see whether the problem is new,
old, or already fixed.

Security issues that should not be recorded in the public ticket tracker should
be characterized as carefully as possible and sent to `security@postgis.net`.
Follow `SECURITY.md` for the current private reporting procedure.

The official source repository is the OSGeo Gitea repository:

```sh
git clone https://gitea.osgeo.org/postgis/postgis.git
cd postgis
./autogen.sh
./configure
```

GitHub, GitLab, Codeberg, and gitea.com mirrors are maintained for contributor
convenience. Pull requests on a mirror are welcome, but they are easier to miss
unless they also have a matching Trac ticket.

Maintainers landing mirror pull requests should follow the
[Pull request and maintainer workflow](maintenance/_index.md), including canonical
branch readback, tracker trailers, and `NEWS` conventions.

Subscribe to the
[postgis-devel mailing list](https://lists.osgeo.org/mailman/listinfo/postgis-devel)
if you intend to contribute more than occasional patches. Technical design
discussion, internal API changes, release policy, and contentious behavior
changes belong there.

Use [postgis-users](https://lists.osgeo.org/mailman/listinfo/postgis-users) for
user support and general usage questions. The public chat surfaces currently
include `#postgis:osgeo.org` on Matrix, PostGIS channels in PostgreSQL/OSGeo
Slack communities, the PostGIS channel on the "PostgreSQL, People & Data"
Discord server, and `#postgis` on Libera.chat IRC.

When starting as a contributor, keep the first patch small. A focused bug fix,
documentation correction, or test improvement builds reviewer trust faster than
a large redesign. Discuss larger plans on `postgis-devel` before spending a lot
of implementation time.

## First Contribution Path

1. Fork a public repo listed in [Source Code Repository](https://postgis.net/development/source_code/).
2. Build the tree with the setup documented in [Ubuntu setup](environment/ubuntu.md)
   or [Docker development environment](environment/docker.md).
3. Create a branch with a name that describes the ticket, function, or topic.
4. For a bug fix, find or create the matching Trac ticket before opening the
   patch for review.
5. For a larger feature, discuss the idea on `postgis-devel` before investing
   heavily in implementation.
6. Change the lowest layer that owns the behavior. SQL-visible behavior often
   starts in the SQL declaration and reaches C entry points in `postgis/`,
   `liblwgeom/`, raster, topology, or SFCGAL code.
7. Add the matching test: CUnit for isolated `liblwgeom` behavior, SQL
   regression tests for PostgreSQL-visible behavior, and documentation examples
   for user-visible SQL features.
8. Run focused validation before submitting the patch.

New features target `master` and the next minor or major PostGIS release, not
the next micro bugfix release. Bug fixes may be backpatched to maintained stable
branches when they are appropriate for that release line; check the branch's
`Version.config` and top `NEWS` section before preparing a backpatch.

Patch submission basics:

* Keep each patch focused on one ticket or reviewable topic.
* Add regression or CUnit coverage for behavior changes.
* Run the focused local validation described in [Ubuntu setup](environment/ubuntu.md)
  and [Testing and debugging](testing/_index.md).
* Follow [Coding style](style.md) for C naming, formatting, and Doxygen
  comments before writing or submitting source patches.
* Add documentation in the same change when a feature adds or changes SQL
  user-visible behavior.
* For SQL API, upgrade, and release-policy changes, read
  [Release and upgrade rules](release/_index.md) before opening a pull request.
* Git patches sent by email are also accepted; see
  <https://git-send-email.io/> for a practical walkthrough.

Bug reports for the raster extension should have a summary prefixed with
`[raster]` and use the raster component. Include `gdalinfo` output when the bug
depends on a source raster. If the bug concerns `raster2pgsql`, include the
loader command line.

Bug reports for `postgis_sfcgal` should have a summary prefixed with
`[sfcgal]`. Bug reports for `postgis_topology` should have a summary prefixed
with `[topology]`.
