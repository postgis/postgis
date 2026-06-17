# Governance Notes

This page records governance documentation that is maintained or being
consolidated in the repository. Published governance pages on
<https://postgis.net/development/> remain the public reference until the PSC
replaces or supersedes them.

## Project Steering Committee

RFC-1, published at <https://postgis.net/development/rfcs/rfc01/>, describes
the PostGIS Project Steering Committee as the body responsible for codebase
control and for the public face of the project. Its responsibilities include
quality-control mechanisms, legal compliance, release cycles, infrastructure,
website maintenance, promotion, public relations, and relationships with
organizations such as OSGeo.

Project proposals are discussed on the development mailing list. Proposals with
substantial technical detail may be written as RFCs. A proposal should remain
available for review for at least two business days before a decision is made.
PSC votes use the usual `+1`, `+0`, `0`, `-0`, and `-1` convention; a `-1`
veto must include reasoning and an alternate path. RFC-1 says a proposal passes
with `+2`, including the author, and no vetoes; an unresolved veto can be
overridden only by a majority of all eligible PSC members.

Votes are required for committee membership changes, infrastructure changes,
backward-compatibility breaks, substantial new code, inter-subsystem API or
object changes, procedure changes, release timing, relationships with external
entities, and other controversial matters.

The public website also carries the PostGIS code of conduct at
<https://postgis.net/community/conduct/>. It applies to project forums,
events, mailing lists, chat, tickets, wiki, blogs, social media, and other
community spaces. Serious or persistent conduct concerns may be reported to
event staff, a forum leader, or the PSC; private PSC mail goes to
`psc@postgis.net`.

## RFC-5 Status

RFC-5, published at <https://postgis.net/development/rfcs/rfc05/>, is the
historical PostGIS core contributor guideline. It is useful background for
write access, commit practice, Trac references, `NEWS`, code provenance, and
legal review.

The page is known to be stale in at least two ways:

* its "Projects under PostGIS umbrella" section lists only the three older
  projects;
* it points the canonical PSC and core contributor lists at the XML manual.

The PSC has discussed replacing the old RFC layout with a clearer governance
page and moving canonical PSC/core contributor records out of the XML manual.
Until that replacement is published, do not treat RFC-5 as a complete current
inventory of PostGIS governance or subprojects.

RFC-5 also documents maintainer expectations that are still relevant: keep
commits meaningful, preserve contributor authorship, update `NEWS`, route new
features to `master`, avoid new features on stable branches without release
manager or PSC permission, discuss significant or backward-incompatible work on
the development mailing list, avoid routine new branches in the official
repository except short-lived `test/` branches for CI, and check build bots
after commits.

Core contributors are the first provenance and license-review gate. They should
ensure contributors understand the project license, preserve copyright and
license headers, mark code derived from other projects, add required license
credits where appropriate, and ask the PSC or OSGeo legal counsel when a
contribution has an unusual licensing situation.

## Projects Under The PostGIS Umbrella

The current umbrella projects are:

| Project | Repository |
|---------|------------|
| Docker PostGIS | <https://github.com/postgis/docker-postgis> |
| PostGIS Java | <https://github.com/postgis/postgis-java> |
| PostGIS Workshop | <https://github.com/postgis/postgis-workshops> |
| H3-pg | <https://github.com/postgis/h3-pg> |
| `address_standardizer` | <https://github.com/postgis/address_standardizer> |
| `postgis_tiger_geocoder` | <https://git.osgeo.org/gitea/postgis/postgis_tiger_geocoder> |

For source-of-truth rules when landing pull requests or routing component work,
see [Pull request and maintainer workflow](../maintenance/).

## Related Projects To Check Before Major Releases

RFC-5 names upstream projects and downstream GIS suites that may be affected by
major PostGIS changes. When a change belongs in an upstream dependency, fix it
there first and then apply the result to PostGIS. Before major releases, test
or at least consider affected projects where practical.

Upstream dependencies include PostgreSQL, GEOS, PROJ, GDAL, and SFCGAL.
Downstream suites and extensions named in RFC-5 include MapServer, GeoServer,
OpenJUMP, QGIS, gvSIG, pgRouting, MobilityDB, pgPointcloud, osm2pgsql, and
other OpenStreetMap components such as Mapnik.
