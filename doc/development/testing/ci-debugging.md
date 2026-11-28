---
title: "Debugging CI Failures"
date: 2026-07-26
weight: 14
geekdocHidden: false
---

Open the status context's **Details** link and verify the run's commit SHA
against the branch or pull request head. Find the first failed step and read
its log before investigating downstream failures or requesting a rerun.

## Woodpecker

A Woodie pipeline is one run for a commit and event. It contains workflows,
each with its own steps. The Details link has the form:

```text
https://woodie.osgeo.org/repos/30/pipeline/<pipeline-number>/<workflow-number>
```

The matching pipeline record is:

```text
https://woodie.osgeo.org/api/repos/30/pipelines/<pipeline-number>
```

Check the commit, ref, event, and pipeline status. Find the workflow by its
`pid` (the last number in the Details link), then check its state and
`agent_id` if assigned. Open the failed step in the UI or use the step's
`id` from the workflow's `children` array:

```text
https://woodie.osgeo.org/api/repos/30/logs/<pipeline-number>/<step-id>
```

A numbered status such as `ci/woodpecker/pr/regress/19` refers to a matrix
entry. Read `.woodpecker/regress.yml` or `.woodpecker/qa.yml` at the tested
revision to identify the platform, PostgreSQL version, and test mode. See
[PR CI gating](ci-gating.md) to check whether the relevant tests ran.

Inspect the pipeline state before restarting it. Restarting a live pipeline
can cancel unfinished workflows. A canceled pipeline can leave pending forge
contexts, and a successful rerun only shows that the same failure did not
recur in that run.

## Diagnosing the First Failure

Start with the component that emitted the error. Compiler diagnostics after
checkout and SQL regression diffs after extension install usually point to
source or tests. Image pull errors, checkout failures, and missing emulation
support usually point to the registry, runner, or network.

* **Clone and DNS errors.** `curl 56`, `fatal: early EOF`, and resolver errors
  have several possible causes. To confirm Docker network pool exhaustion,
  look for `all predefined address pools have been fully subnetted` in the
  affected agent's daemon log. Verify that candidate networks belong to
  finished workflows before removing them. Reducing agent concurrency can
  relieve pool pressure during repair. On routed agents, check the settings
  on Woodpecker's user-defined Docker networks when testing an MTU hypothesis.
* **PostgreSQL startup or binding failures.** Read the server log for a
  competing postmaster, port, or socket. Older regress and QA workflows could
  reuse a packaged Debian cluster. Current workflows use
  `ci/start-postgresql.sh` to give each workflow its own data directory,
  socket, log, and port. Check the helper at the failing revision.
* **Image pulls.** Check the image name, tag, registry, and authentication.
  [CI images and runners](ci-images-runners.md) lists the image and
  runner owners.
* **Queued jobs and offline workers.** Check the workflow and agent state.
  When a job has no assigned agent, check runner eligibility and capacity
  before investigating build commands.

If a log points to an upstream dependency, record the first failing command,
dependency version, platform, and expected and observed behavior. Reduce the
case before filing it with the dependency's maintainers.

## Other CI Services

**GitHub Actions.** Open the job from the pull request checks or the Actions
run page. To print failed steps from the command line, run:

```sh
gh run view <run-id> --log-failed
```

Use the workflow at the tested revision under `.github/workflows/` to identify
the build and test commands.

**Jenkins.** Open the job's console log and check the branch parameter and
worker label. The PostGIS job scripts are under `ci/`. Worker provisioning
lives in the [PostGIS buildbots repository](https://gitea.osgeo.org/postgis/postgis-buildbots).

**OSS-Fuzz.** Check the service result and follow the local replay instructions
in the [PostGIS fuzzer README](https://gitea.osgeo.org/postgis/postgis/src/branch/master/fuzzers/README.md).
The [OSS-Fuzz project configuration](https://github.com/google/oss-fuzz/tree/master/projects/postgis)
points to the in-tree `fuzzers/build_oss_fuzz.sh`. Local corpus replay exercises
the checked-in seeds while OSS-Fuzz runs continuous fuzzing.

See [CI inventory standards](ci.md) for maintaining the service and job inventory.
