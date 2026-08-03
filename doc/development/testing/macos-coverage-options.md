---
title: "macOS Coverage Options"
date: 2026-07-27
weight: 11
geekdocHidden: false
---

The macOS CI job is not a Woodpecker configuration gap. The current Darwin
coverage is the GitHub Actions workflow in `.github/workflows/ci-macos.yml`,
which runs on `macos-latest`, installs dependencies with Homebrew, builds with
Apple clang and Darwin `make`, installs PostGIS, and runs regression tests with
extension and dump/restore coverage.

Woodpecker jobs running on Linux cannot substitute for that lane. Containers,
Wine, and cross-compilation do not exercise Darwin libc, the Mach-O dynamic
loader, Homebrew's dependency graph, Apple clang, Darwin filesystem behavior, or
codesign and SIP-adjacent behavior. Real macOS coverage needs macOS on Apple
hardware.

## Option 1: Keep GitHub Actions Authoritative

This is the recommended default.

PostGIS keeps the current GitHub Actions macOS job as the authoritative Darwin
signal and does not try to make Woodpecker claim full parity. The project loses
one property: Woodpecker cannot independently prove every release and pull
request platform from the OSGeo CI surface alone.

If GitHub Actions is unavailable, if `macos-latest` changes underneath the
workflow, or if GitHub changes access to macOS runners, the project loses the
Darwin signal until the workflow is updated or another Apple runner is
available. That does not by itself block source tarball creation, Debbie release
jobs, Winnie jobs, or Linux/FreeBSD/ARM Woodpecker coverage, but the release
greenlight checklist's "all bots are green" check should treat the Darwin row as
unknown rather than green.

The cost is the existing GitHub Actions dependency. As of 2026-07-27, GitHub's
published hosted-runner rates list standard macOS at `$0.062` per minute,
larger macOS at `$0.077` per minute, and M2 Pro larger macOS at `$0.102` per
minute. Public-repository billing and organization quotas are GitHub account
policy, not a PostGIS repository setting, so the operational risk is runner
availability and image churn more than direct per-minute cost in this repo.

## Option 2: Register A Hosted Apple Runner As A Woodpecker Agent

This gives Woodpecker real Darwin coverage, but it creates a standing hosted
machine to administer.

Representative hosted Apple hardware prices found on 2026-07-27:

| Provider | Representative published price | Third-party agent feasibility |
| -------- | ------------------------------ | ----------------------------- |
| MacStadium | M2 Mac mini from `$109` per month; M4 Mac mini from `$149` per month | Suitable in principle. The product is dedicated Apple hardware with root access, a dedicated IP address, and current macOS, so a Woodpecker agent can be installed by the project. |
| Macly | M4 Mac mini `$99.99` per month or `$14.99` per day | Suitable in principle. The product advertises full SSH/admin access and explicitly says users can install any CI/CD agent. |
| Scaleway Apple Mac mini | M4 advertised at `EUR 0.22` per hour in Scaleway's product announcement, about `EUR 160.60` for a 730-hour month before tax; the product page describes dedicated Mac minis reachable over remote desktop or SSH | Suitable in principle. The product exposes a dedicated Mac mini and documents runner setup; the project would administer the Woodpecker agent. |
| AWS EC2 Mac Dedicated Host | `mac2-m2` listed at `$0.878` per host-hour and `mac-m4` listed on the dedicated-host price table; EC2 Mac hosts have a 24-hour minimum allocation | Technically suitable but usually too expensive for always-on open-source CI. It is dedicated Apple hardware presented as EC2 infrastructure, so installing an agent is possible, but the host-hour model makes it a poor fit for a mostly idle PostGIS Darwin lane. |
| MacinCloud Dedicated Server | Dedicated server advertised from `$49` per month | Possibly suitable only for tiers with full administrator/root access. Shared or managed remote-desktop plans are not enough for an unattended Woodpecker agent. |

The real total is provider cost plus OS patching, Homebrew cache and package
maintenance, Woodpecker secret handling, runner upgrades, monitoring, and a
named administrator. If nobody owns those tasks, the hosted runner will become a
stale red CI row rather than useful parity.

## Option 3: Project-Owned Apple Hardware

A project-owned Mac mini is the usual physical answer. As of 2026-07-27,
Apple's US shop structured product data listed the Mac mini aggregate low price
as `$799`, with M4 and M4 Pro configurations available. The CI total is not only
the purchase price:

* a Mac mini with enough RAM and storage for Homebrew, PostgreSQL, build trees,
  and ccache;
* hosting on an OSGeo-administered buildbot network or on a trusted
  maintainer's desk;
* outbound network access to `woodie.osgeo.org`, plus a policy for inbound SSH
  or VPN administration if needed;
* power, storage replacement, OS upgrades, reboots, and physical recovery;
* a named administrator who keeps the Woodpecker agent, Homebrew dependencies,
  and macOS updates current.

OSGeo already hosts buildbots, so this is not a new infrastructure category.
The decision is whether the incremental Apple-hardware ownership and
administration cost is worth moving the Darwin signal from GitHub Actions into
Woodpecker.

## Option 4: Virtualized macOS

Virtualized macOS is only a compliant CI option when it runs on Apple hardware.
Apple's current macOS license permits up to two additional macOS instances in
virtual operating system environments on each Apple-branded computer the
licensee owns or controls and that is already running macOS, for software
development and testing. The same license language does not permit using those
virtualized copies for service-bureau, time-sharing, terminal-sharing, relay, or
similar services.

That permits a project-owned or hosted Apple machine to run macOS VMs for
isolation and reproducibility, subject to the license and the provider's terms.
It does not permit a Linux KVM host, ordinary cloud VM, or non-Apple bare-metal
host to run macOS as a Woodpecker substitute.

## Recommendation

Keep GitHub Actions authoritative for Darwin and record the Woodpecker row as
not coverable by configuration alone.

The maintainer-disagreeable version of the reasoning is simple: Darwin coverage
matters, but PostGIS does not currently have enough macOS-specific failure rate
or release dependency to justify adding a permanent Apple machine and a named
administrator. The cheapest credible hosted option is about `$100` to `$150` per
month before maintenance time. A project-owned Mac mini lowers recurring rental
cost but replaces it with physical hosting and administration. AWS is too
expensive for an always-on lane. Virtualized macOS is useful only after the
project already has Apple hardware, so it is not a way around the ownership
decision.

Revisit this decision if GitHub Actions macOS becomes unavailable for the
project, if a recurring Darwin-only defect starts escaping releases, or if OSGeo
accepts a named owner and budget for an Apple Woodpecker agent.
