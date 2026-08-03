"""Collect PostGIS CI provider status and render terminal or static reports."""

import asyncio
import datetime as dt
import http.client
import json
import os
import pathlib
import subprocess
import tempfile
import threading
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET


SUCCESS = "success"
FAILURE = "failure"
IN_PROGRESS = "in_progress"
UNKNOWN = "unknown"
STALE = "stale"
STALE_PASSED = "stale-passed"
STALE_FAILED = "stale-fail"
DISABLED = "disabled"
NOT_APPLICABLE = "not_applicable"
JENKINS_STALE_QUEUE_HOURS = 4

STATUS_DISPLAY_ORDER = {
    FAILURE: 0,
    UNKNOWN: 1,
    STALE: 1,
    STALE_PASSED: 1,
    STALE_FAILED: 1,
    IN_PROGRESS: 2,
    SUCCESS: 3,
    NOT_APPLICABLE: 4,
    DISABLED: 5,
}

SYMBOLS = {
    SUCCESS: ("✅", "OK"),
    FAILURE: ("❌", "FAIL"),
    IN_PROGRESS: ("🔄", "RUN"),
    UNKNOWN: ("⚠️", "UNKNOWN"),
    STALE: ("⚠️", "STALE"),
    STALE_PASSED: ("⚠️", "STALE-OK"),
    STALE_FAILED: ("⚠️", "STALE-FAIL"),
    DISABLED: ("➖", "DISABLED"),
    NOT_APPLICABLE: ("➖", "N/A"),
}

COLORS = {
    SUCCESS: "\033[32m",
    FAILURE: "\033[31m",
    IN_PROGRESS: "\033[36m",
    UNKNOWN: "\033[33m",
    STALE: "\033[33m",
    STALE_PASSED: "\033[33m",
    STALE_FAILED: "\033[33m",
    DISABLED: "\033[2m",
    NOT_APPLICABLE: "\033[2m",
}
RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
_GITHUB_RUNS_CACHE = {}
_GITHUB_RUNS_INFLIGHT = {}
_GITHUB_RUNS_LOCK = threading.Lock()


class ConfigError(Exception):
    pass


class ProviderContentError(Exception):
    pass


def check_status_sort_key(check):
    status = check.get("status")
    if isinstance(status, str):
        return STATUS_DISPLAY_ORDER.get(status, STATUS_DISPLAY_ORDER[FAILURE])
    return STATUS_DISPLAY_ORDER[FAILURE]


def normalize_check_status(check):
    status = check.get("status")
    if isinstance(status, str) and status in STATUS_DISPLAY_ORDER:
        return check

    normalized = dict(check)
    normalized["reported_status"] = status
    normalized["status"] = FAILURE
    normalized["status_label"] = "Unsupported status"
    diagnostic = f"provider reported unsupported status {status!r}"
    if check.get("message"):
        diagnostic += f"; {check['message']}"
    normalized["message"] = diagnostic
    return normalized


RECOVERABLE_PROVIDER_ERRORS = (
    OSError,
    urllib.error.URLError,
    urllib.error.HTTPError,
    TimeoutError,
    ProviderContentError,
    json.JSONDecodeError,
    http.client.IncompleteRead,
)


def utc_now():
    return dt.datetime.now(dt.timezone.utc)


def parse_time(value):
    if value is None:
        return None
    if isinstance(value, (int, float)):
        # Jenkins timestamps are milliseconds; Woodpecker timestamps are seconds.
        if value > 10_000_000_000:
            value = value / 1000
        return dt.datetime.fromtimestamp(value, dt.timezone.utc)
    if isinstance(value, str):
        try:
            parsed = dt.datetime.fromisoformat(value.replace("Z", "+00:00"))
        except ValueError:
            return None
        if parsed.tzinfo is None:
            parsed = parsed.replace(tzinfo=dt.timezone.utc)
        return parsed.astimezone(dt.timezone.utc)
    return None


def age_text(value, now=None):
    timestamp = parse_time(value)
    if not timestamp:
        return None
    now = now or utc_now()
    delta = now - timestamp
    if delta.total_seconds() < 0:
        delta = dt.timedelta(0)
    minutes = int(delta.total_seconds() // 60)
    if minutes < 90:
        return f"{minutes}m ago"
    hours = minutes // 60
    if hours < 48:
        return f"{hours}h ago"
    return f"{hours // 24}d ago"


def status_symbol(status, color):
    glyph, plain = SYMBOLS.get(status, SYMBOLS[UNKNOWN])
    text = glyph if color else plain
    if color and status in COLORS:
        return f"{COLORS[status]}{text}{RESET}"
    return text


def terminal_style(text, color=None, bold=False, dim=False, enabled=True):
    if not enabled:
        return text
    pieces = []
    if bold:
        pieces.append(BOLD)
    if dim:
        pieces.append(DIM)
    if color:
        pieces.append(color)
    if not pieces:
        return text
    return "".join(pieces) + text + RESET


def terminal_status_label(status, color):
    _, plain = SYMBOLS.get(status, SYMBOLS[UNKNOWN])
    label = plain
    if status == IN_PROGRESS:
        label = "RUN"
    if status == NOT_APPLICABLE:
        label = "N/A"
    return terminal_style(f"{label:<8}", COLORS.get(status), bold=status in (FAILURE, SUCCESS), enabled=color)


def http_json(url, token=None, timeout=30):
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        raise ConfigError(f"unsupported CI API URL: {url}")
    headers = {
        "Accept": "application/json",
        "User-Agent": "postgis-ci-status/1.0",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    request = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(request, timeout=timeout) as response:
        content_type = response.headers.get("Content-Type", "")
        data = response.read()
    if "json" not in content_type.lower():
        raise ProviderContentError(f"expected JSON from {url}, got {content_type or 'unknown content type'}")
    return json.loads(data.decode("utf-8"))


def http_text(url, timeout=30):
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        raise ConfigError(f"unsupported CI URL: {url}")
    request = urllib.request.Request(url, headers={"User-Agent": "postgis-ci-status/1.0"})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        data = response.read()
    return data.decode("utf-8", errors="replace")


def render_template(value, branch):
    version = branch.get("version") or branch["label"]
    values = {
        "branch": branch["name"],
        "label": branch["label"],
        "version": version,
        "version_or_trunk": "trunk" if branch["name"] == "master" else version,
    }
    return value.replace("${version_or_trunk}", values["version_or_trunk"]).replace(
        "${version}", values["version"]
    ).replace("${branch}", values["branch"]).replace("${label}", values["label"])


def normalize_github_status(run):
    status = run.get("status")
    conclusion = run.get("conclusion")
    if status != "completed":
        if status in ("queued", "in_progress", "waiting", "requested", "pending"):
            return IN_PROGRESS
        return UNKNOWN
    if conclusion == "success":
        return SUCCESS
    if conclusion in ("failure", "timed_out", "startup_failure", "action_required"):
        return FAILURE
    if conclusion in ("cancelled", "skipped", "neutral"):
        return UNKNOWN
    return UNKNOWN


def github_workflow_matches(run, workflow):
    path = run.get("path") or ""
    return path == workflow or path.endswith(f"/{workflow}")


def github_workflow_url(repo, workflow, branch):
    query = urllib.parse.quote(f"branch:{branch['name']}", safe="")
    return f"https://github.com/{repo}/actions/workflows/{urllib.parse.quote(workflow)}?query={query}"


def github_badge_url(repo, workflow, branch):
    query = urllib.parse.urlencode({"branch": branch["name"]})
    return f"https://github.com/{repo}/actions/workflows/{urllib.parse.quote(workflow)}/badge.svg?{query}"


def parse_github_badge_status(svg):
    try:
        root = ET.fromstring(svg)
    except ET.ParseError as exc:
        raise ProviderContentError("cannot parse GitHub workflow badge SVG") from exc
    title = root.findtext("{http://www.w3.org/2000/svg}title") or root.findtext("title")
    if not title:
        raise ProviderContentError("GitHub workflow badge SVG has no status title")
    status = None
    for separator in (" - ", ": "):
        if separator in title:
            status = title.rsplit(separator, 1)[1].strip().lower()
            break
    if status is None:
        raise ProviderContentError("GitHub workflow badge SVG has no status title")
    if status == "passing":
        return SUCCESS, title
    if status in ("failing", "failure", "failed", "error"):
        return FAILURE, title
    if status in ("pending", "queued", "in progress", "waiting", "requested"):
        return IN_PROGRESS, title
    return UNKNOWN, title


def github_badge_check(check, branch, repo, workflow, timeout, api_error=None):
    badge_url = github_badge_url(repo, workflow, branch)
    status, title = parse_github_badge_status(http_text(badge_url, timeout=timeout))
    message = f"badge: {title}"
    if api_error:
        message = f"{message}; GitHub API unavailable: {api_error}"
    return make_result(
        check,
        branch,
        status,
        url=github_workflow_url(repo, workflow, branch),
        debug_url=badge_url,
        message=message,
    )


def github_runs_for_branch(repo, branch, token, timeout):
    cache_key = (repo, branch["name"], token or "")
    with _GITHUB_RUNS_LOCK:
        cached = _GITHUB_RUNS_CACHE.get(cache_key)
        if cached is not None:
            runs, url, exc = cached
            if exc is not None:
                raise exc
            return runs, url

        event = _GITHUB_RUNS_INFLIGHT.get(cache_key)
        if event is None:
            event = threading.Event()
            _GITHUB_RUNS_INFLIGHT[cache_key] = event
            owner = True
        else:
            owner = False

    if not owner:
        event.wait()
        with _GITHUB_RUNS_LOCK:
            runs, url, exc = _GITHUB_RUNS_CACHE[cache_key]
        if exc is not None:
            raise exc
        return runs, url

    query = urllib.parse.urlencode({"branch": branch["name"], "event": "push", "per_page": "100"})
    url = f"https://api.github.com/repos/{repo}/actions/runs?{query}"
    try:
        data = http_json(url, token=token, timeout=timeout)
    except Exception as exc:
        cached = ([], url, exc)
    else:
        cached = (data.get("workflow_runs", []), url, None)

    with _GITHUB_RUNS_LOCK:
        _GITHUB_RUNS_CACHE[cache_key] = cached
        _GITHUB_RUNS_INFLIGHT.pop(cache_key, None)
        event.set()

    runs, url, exc = cached
    if exc is not None:
        raise exc
    return runs, url


def github_actions_check(check, branch, timeout):
    workflow = check["workflow"]
    repo = check.get("repo", "postgis/postgis")
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    try:
        runs, debug_url = github_runs_for_branch(repo, branch, token, timeout)
    except RECOVERABLE_PROVIDER_ERRORS as exc:
        return github_badge_check(check, branch, repo, workflow, timeout, api_error=exc)
    runs = [run for run in runs if github_workflow_matches(run, workflow)]
    if not runs:
        try:
            return github_badge_check(check, branch, repo, workflow, timeout)
        except RECOVERABLE_PROVIDER_ERRORS:
            return make_result(
                check,
                branch,
                UNKNOWN,
                message="no workflow runs found",
                debug_url=debug_url,
                url=github_workflow_url(repo, workflow, branch),
            )

    current = runs[0]
    previous = next((run for run in runs[1:] if run.get("status") == "completed"), None)
    current_status = normalize_github_status(current)
    result = make_result(
        check,
        branch,
        current_status,
        url=current.get("html_url"),
        debug_url=debug_url,
        revision=current.get("head_sha"),
        completed_at=current.get("updated_at") or current.get("created_at"),
        message=current.get("display_title"),
    )
    if previous:
        result.update(previous_fields(normalize_github_status(previous), previous))
    return result


def normalize_woodpecker_status(value):
    mapping = {
        "success": SUCCESS,
        "failure": FAILURE,
        "error": FAILURE,
        "killed": FAILURE,
        "blocked": IN_PROGRESS,
        "declined": FAILURE,
        "running": IN_PROGRESS,
        "pending": IN_PROGRESS,
        "started": IN_PROGRESS,
        "created": IN_PROGRESS,
        "skipped": UNKNOWN,
    }
    return mapping.get(str(value).lower(), UNKNOWN)


def woodpecker_matches_branch(build, check, branch):
    if build.get("branch") != branch["name"]:
        return False

    event = check.get("event", "push")
    if event and build.get("event") != event:
        return False

    expected_ref = render_template(check.get("ref", "refs/heads/${branch}"), branch)
    return build.get("ref") in (None, expected_ref)


def woodpecker_build_sort_key(build):
    return (
        build.get("created") or build.get("created_at") or 0,
        build.get("started") or build.get("started_at") or 0,
        build.get("finished") or build.get("finished_at") or 0,
        build.get("number") or 0,
    )


def woodpecker_workflow_base_label(workflow, duplicate_names):
    """Return the human workflow label, disambiguating duplicate names by pid."""
    name = workflow.get("name") or f"workflow {workflow.get('pid') or workflow.get('id')}"
    pid = workflow.get("pid")
    if pid is not None and name in duplicate_names:
        return f"{name}/{pid}"
    return name


def woodpecker_failed_step_labels(workflow):
    """Return failed child step labels already included in a workflow record."""
    return [
        str(step.get("name") or f"step {step.get('pid') or step.get('id')}")
        for step in workflow.get("children") or []
        if normalize_woodpecker_status(step.get("state") or step.get("status")) == FAILURE
    ]


def woodpecker_workflow_label(workflow, duplicate_names, status):
    base_label = woodpecker_workflow_base_label(workflow, duplicate_names)
    if status != FAILURE:
        return base_label

    failed_children = woodpecker_failed_step_labels(workflow)
    if not failed_children:
        return base_label

    return f"{base_label} ({', '.join(failed_children)})"


def woodpecker_workflow_url(web_url, pipeline, workflow):
    if not web_url or not pipeline.get("number") or workflow.get("pid") is None:
        return None
    return f"{web_url}/pipeline/{pipeline['number']}/{workflow['pid']}"


def woodpecker_pipeline_url(web_url, pipeline):
    run_url = pipeline.get("link") or pipeline.get("url")
    if not run_url and web_url and pipeline.get("number"):
        run_url = f"{web_url}/pipeline/{pipeline['number']}"
    return run_url


def woodpecker_pipeline_detail_url(api_url, pipeline):
    if not pipeline.get("number"):
        return None
    return f"{api_url.rstrip('/')}/{pipeline['number']}"


def woodpecker_workflow_details(pipeline, web_url):
    workflows = pipeline.get("workflows") or []
    if not workflows:
        return None

    name_counts = {}
    for workflow in workflows:
        name = workflow.get("name") or f"workflow {workflow.get('pid') or workflow.get('id')}"
        name_counts[name] = name_counts.get(name, 0) + 1
    duplicate_names = {name for name, count in name_counts.items() if count > 1}

    buckets = [
        ("failed", []),
        ("running", []),
        ("unknown", []),
    ]
    by_bucket = dict(buckets)
    for workflow in sorted(workflows, key=lambda item: item.get("pid") or item.get("id") or 0):
        status = normalize_woodpecker_status(workflow.get("state") or workflow.get("status"))
        if status == SUCCESS:
            continue
        label = woodpecker_workflow_label(workflow, duplicate_names, status)
        if status == FAILURE:
            by_bucket["failed"].append((label, workflow))
        elif status == IN_PROGRESS:
            by_bucket["running"].append((label, workflow))
        else:
            by_bucket["unknown"].append((label, workflow))

    parts = []
    non_success = []
    for heading, items in buckets:
        if not items:
            continue
        labels = [label for label, workflow in items]
        parts.append(f"{heading}: {', '.join(labels)}")
        non_success.extend(workflow for label, workflow in items)
    if not parts:
        return None

    details = {"message": "; ".join(parts)}
    if len(non_success) == 1:
        details["url"] = woodpecker_workflow_url(web_url, pipeline, non_success[0])
    return details


def woodpecker_error_details(pipeline):
    errors = [
        error.get("message")
        for error in pipeline.get("errors") or []
        if isinstance(error, dict) and error.get("message") and not error.get("is_warning")
    ]
    if not errors:
        return None
    return {
        "message": "; ".join(errors),
        "status_label": "Config error",
    }


def woodpecker_leaf_steps(workflow):
    children = workflow.get("children") or []
    if children:
        return children
    return [workflow]


def woodpecker_killed_details(pipeline):
    workflows = pipeline.get("workflows") or []
    if not workflows:
        return None

    non_success = []
    killed_zero = []
    for workflow in workflows:
        for step in woodpecker_leaf_steps(workflow):
            status = normalize_woodpecker_status(step.get("state") or step.get("status"))
            if status == SUCCESS:
                continue
            non_success.append(step)
            if str(step.get("state") or step.get("status")).lower() == "killed" and step.get("exit_code") == 0:
                killed_zero.append(step)

    if not non_success or len(non_success) != len(killed_zero):
        return None

    labels = [
        str(step.get("name") or f"step {step.get('pid') or step.get('id')}")
        for step in killed_zero[:3]
    ]
    suffix = f" ({', '.join(labels)}" + (", ..." if len(killed_zero) > len(labels) else "") + ")"
    return {
        "message": f"agent lost: {plural(len(killed_zero), 'step')} killed at exit 0{suffix}",
        "status_label": "Agent lost",
    }


def woodpecker_check(check, branch, timeout):
    query = urllib.parse.urlencode({
        "branch": branch["name"],
        "event": check.get("event", "push"),
        "limit": "20",
    })
    api_url = check["api_url"]
    separator = "&" if "?" in api_url else "?"
    url = f"{api_url}{separator}{query}"
    builds = http_json(url, timeout=timeout)
    if isinstance(builds, dict):
        builds = builds.get("builds") or builds.get("pipelines") or builds.get("data") or []
    builds = [build for build in builds if woodpecker_matches_branch(build, check, branch)]
    if not builds:
        return make_result(check, branch, UNKNOWN, message="no Woodpecker builds found", debug_url=url)

    builds = sorted(builds, key=woodpecker_build_sort_key, reverse=True)
    current = builds[0]
    previous = next(
        (build for build in builds[1:] if normalize_woodpecker_status(build.get("status")) not in (IN_PROGRESS, UNKNOWN)),
        None,
    )
    web_url = check.get("web_url")
    run_url = woodpecker_pipeline_url(web_url, current)
    detail_url = woodpecker_pipeline_detail_url(api_url, current)
    needs_detail = "workflows" not in current
    if (
        normalize_woodpecker_status(current.get("status")) == FAILURE
        and str(current.get("status")).lower() == "error"
        and current.get("errors")
    ):
        needs_detail = False
    if detail_url and needs_detail:
        try:
            current = {**current, **http_json(detail_url, timeout=timeout)}
        except RECOVERABLE_PROVIDER_ERRORS:
            pass
    message = current.get("message")
    extra = {}
    if normalize_woodpecker_status(current.get("status")) != SUCCESS:
        details = None
        if str(current.get("status")).lower() == "error" and not (current.get("workflows") or []):
            details = woodpecker_error_details(current)
        if not details and str(current.get("status")).lower() == "failure":
            details = woodpecker_killed_details(current)
        if not details:
            details = woodpecker_workflow_details(current, web_url)
        if details:
            message = details["message"]
            run_url = details.get("url") or run_url
            extra.update({key: details[key] for key in ("status_label",) if key in details})
    result = make_result(
        check,
        branch,
        normalize_woodpecker_status(current.get("status")),
        url=run_url or web_url,
        debug_url=url,
        revision=current.get("commit"),
        completed_at=current.get("finished") or current.get("updated") or current.get("created"),
        message=message,
        **extra,
    )
    if previous:
        previous_url = woodpecker_pipeline_url(web_url, previous)
        if previous_url and not (previous.get("link") or previous.get("url")):
            previous = {**previous, "url": previous_url}
        result.update(previous_fields(normalize_woodpecker_status(previous.get("status")), previous))
    return result


def normalize_gitlab_status(value):
    mapping = {
        "success": SUCCESS,
        "failed": FAILURE,
        "canceled": UNKNOWN,
        "skipped": UNKNOWN,
        "manual": IN_PROGRESS,
        "scheduled": IN_PROGRESS,
        "created": IN_PROGRESS,
        "waiting_for_resource": IN_PROGRESS,
        "preparing": IN_PROGRESS,
        "pending": IN_PROGRESS,
        "running": IN_PROGRESS,
    }
    return mapping.get(str(value).lower(), UNKNOWN)


def gitlab_jobs_url(api_url, project, pipeline):
    pipeline_id = pipeline.get("id")
    if pipeline_id is None:
        return None
    query = urllib.parse.urlencode({"per_page": "100"})
    return f"{api_url.rstrip('/')}/projects/{project}/pipelines/{pipeline_id}/jobs?{query}"


def gitlab_job_summary(jobs):
    if not jobs:
        return None
    parts = []
    for job in sorted(jobs, key=lambda item: item.get("id") or 0):
        name = job.get("name") or f"job {job.get('id')}"
        status = job.get("status") or "unknown"
        reason = job.get("failure_reason")
        text = f"{name} {status}"
        if reason:
            if reason == "ci_quota_exceeded":
                reason = "no more compute minutes available"
            text = f"{text} ({reason})"
        parts.append(text)
    return "; ".join(parts)


def gitlab_status_from_jobs(status, jobs):
    if status != FAILURE:
        return status
    failed = [job for job in jobs if job.get("status") == "failed"]
    if failed and all(job.get("failure_reason") == "ci_quota_exceeded" for job in failed):
        return UNKNOWN
    return status


def git_ref_exists(ref):
    try:
        subprocess.run(
            ["git", "rev-parse", "--verify", "--quiet", f"{ref}^{{commit}}"],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except (OSError, subprocess.CalledProcessError):
        return False
    return True


def git_commit_distance(revision, ref):
    if not revision or not ref or not git_ref_exists(ref):
        return None
    try:
        ancestor = subprocess.run(
            ["git", "merge-base", "--is-ancestor", revision, ref],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if ancestor.returncode != 0:
            return None
        count = subprocess.check_output(
            ["git", "rev-list", "--count", f"{revision}..{ref}"],
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return None
    try:
        return int(count.strip())
    except ValueError:
        return None


def branch_compare_refs(branch):
    name = branch["name"]
    return [f"upstream/{name}", f"gitea/{name}", f"gh/{name}", f"origin/{name}", name]


def revision_distance_text(count, ref_name):
    if count is None:
        return None
    if count == 0:
        return f"at {ref_name}"
    return f"{plural(count, 'commit')} behind {ref_name}"


def gitlab_revision_distance(check, branch, revision):
    compare_ref = check.get("compare_ref")
    ref_candidates = [render_template(compare_ref, branch)] if compare_ref else branch_compare_refs(branch)
    for candidate in ref_candidates:
        count = git_commit_distance(revision, candidate)
        if count is not None:
            return count, branch["name"]
    return None, None


def result_revision_distance(config, result):
    revision = result.get("revision")
    if not revision:
        return None, None
    branch = next((item for item in config["branches"] if item["name"] == result["branch"]), None)
    if branch is None:
        return None, None
    for candidate in branch_compare_refs(branch):
        count = git_commit_distance(revision, candidate)
        if count is not None:
            return count, branch["name"]
    return None, None


def gitlab_check(check, branch, timeout):
    api_url = check.get("api_url", "https://gitlab.com/api/v4")
    project = urllib.parse.quote(check.get("project", "postgis/postgis"), safe="")
    ref = render_template(check.get("ref", "${branch}"), branch)
    query = urllib.parse.urlencode({
        "ref": ref,
        "per_page": check.get("per_page", 20),
        "order_by": "id",
        "sort": "desc",
    })
    url = f"{api_url.rstrip('/')}/projects/{project}/pipelines?{query}"
    pipelines = http_json(url, timeout=timeout)
    if isinstance(pipelines, dict):
        pipelines = pipelines.get("pipelines") or pipelines.get("data") or []
    pipelines = [pipeline for pipeline in pipelines if pipeline.get("ref") == ref]
    if not pipelines:
        return make_result(check, branch, UNKNOWN, message=f"no GitLab pipelines found for {ref}", debug_url=url)

    current = pipelines[0]
    previous = next((pipeline for pipeline in pipelines[1:] if normalize_gitlab_status(pipeline.get("status")) not in (IN_PROGRESS, UNKNOWN)), None)
    web_url = current.get("web_url") or render_template(check.get("web_url", ""), branch) or None
    current_status = normalize_gitlab_status(current.get("status"))
    message = f"pipeline {current.get('iid') or current.get('id')}"
    jobs_url = gitlab_jobs_url(api_url, project, current)
    if jobs_url:
        try:
            jobs = http_json(jobs_url, timeout=timeout)
        except RECOVERABLE_PROVIDER_ERRORS as exc:
            message = f"{message}; job details unavailable: {exc}"
        else:
            if isinstance(jobs, dict):
                jobs = jobs.get("jobs") or jobs.get("data") or []
            current_status = gitlab_status_from_jobs(current_status, jobs)
            summary = gitlab_job_summary(jobs)
            if summary:
                message = f"{message}; {summary}"
    result = make_result(
        check,
        branch,
        current_status,
        url=web_url,
        debug_url=url,
        revision=current.get("sha"),
        completed_at=current.get("updated_at") or current.get("created_at"),
        message=message,
    )
    distance_count, distance_ref = gitlab_revision_distance(check, branch, current.get("sha"))
    distance_text = revision_distance_text(distance_count, distance_ref)
    if distance_text:
        result["revision_commits_behind"] = distance_count
        result["revision_compare_ref"] = distance_ref
        result["revision_distance"] = distance_text
    if previous:
        result.update(previous_fields(normalize_gitlab_status(previous.get("status")), previous))
    return result


def normalize_jenkins_status(build):
    if build.get("building"):
        return IN_PROGRESS
    result = build.get("result")
    if result == "SUCCESS":
        return SUCCESS
    if result in ("FAILURE", "UNSTABLE"):
        return FAILURE
    if result in ("ABORTED", "NOT_BUILT"):
        return UNKNOWN
    return UNKNOWN


def jenkins_parameters(build):
    found = {}
    for action in build.get("actions") or []:
        for param in action.get("parameters") or []:
            name = param.get("name")
            if name:
                found[name] = str(param.get("value", ""))
    return found


def jenkins_revision(build):
    for action in build.get("actions") or []:
        revision = action.get("lastBuiltRevision") or {}
        sha1 = revision.get("SHA1")
        if sha1:
            return sha1
    params = jenkins_parameters(build)
    for name in ("after", "BRANCH", "commit", "GIT_COMMIT"):
        value = params.get(name)
        if value and len(value) == 40 and all(ch in "0123456789abcdefABCDEF" for ch in value):
            return value
    return None


def jenkins_matches_branch(build, check, branch):
    branch_parameter = check.get("branch_parameter")
    if not branch_parameter:
        return True
    expected = f"refs/heads/{branch['name']}"
    params = jenkins_parameters(build)
    return any(value == expected for name, value in params.items() if name == branch_parameter or name.lower() == branch_parameter.lower())


def jenkins_queue_url(job_url):
    parsed = urllib.parse.urlparse(job_url)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        raise ConfigError(f"cannot build Jenkins queue URL for {job_url}")
    return urllib.parse.urlunparse((parsed.scheme, parsed.netloc, "/queue/api/json", "", "", ""))


def jenkins_project_url(job_url):
    parsed = urllib.parse.urlparse(job_url)
    path = parsed.path.rstrip("/") + "/"
    if path.startswith("/view/") and "/job/" in path:
        path = "/job/" + path.split("/job/", 1)[1]
    if "/label=" in path:
        path = path.split("/label=", 1)[0].rstrip("/") + "/"
    return urllib.parse.urlunparse((parsed.scheme, parsed.netloc, path, "", "", ""))


def jenkins_queue_items(job_url, timeout):
    tree = (
        "items[id,task[name,url],why,params,inQueueSince,url,blocked,buildable,stuck,"
        "actions[parameters[name,value]]]"
    )
    url = jenkins_queue_url(job_url) + "?" + urllib.parse.urlencode({"tree": tree})
    return http_json(url, timeout=timeout).get("items") or []


def jenkins_queue_item_matches(item, job_url, check, branch):
    task = item.get("task") or {}
    task_url = (task.get("url") or "").rstrip("/") + "/"
    if task_url != jenkins_project_url(job_url):
        return False
    branch_parameter = check.get("branch_parameter")
    if not branch_parameter:
        return True
    expected = f"refs/heads/{branch['name']}"
    params = jenkins_parameters(item)
    return any(value == expected for name, value in params.items() if name == branch_parameter or name.lower() == branch_parameter.lower())


def queued_jenkins_revision(item):
    params = jenkins_parameters(item)
    for name in ("after", "BRANCH", "commit", "GIT_COMMIT"):
        value = params.get(name)
        if value and len(value) == 40 and all(ch in "0123456789abcdefABCDEF" for ch in value):
            return value
    return None


def jenkins_queued_status_label(item):
    queued_at = parse_time(item.get("inQueueSince"))
    if not queued_at:
        return "Queued"
    age = utc_now() - queued_at
    if age.total_seconds() > JENKINS_STALE_QUEUE_HOURS * 3600:
        return "Queued stale"
    if item.get("stuck"):
        return "Queued stuck"
    return "Queued"


def jenkins_queue_item_rank(item, branch):
    revision = queued_jenkins_revision(item)
    is_current = False
    if revision:
        for ref in branch_compare_refs(branch):
            distance = git_commit_distance(revision, ref)
            if distance == 0:
                is_current = True
                break
    return (
        0 if is_current else 1,
        -(item.get("inQueueSince") or 0),
        item.get("id") or 0,
    )


def jenkins_queued_check(check, branch, job_url, timeout):
    if not check.get("branch_parameter"):
        return None
    try:
        queued = [
            item for item in jenkins_queue_items(job_url, timeout)
            if jenkins_queue_item_matches(item, job_url, check, branch)
        ]
    except RECOVERABLE_PROVIDER_ERRORS:
        return None
    if not queued:
        return None
    item = sorted(queued, key=lambda value: jenkins_queue_item_rank(value, branch))[0]
    params = jenkins_parameters(item)
    message = f"queued item {item.get('id')}"
    if item.get("why"):
        message = f"{message}: {item['why']}"
    return make_result(
        check,
        branch,
        IN_PROGRESS,
        url=job_url,
        debug_url=jenkins_queue_url(job_url),
        revision=params.get("after") or params.get("BRANCH"),
        completed_at=item.get("inQueueSince"),
        message=message,
        status_label=jenkins_queued_status_label(item),
    )


def jenkins_builds(job_url, check, timeout):
    try:
        limit = int(check.get("build_scan_limit", 200 if check.get("branch_parameter") else 25))
    except (TypeError, ValueError) as exc:
        raise ConfigError(f"invalid build_scan_limit for {check['name']}") from exc
    if limit <= 0:
        raise ConfigError(f"invalid build_scan_limit for {check['name']}: {limit}")
    page_size = min(50, limit)
    builds = []
    for offset in range(0, limit, page_size):
        page_limit = min(page_size, limit - offset)
        page_end = offset + page_limit
        tree = (
            "builds[number,url,result,building,timestamp,duration,"
            f"actions[parameters[name,value],lastBuiltRevision[SHA1,branch[name]]]]{{{offset},{page_end}}}"
        )
        url = job_url + "api/json?" + urllib.parse.urlencode({"tree": tree})
        data = http_json(url, timeout=timeout)
        page = data.get("builds", [])
        builds.extend(page)
        if len(page) < page_limit:
            break
    return builds


def jenkins_matrix_configurations(job_url, timeout):
    tree = (
        "activeConfigurations[name,url,color,"
        "lastBuild[number,url,result,building,timestamp,duration],"
        "lastCompletedBuild[number,url,result,timestamp],"
        "lastFailedBuild[number,url,result,timestamp]]"
    )
    url = job_url + "api/json?" + urllib.parse.urlencode({"tree": tree})
    return http_json(url, timeout=timeout).get("activeConfigurations") or []


def jenkins_matrix_axes(configuration):
    axes = {}
    for item in str(configuration.get("name") or "").split(","):
        if "=" not in item:
            continue
        name, value = item.split("=", 1)
        axes[name.strip()] = value.strip()
    return axes


def jenkins_matrix_axis_text(name, value):
    if name == "PG_VER":
        return f"PG{value}"
    if name == "label":
        return value
    return f"{name}={value}"


def jenkins_matrix_summary_axes(configurations):
    parsed = [jenkins_matrix_axes(configuration) for configuration in configurations]
    priority = (
        "label",
        "PG_VER",
        "POSTGIS_TAG",
        "OS_BUILD",
        "GEOS_VER",
        "GDAL_VER",
        "GCC_TYPE",
        "SFCGAL_VER",
        "CGAL_VER",
    )
    varying = []
    for name in priority:
        values = {axes.get(name) for axes in parsed if axes.get(name)}
        if len(values) > 1:
            varying.append(name)
    return varying[:3]


def jenkins_matrix_configuration_label(configuration, selected):
    axes = jenkins_matrix_axes(configuration)
    parts = [
        jenkins_matrix_axis_text(name, axes[name])
        for name in selected
        if name in axes
    ]
    if parts:
        return ", ".join(parts)
    return configuration.get("name") or configuration.get("url") or "configuration"


def jenkins_matrix_details(job_url, timeout):
    try:
        configurations = jenkins_matrix_configurations(job_url, timeout)
    except RECOVERABLE_PROVIDER_ERRORS:
        return None
    if len(configurations) < 2:
        return None

    selected = jenkins_matrix_summary_axes(configurations)

    by_status = {
        FAILURE: [],
        IN_PROGRESS: [],
        UNKNOWN: [],
    }
    for configuration in configurations:
        build = configuration.get("lastBuild") or {}
        status = normalize_jenkins_status(build)
        if status == SUCCESS:
            continue
        label = jenkins_matrix_configuration_label(configuration, selected)
        by_status.setdefault(status, []).append((label, build))

    parts = []
    for status, prefix in (
        (FAILURE, "failed"),
        (IN_PROGRESS, "running"),
        (UNKNOWN, "unknown"),
    ):
        items = by_status.get(status) or []
        if not items:
            continue
        item_labels = [label for label, build in items]
        parts.append(f"{prefix}: {', '.join(item_labels)}")
    if not parts:
        return None

    non_success = [
        item
        for status in (FAILURE, IN_PROGRESS, UNKNOWN)
        for item in by_status.get(status) or []
    ]
    url = non_success[0][1].get("url") if len(non_success) == 1 else None
    return {
        "message": "; ".join(parts),
        "url": url,
    }


def jenkins_badge_url(job_url, check, branch):
    parsed = urllib.parse.urlparse(job_url)
    if parsed.scheme not in ("http", "https") or not parsed.netloc or "/job/" not in parsed.path:
        raise ConfigError(f"cannot build Jenkins badge URL for {job_url}")
    job_path = parsed.path.split("/job/", 1)[1].strip("/")
    query = {"job": job_path}
    branch_parameter = check.get("branch_parameter")
    if branch_parameter:
        query["build"] = f"last:${{params.{branch_parameter}=refs/heads/{branch['name']}}}"
    return urllib.parse.urlunparse((parsed.scheme, parsed.netloc, "/buildStatus/icon", "", urllib.parse.urlencode(query), ""))


def parse_jenkins_badge_status(svg):
    try:
        root = ET.fromstring(svg)
    except ET.ParseError as exc:
        raise ProviderContentError("cannot parse Jenkins badge SVG") from exc
    texts = [
        "".join(element.itertext()).strip().lower()
        for element in root.findall(".//{http://www.w3.org/2000/svg}text")
    ]
    labels = [text for text in texts if text and text != "build"]
    label = labels[-1] if labels else ""
    if label == "passing":
        return SUCCESS, label
    if label in ("failing", "failure", "failed", "error"):
        return FAILURE, label
    if label in ("pending", "queued", "in progress", "running"):
        return IN_PROGRESS, label
    if label in ("not run", "not built"):
        return UNKNOWN, label
    return UNKNOWN, label or "unknown"


def jenkins_badge_check(check, branch, job_url, timeout, api_error=None):
    badge_url = jenkins_badge_url(job_url, check, branch)
    status, label = parse_jenkins_badge_status(http_text(badge_url, timeout=timeout))
    message = f"badge: {label}"
    if api_error:
        message = f"{message}; Jenkins API unavailable: {api_error}"
    extra = {}
    if label in ("not run", "not built"):
        extra["status_label"] = "Not run"
    return make_result(
        check,
        branch,
        status,
        url=job_url,
        debug_url=badge_url,
        message=message,
        **extra,
    )


def jenkins_check(check, branch, timeout):
    job_url = render_template(check["job_url"], branch).rstrip("/") + "/"
    queued_result = jenkins_queued_check(check, branch, job_url, timeout)
    try:
        builds = [
            build for build in jenkins_builds(job_url, check, timeout)
            if jenkins_matches_branch(build, check, branch)
        ]
    except RECOVERABLE_PROVIDER_ERRORS as exc:
        return jenkins_badge_check(check, branch, job_url, timeout, api_error=exc)
    if not builds:
        if queued_result:
            return queued_result
        badge_result = jenkins_badge_check(check, branch, job_url, timeout)
        if badge_result["status"] != UNKNOWN or badge_result.get("status_label") == "Not run":
            return badge_result
        return make_result(check, branch, UNKNOWN, url=job_url, debug_url=job_url + "api/json", message="no matching Jenkins builds found")

    current = builds[0]
    previous = next((build for build in builds[1:] if not build.get("building")), None)
    if queued_result and not current.get("building"):
        queued_result.update(previous_fields(normalize_jenkins_status(current), current))
        return queued_result
    result = make_result(
        check,
        branch,
        normalize_jenkins_status(current),
        url=current.get("url") or job_url,
        debug_url=job_url + "api/json",
        revision=jenkins_revision(current),
        completed_at=current.get("timestamp"),
        message=f"build {current.get('number')}",
    )
    if result["status"] != SUCCESS:
        details = jenkins_matrix_details(job_url, timeout)
        if details:
            result["message"] = f"{result['message']}; {details['message']}"
            if details.get("url"):
                result["url"] = details["url"]
    if previous:
        result.update(previous_fields(normalize_jenkins_status(previous), previous))
    return result


def disabled_check(check, branch, timeout):
    status = DISABLED if check.get("status") == "disabled" else NOT_APPLICABLE
    url = render_template(check.get("url", ""), branch) or None
    return make_result(check, branch, status, url=url, message=check.get("message"))


PROVIDERS = {
    "github_actions": github_actions_check,
    "gitlab": gitlab_check,
    "woodpecker": woodpecker_check,
    "jenkins": jenkins_check,
    "disabled": disabled_check,
}


def make_result(check, branch, status, **extra):
    result = {
        "branch": branch["name"],
        "branch_label": branch["label"],
        "check": check["name"],
        "provider": check["provider"],
        "required": bool(check.get("required", True)),
        "status": status,
    }
    for key, value in extra.items():
        if value is not None:
            result[key] = value
    return result


def previous_fields(status, item):
    return {
        "previous_completed_status": status,
        "previous_completed_url": item.get("html_url") or item.get("web_url") or item.get("url") or item.get("link"),
        "previous_completed_revision": item.get("head_sha") or item.get("sha") or item.get("commit"),
    }


def result_from_exception(check, branch, exc):
    debug_url = check.get("url") or check.get("api_url") or check.get("job_url")
    if debug_url:
        debug_url = render_template(debug_url, branch)
    return make_result(check, branch, UNKNOWN, message=str(exc), debug_url=debug_url)


def index_status_cache(data):
    if data is None:
        return None
    if not isinstance(data, dict) or not isinstance(data.get("branches"), list):
        raise ConfigError("status cache must contain a branches array")
    checks = {}
    for branch in data["branches"]:
        if not isinstance(branch, dict):
            continue
        branch_name = branch.get("name")
        for result in branch.get("checks") or []:
            if not isinstance(result, dict):
                continue
            check_name = result.get("check")
            if branch_name and check_name:
                checks[(branch_name, check_name)] = result
    return {
        "generated_at": data.get("generated_at"),
        "checks": checks,
    }


def load_status_cache(path):
    if not path:
        return None
    try:
        with open(path, "r", encoding="utf-8") as handle:
            return index_status_cache(json.load(handle))
    except FileNotFoundError:
        return None
    except OSError as exc:
        raise ConfigError(f"cannot read status cache {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ConfigError(f"invalid JSON in status cache {path}: {exc}") from exc


def resolve_cache_heads(config, work, cache, timeout):
    if not cache:
        return {}
    remote = config.get("cache_head_remote")
    if not remote:
        return {}
    branch_names = sorted({
        branch["name"]
        for branch, _check in work
        if any(
            cached.get("status") == SUCCESS
            for (cached_branch, _cached_check), cached in cache["checks"].items()
            if cached_branch == branch["name"]
        )
    })
    if not branch_names:
        return {}
    refs = [f"refs/heads/{name}" for name in branch_names]
    try:
        completed = subprocess.run(
            ["git", "ls-remote", "--exit-code", "--heads", remote, *refs],
            check=True,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return {}

    heads = {}
    wanted = set(refs)
    for line in completed.stdout.splitlines():
        fields = line.split()
        if len(fields) != 2 or fields[1] not in wanted:
            continue
        heads[fields[1].removeprefix("refs/heads/")] = fields[0]
    return heads


def cached_success_result(branch, check, cache, cache_heads):
    if not cache:
        return None
    cached = cache["checks"].get((branch["name"], check["name"]))
    if not cached or cached.get("status") != SUCCESS:
        return None
    if cached.get("branch") != branch["name"] or cached.get("check") != check["name"]:
        return None
    if cached.get("provider") != check.get("provider"):
        return None
    if cached.get("required") != bool(check.get("required", True)):
        return None
    revision = cached.get("revision")
    if not revision or revision != cache_heads.get(branch["name"]):
        return None

    result = dict(cached)
    result.update({
        "branch": branch["name"],
        "branch_label": branch["label"],
        "check": check["name"],
        "provider": check.get("provider"),
        "required": bool(check.get("required", True)),
        "cached": True,
        "cached_at": cache.get("generated_at"),
    })
    message = result.get("message") or "successful run"
    suffix = " (cached; unchanged revision)"
    result["message"] = message if message.endswith(suffix) else f"{message}{suffix}"
    return result


def stale_after_hours(config, check):
    value = check.get("stale_after_hours", config.get("stale_after_hours"))
    if value is None:
        return None
    try:
        return float(value)
    except (TypeError, ValueError):
        raise ConfigError(f"invalid stale_after_hours for {check['name']}: {value}")


def apply_staleness(result, config, check):
    threshold = stale_after_hours(config, check)
    distance_count, distance_ref = None, None
    if result["status"] != IN_PROGRESS:
        distance_count, distance_ref = result_revision_distance(config, result)
    if result["status"] not in (IN_PROGRESS, SUCCESS) and distance_count and distance_count > 0:
        stale = dict(result)
        stale["revision_commits_behind"] = distance_count
        stale["revision_compare_ref"] = distance_ref
        stale["revision_distance"] = revision_distance_text(distance_count, distance_ref)
        stale["stale_base_status"] = result["status"]
        stale["status"] = stale_status(result["status"])
        stale["status_label"] = stale_status_label(result["status"])
        stale["message"] = f"{result.get('message', 'CI run')} ({stale['revision_distance']})"
        return stale
    if result["status"] != SUCCESS:
        return result
    completed_at = parse_time(result.get("completed_at"))
    if threshold is None or completed_at is None:
        return result
    age = utc_now() - completed_at
    if age.total_seconds() <= threshold * 3600:
        return result
    if distance_count == 0:
        return result
    stale = dict(result)
    distance_text = revision_distance_text(distance_count, distance_ref)
    if distance_text:
        stale["revision_commits_behind"] = distance_count
        stale["revision_compare_ref"] = distance_ref
        stale["revision_distance"] = distance_text
    stale["stale_base_status"] = result["status"]
    stale["status"] = stale_status(result["status"])
    stale["status_label"] = stale_status_label(result["status"])
    stale["message"] = f"{result.get('message', 'successful run')} (older than {threshold:g}h)"
    return stale


def branch_checks(config, selected_branch, include_eol=False):
    branches = config["branches"]
    if selected_branch:
        branches = [branch for branch in branches if branch["name"] == selected_branch or branch["label"] == selected_branch]
        if not branches:
            raise ConfigError(f"unknown branch: {selected_branch}")
    elif not include_eol:
        branches = [branch for branch in branches if not branch.get("eol")]

    for branch in branches:
        for check in config["checks"]:
            only = check.get("branches")
            except_branches = check.get("except_branches", [])
            if only and branch["name"] not in only and branch["label"] not in only:
                continue
            if branch["name"] in except_branches or branch["label"] in except_branches:
                continue
            yield branch, check


def default_concurrency():
    return min(32, max(1, os.cpu_count() or 1))


async def collect_status_async(config, selected_branch=None, include_eol=False, timeout=30, cache=None):
    work = list(branch_checks(config, selected_branch, include_eol))
    cache_heads = await asyncio.to_thread(resolve_cache_heads, config, work, cache, timeout)
    semaphore = asyncio.Semaphore(min(default_concurrency(), max(1, len(work))))

    def collect_one(item):
        branch, check = item
        cached = cached_success_result(branch, check, cache, cache_heads)
        if cached:
            return apply_staleness(cached, config, check)
        provider = PROVIDERS.get(check.get("provider"))
        if provider is None:
            raise ConfigError(f"unsupported provider for {check['name']}: {check.get('provider')}")
        try:
            return apply_staleness(provider(check, branch, timeout), config, check)
        except RECOVERABLE_PROVIDER_ERRORS as exc:
            return result_from_exception(check, branch, exc)

    async def collect_one_async(item):
        async with semaphore:
            return await asyncio.to_thread(collect_one, item)

    results = await asyncio.gather(*(collect_one_async(item) for item in work))
    return aggregate(config, results)


def collect_status(config, selected_branch=None, include_eol=False, timeout=30, cache=None):
    return asyncio.run(collect_status_async(config, selected_branch, include_eol, timeout, cache))


def aggregate(config, results):
    branches = []
    by_branch = {}
    branch_order = {branch["name"]: branch for branch in config["branches"]}
    for result in results:
        result = normalize_check_status(result)
        by_branch.setdefault(result["branch"], []).append(result)

    for branch_name, checks in by_branch.items():
        checks.sort(key=check_status_sort_key)
        required = [check for check in checks if check.get("required") and check["status"] not in (DISABLED, NOT_APPLICABLE)]
        if any(check["status"] == FAILURE for check in required):
            status = FAILURE
        elif any(check["status"] == UNKNOWN or is_stale_status(check["status"]) for check in required):
            status = UNKNOWN
        elif any(check["status"] == IN_PROGRESS for check in required):
            status = IN_PROGRESS
        elif required:
            status = SUCCESS
        else:
            status = NOT_APPLICABLE

        failures = sum(1 for check in required if check["status"] == FAILURE)
        label = branch_order.get(branch_name, {}).get("label", branch_name)
        branches.append({
            "name": branch_name,
            "label": label,
            "status": status,
            "failures": failures,
            "checks": checks,
        })

    order = {branch["name"]: index for index, branch in enumerate(config["branches"])}
    branches.sort(key=lambda item: order.get(item["name"], 999))
    return {
        "generated_at": utc_now().isoformat(timespec="seconds"),
        "branches": branches,
    }


def check_counts(branch):
    required = [
        check for check in branch["checks"]
        if check.get("required") and check["status"] not in (DISABLED, NOT_APPLICABLE)
    ]
    counts = {
        SUCCESS: 0,
        FAILURE: 0,
        IN_PROGRESS: 0,
        UNKNOWN: 0,
        STALE: 0,
        STALE_PASSED: 0,
        STALE_FAILED: 0,
    }
    for check in required:
        counts[check["status"]] = counts.get(check["status"], 0) + 1
    counts["required"] = len(required)
    counts["informational"] = len(branch["checks"]) - len(required)
    return counts


def stale_status_label(base_status):
    return {
        SUCCESS: "Stale passed",
        FAILURE: "Stale fail",
        UNKNOWN: "Stale unknown",
    }.get(base_status, "Stale")


def stale_status(base_status):
    return {
        SUCCESS: STALE_PASSED,
        FAILURE: STALE_FAILED,
    }.get(base_status, STALE)


def is_stale_status(status):
    return status in (STALE, STALE_PASSED, STALE_FAILED)


def stale_count_bucket(check):
    if not is_stale_status(check["status"]):
        return None
    base_status = check.get("stale_base_status")
    if check["status"] == STALE_PASSED:
        return SUCCESS
    if check["status"] == STALE_FAILED:
        return FAILURE
    if base_status in (SUCCESS, FAILURE):
        return base_status
    return UNKNOWN


def stale_check_counts(branch):
    counts = {
        SUCCESS: 0,
        FAILURE: 0,
        UNKNOWN: 0,
    }
    for check in branch["checks"]:
        if not check.get("required"):
            continue
        bucket = stale_count_bucket(check)
        if bucket:
            counts[bucket] += 1
    return counts


def stale_summary_parts(branch):
    counts = stale_check_counts(branch)
    return nonzero_parts(
        (counts[SUCCESS], f"{counts[SUCCESS]} stale-passed"),
        (counts[FAILURE], f"{counts[FAILURE]} stale-fail"),
        (counts[UNKNOWN], f"{counts[UNKNOWN]} stale-unknown"),
    )


def plural(count, word):
    return f"{count} {word}" + ("" if count == 1 else "s")


def nonzero_parts(*items):
    return [text for count, text in items if count]


def summary_text(branch):
    status = branch["status"]
    counts = check_counts(branch)
    stale_parts = stale_summary_parts(branch)
    if status == SUCCESS:
        return f"all {counts['required']} required CI checks OK"
    if status == FAILURE:
        parts = nonzero_parts(
            (counts[SUCCESS], f"{counts[SUCCESS]} OK"),
            (counts[FAILURE], plural(counts[FAILURE], "failure")),
            (counts[IN_PROGRESS], f"{counts[IN_PROGRESS]} running"),
            (counts[UNKNOWN], f"{counts[UNKNOWN]} unknown"),
        )
        parts.extend(stale_parts)
        return "; ".join(parts)
    if status == IN_PROGRESS:
        previous = previous_summary(branch["checks"])
        parts = nonzero_parts(
            (counts[SUCCESS], f"{counts[SUCCESS]} OK"),
            (counts[IN_PROGRESS], f"{counts[IN_PROGRESS]} running"),
            (counts[UNKNOWN], f"{counts[UNKNOWN]} unknown"),
        )
        parts.extend(stale_parts)
        prefix = "no known failures; " + ", ".join(parts)
        return f"{prefix}; {previous}" if previous else prefix
    if status == UNKNOWN:
        parts = nonzero_parts(
            (counts[SUCCESS], f"{counts[SUCCESS]} OK"),
            (counts[IN_PROGRESS], f"{counts[IN_PROGRESS]} running"),
            (counts[UNKNOWN], f"{counts[UNKNOWN]} unknown"),
        )
        parts.extend(stale_parts)
        return "no known failures; " + ", ".join(parts)
    return "no required CI configured"


def previous_summary(checks):
    running = [check for check in checks if check["status"] == IN_PROGRESS and check.get("required")]
    if not running:
        return None
    previous = {check.get("previous_completed_status") for check in running if check.get("previous_completed_status")}
    if previous == {SUCCESS}:
        return "previous OK, new run in progress"
    if previous == {FAILURE}:
        return "previous failed, new run in progress"
    if previous:
        return "previous result mixed, new run in progress"
    return "in progress, no previous result known"


def interesting_checks(branch, verbose=False):
    if verbose:
        return branch["checks"]
    has_visible_jenkins_problem = any(
        check.get("provider") == "jenkins"
        and (check["status"] in (FAILURE, UNKNOWN, IN_PROGRESS, NOT_APPLICABLE) or is_stale_status(check["status"]))
        for check in branch["checks"]
    )
    visible = []
    for check in branch["checks"]:
        if (
            (check["status"] in (FAILURE, UNKNOWN, IN_PROGRESS) or is_stale_status(check["status"]))
            or (check["status"] == NOT_APPLICABLE and check.get("provider") == "jenkins")
            or (
                has_visible_jenkins_problem
                and check.get("provider") == "jenkins"
                and check["status"] == SUCCESS
            )
        ) and check["status"] != DISABLED:
            visible.append(check)
    visible.sort(key=check_status_sort_key)
    return visible


def safe_http_href(value):
    if not value:
        return ""
    parsed = urllib.parse.urlparse(value)
    if parsed.scheme in ("http", "https") and parsed.netloc:
        return value
    return ""


def result_url(check):
    return safe_http_href(check.get("url") or check.get("debug_url") or "")


def terminal_link(text, url, enabled):
    if enabled and url:
        return f"\033]8;;{url}\033\\{text}\033]8;;\033\\"
    return text


def terminal_count(value, status, color):
    text = f"{value:>3}"
    if not value:
        return terminal_style(text, dim=True, enabled=color)
    return terminal_style(text, COLORS.get(status), bold=status in (FAILURE, SUCCESS), enabled=color)


def terminal_header(data, use_color):
    generated = data.get("generated_at", "")
    print(terminal_style("PostGIS CI status", bold=True, enabled=use_color))
    if generated:
        print(terminal_style(f"generated {generated}", dim=True, enabled=use_color))
    print()


def print_branch_table(branches, use_color):
    print(terminal_style("Branches", bold=True, enabled=use_color))
    header = f"{'branch':<10} {'status':<8} {'ok':>3} {'fail':>4} {'run':>3} {'unk':>3}  summary"
    print(terminal_style(header, dim=True, enabled=use_color))
    print(terminal_style("-" * len(header), dim=True, enabled=use_color))
    for branch in branches:
        counts = check_counts(branch)
        unknown = counts[UNKNOWN] + counts[STALE]
        label = f"{branch['label']:<10}"
        print(
            f"{label} "
            f"{terminal_status_label(branch['status'], use_color)} "
            f"{terminal_count(counts[SUCCESS], SUCCESS, use_color)} "
            f"{terminal_count(counts[FAILURE], FAILURE, use_color)} "
            f"{terminal_count(counts[IN_PROGRESS], IN_PROGRESS, use_color)} "
            f"{terminal_count(unknown, UNKNOWN, use_color)}  "
            f"{summary_text(branch)}"
        )


def terminal_field(label, value, use_color):
    label_text = f"{label + ':':<10}"
    return f"  {terminal_style(label_text, dim=True, enabled=use_color)} {value}"


def print_terminal(data, use_color=True, verbose=False):
    terminal_header(data, use_color)
    print_branch_table(data["branches"], use_color)

    details = [(branch, check) for branch in data["branches"] for check in interesting_checks(branch, verbose)]
    if not details:
        return
    print()
    title = "Checks" if verbose else "Problem checks"
    print(terminal_style(title, bold=True, enabled=use_color))
    if verbose:
        print(terminal_style("showing all checks, including passing and informational checks", dim=True, enabled=use_color))
    else:
        print(terminal_style("use --verbose to show passing and informational checks", dim=True, enabled=use_color))
    for branch, check in details:
        url = result_url(check)
        glyph = status_symbol(check["status"], use_color)
        heading_text = f"{glyph} {branch['label']} / {check['check']}"
        heading = terminal_link(heading_text, url, use_color)
        print(heading)
        status_value = check.get("status_label") or check["status"]
        print(terminal_field("status", status_value, use_color))
        if url:
            print(terminal_field("url", url, use_color))
        if check["status"] == IN_PROGRESS and check.get("previous_completed_status"):
            print(terminal_field("previous", check["previous_completed_status"], use_color))
        if check.get("revision"):
            revision = check["revision"]
            if check.get("revision_distance"):
                revision = f"{revision} ({check['revision_distance']})"
            print(terminal_field("revision", revision, use_color))
        if check.get("completed_at"):
            age = age_text(check["completed_at"])
            if age:
                print(terminal_field("age", age, use_color))
        if check.get("message"):
            message = " ".join(str(check["message"]).split())
            print(terminal_field("message", message, use_color))
        print()


def exit_code_for_terminal(data):
    statuses = [branch["status"] for branch in data["branches"]]
    if any(status == FAILURE for status in statuses):
        return 1
    if any(status == IN_PROGRESS or status == UNKNOWN or is_stale_status(status) for status in statuses):
        return 2
    return 0


def write_atomic(path, content, mode="w"):
    path = pathlib.Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    target_mode = path.stat().st_mode & 0o777 if path.exists() else 0o644
    fd, tmpname = tempfile.mkstemp(prefix=f".{path.name}.", dir=str(path.parent), text="b" not in mode)
    try:
        with os.fdopen(fd, mode) as handle:
            handle.write(content)
        os.chmod(tmpname, target_mode)
        os.replace(tmpname, path)
    except Exception:
        try:
            os.unlink(tmpname)
        finally:
            raise


def write_json_output(data, output_file):
    write_atomic(output_file, json.dumps(data, indent=2, sort_keys=True) + "\n")


def load_config(path):
    try:
        with open(path, "r", encoding="utf-8") as handle:
            config = json.load(handle)
    except OSError as exc:
        raise ConfigError(f"cannot read configuration {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ConfigError(f"invalid JSON in configuration {path}: {exc}") from exc
    if "branches" not in config or "checks" not in config:
        raise ConfigError("configuration must contain branches and checks")
    return config
