---
title: "Developer Tools"
date: 2026-06-26
weight: 50
geekdocHidden: false
---

These tools support debugging, formatting, profiling, and coverage work. Install
the relevant packages through your development environment first, then use the
focused workflow for the task at hand.

## Core Dumps and logbt

The `ci/github/logbt` helper captures backtraces from crashes. On Ubuntu you
need elevated privileges once to reconfigure the kernel core pattern.

```sh
sudo ./ci/github/logbt --setup
sudo sysctl kernel.core_pattern
sudo mkdir -p /tmp/logbt-coredumps
sudo chmod 1777 /tmp/logbt-coredumps
ulimit -c unlimited
```

When running under constrained CI where you cannot change `kernel.core_pattern`,
skip the `logbt` setup and use the manual `gdb` workflow in
[Testing and debugging](testing/_index.md#backtraces).

## Formatting

Use [Coding style](style.md) for `git clang-format`, comments, and source naming
rules.

## Coverage

Use the coverage workflow in [Testing and debugging](testing/_index.md#coverage).

## Profiling

Profiling setup depends on the subsystem and platform. Keep flamegraph notes here
once a maintained PostGIS-specific workflow is written.
