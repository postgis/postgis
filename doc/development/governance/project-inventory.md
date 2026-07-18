---
title: "PostGIS Project Inventory"
date: 2026-06-26
weight: 10
geekdocHidden: false
---

This page records the project codebases and services that need governance or
maintainer routing. It is intentionally separate from the PSC process so the
governance notes can describe how decisions are made while this page describes
what the project currently operates or routes.

## Umbrella Codebases

Route new work to the component repository when the component has a separate
upstream. Use the in-tree PostGIS repository only for core PostGIS work and for
explicit in-tree backpatches.

| Project or component | Repository |
|----------------------|------------|
| Core PostGIS | <https://gitea.osgeo.org/postgis/postgis> |
| Docker PostGIS | <https://github.com/postgis/docker-postgis> |
| PostGIS Java | <https://github.com/postgis/postgis-java> |
| PostGIS Workshop | <https://github.com/postgis/postgis-workshops> |
| H3-pg | <https://github.com/postgis/h3-pg> |
| `address_standardizer` | <https://github.com/postgis/address_standardizer> |
| `postgis_tiger_geocoder` | <https://git.osgeo.org/gitea/postgis/postgis_tiger_geocoder> |

For pull-request routing, source-of-truth branches, and mirror handling, see
[Pull request and maintainer workflow](../maintenance/_index.md).

## Project Services

| Service | Purpose | Maintainer documentation |
|---------|---------|--------------------------|
| `postgis.net` website | Public website, release pointers, news, RFC pages, community pages, and documentation links | [PostGIS website maintenance](../website.md) |
| `postgis.net/stuff` | Release tarballs, checksums, and generated documentation artefacts | [Release process](../release-process.md) |
| Debbie build host and build jobs | Release builds, distribution checks, and build-bot jobs referenced during release work | [Release process](../release-process.md) |
| OSGeo Trac | Tickets, legacy wiki, roadmap, reports, and historical project records | [Contributing workflow](../contributing.md) |
| Weblate | Translation repository maintenance for the PostGIS manual | [Pull request and maintainer workflow](../maintenance/_index.md) and [Manual documentation workflow](../manual.md) |
| Mailing lists and chat | Public development discussion, user support, and community coordination | [Contributing workflow](../contributing.md) |

Keep credentials, server-specific secrets, and deployment tokens out of this
repository. Repository docs should name the service, source-of-truth repository,
and maintainer workflow, not private operational details.
