---
title: "Governance Notes"
date: 2026-06-26
weight: 120
geekdocHidden: false
geekdocCollapseSection: true
---

This page is the maintained source for current PostGIS governance text in this
repository. Published governance pages on <https://postgis.net/development/>
remain the public reference until the PSC publishes this maintained structure
on the website or otherwise supersedes the historical RFC pages.

## Project Steering Committee

The PostGIS Project Steering Committee is the body responsible for codebase
control and for the public face of the project. Its responsibilities include
quality-control mechanisms, legal compliance, release cycles, infrastructure,
website maintenance, promotion, public relations, and relationships with
organizations such as OSGeo.

Project proposals are discussed on the development mailing list. Proposals with
substantial technical detail may be written as RFCs. A proposal should remain
available for review for at least two business days before a decision is made.
PSC votes use the usual `+1`, `+0`, `0`, `-0`, and `-1` convention; a `-1`
veto must include reasoning and an alternate path. A proposal passes with `+2`,
including the author, and no vetoes; an unresolved veto can be overridden only
by a majority of all eligible PSC members.

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

This section preserves the current PSC process from the historical RFC-1 text
published at <https://postgis.net/development/rfcs/rfc01/>. Maintain future PSC
process edits here, then publish the current text through the website
governance pages while keeping the old numbered RFC page as historical
provenance.

## Core Contributor Governance

Core contributor governance covers write access, commit practice, Trac
references, `NEWS`, code provenance, and legal review. Write access to the
canonical repository is granted by the PSC, and contributors with write access
are expected to understand the project process, stay subscribed to the
development mailing list, and support the code they commit or delegate that
support.

Maintainer expectations include keeping commits meaningful, preserving
contributor authorship, updating `NEWS`, routing new features to `master`,
avoiding new features on stable branches without release-manager or PSC
permission, discussing significant or backward-incompatible work on the
development mailing list, avoiding routine new branches in the official
repository except short-lived `test/` branches for CI, and checking build bots
after commits.

Core contributors are the first provenance and license-review gate. They should
ensure contributors understand the project license, preserve copyright and
license headers, mark code derived from other projects, add required license
credits where appropriate, and ask the PSC or OSGeo legal counsel when a
contribution has an unusual licensing situation.

This section carries the RFC-5 maintainer and provenance rules that still guide
core contributor work. Current project inventory and roster maintenance belong
in the dedicated inventory and public project pages rather than in the
historical RFC text.

## Project and Service Inventory

Current umbrella projects, related repositories, and project-operated services
are tracked in [PostGIS project inventory](project-inventory.md). Keep source
routing and service ownership there, rather than mixing it into the PSC process
or historical RFC notes.

For source-of-truth rules when landing pull requests or routing component work,
see [Pull request and maintainer workflow](../maintenance/_index.md).

## Related Projects To Check Before Major Releases

RFC-5 names upstream projects and downstream GIS suites that may be affected by
major PostGIS changes. When a change belongs in an upstream dependency, fix it
there first and then apply the result to PostGIS. Before major releases, test
or at least consider affected projects where practical.

Upstream dependencies include PostgreSQL, GEOS, PROJ, GDAL, and SFCGAL.
Downstream suites and extensions named in RFC-5 include MapServer, GeoServer,
OpenJUMP, QGIS, gvSIG, pgRouting, MobilityDB, pgPointcloud, osm2pgsql, and
other OpenStreetMap components such as Mapnik.
