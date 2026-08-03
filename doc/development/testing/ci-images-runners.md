---
title: "CI Images and Runners"
date: 2026-07-26
weight: 12
geekdocHidden: false
---

PostGIS CI uses two container image families and several runner fleets. Keep
the dependency details in the image repositories and host setup repositories;
this page is the navigation map.

## Container Image Families

| Image family | Published at | Owner | Consumers |
| ------------ | ------------ | ----- | --------- |
| `postgis/postgis-build-env:*` | Docker Hub | <https://github.com/postgis/postgis-build-env> | GitHub Actions Linux matrix in `.github/workflows/ci.yml` |
| `repo.osgeo.org/postgis/build-test:*` | OSGeo container registry | <https://gitea.osgeo.org/postgis/postgis-docker> | Woodpecker workflows under `.woodpecker/` |

`postgis/postgis-build-env` has one `Dockerfile` and `build.py`. The script
builds and pushes the tag matrix used by the GitHub Actions Linux workflow. Its
README points at the Debbie Jenkins job that runs the regular image build.

`postgis/postgis-docker` owns the Woodpecker build-test images. The
`build-test/Containerfile.*` files and `build-test/Makefile` define the image
set, and the repository README documents logging in and pushing to the OSGeo
registry. At the time this page was written, this repository had build
instructions and a Debbie helper script, but no checked-in GitHub Actions,
Woodpecker, GitLab CI, or Jenkinsfile workflow of its own.

An image pull can fail before any PostGIS test starts. Treat registry timeouts,
manifest failures, and authentication failures as infrastructure failures until
the failing step has successfully entered a PostGIS build or test command. A
Docker Hub registry timeout has made the GitHub Actions Linux matrix red before
without identifying a source defect.

## Runner And Host Repositories

| Runner surface | Owner | What it covers |
| -------------- | ----- | -------------- |
| Woodie agents | <https://gitea.osgeo.org/sac/woodpecker-agent-config>, branch `woodie-3` | OSGeo Woodpecker agent startup scripts and deployment notes |
| Jenkins buildbot hosts | <https://gitea.osgeo.org/postgis/postgis-buildbots> | Linux, Debian, FreeBSD, Raspberry Pi, and Windows worker setup notes |
| In-tree Jenkins scripts | `ci/debbie/`, `ci/winnie/`, `ci/bessie/`, `ci/berrie*` | Commands run by PostGIS Jenkins jobs and worker labels |

Woodpecker pull-request jobs run on Woodie agents. Agent capacity, Docker
socket access, `binfmt_misc` handlers, DNS, and registry access are runner
properties, not PostGIS source properties. If a workflow adds an emulated or
cross-architecture job, verify the runner preflight in the Woodie agent fleet
before treating the failure as a test failure.

Jenkins worker setup lives outside this repository in
`postgis/postgis-buildbots`. That repository still contains historical setup
notes as well as active worker notes, so use the maintained inventory in
`utils/docs/ci_status/config.json` and the live Jenkins job labels before
reviving an old platform. Current 32-bit coverage is the Berrie Raspberry Pi
worker, which is 32-bit ARM. The old `bessie32` FreeBSD worker is not part of
the maintained CI inventory; do not rebuild 32-bit FreeBSD coverage from the old badge table
without fresh maintainer approval and a live worker readback.

## Architecture Coverage

Use the owning workflow or worker label for architecture claims:

* GitHub Actions Linux runs in Docker on GitHub-hosted Linux.
* GitHub Actions FreeBSD and macOS are owned by their workflow files and only
  cover the branches where those files exist.
* Woodpecker jobs currently declare `platform: linux/amd64` unless the
  workflow says otherwise.
* Jenkins `berrie` is the maintained 32-bit ARM surface, and `berrie64` is the
  Raspberry Pi 64-bit surface.
* Jenkins `bessie` is the maintained FreeBSD surface; `bessie32` is retired
  from the maintained inventory.

When changing architecture coverage, update the owning workflow or worker job,
then update [CI inventory standards](ci.md) and
`utils/docs/ci_status/config.json`.
