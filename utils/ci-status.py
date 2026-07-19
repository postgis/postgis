#!/usr/bin/env python3
#
# Query the PostGIS CI providers and print a compact status report, or write a
# static status page suitable for publishing from cron.

import argparse
import asyncio
import datetime as dt
import html
import http.client
import json
import os
import pathlib
import subprocess
import sys
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
DISABLED = "disabled"
NOT_APPLICABLE = "not_applicable"

SYMBOLS = {
    SUCCESS: ("✅", "OK"),
    FAILURE: ("❌", "FAIL"),
    IN_PROGRESS: ("🔄", "RUN"),
    UNKNOWN: ("⚠️", "UNKNOWN"),
    STALE: ("⚠️", "STALE"),
    DISABLED: ("➖", "DISABLED"),
    NOT_APPLICABLE: ("➖", "N/A"),
}

COLORS = {
    SUCCESS: "\033[32m",
    FAILURE: "\033[31m",
    IN_PROGRESS: "\033[36m",
    UNKNOWN: "\033[33m",
    STALE: "\033[33m",
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

    current = builds[0]
    previous = next((build for build in builds[1:] if normalize_woodpecker_status(build.get("status")) not in (IN_PROGRESS, UNKNOWN)), None)
    web_url = check.get("web_url")
    run_url = current.get("link") or current.get("url")
    if not run_url and web_url and current.get("number"):
        run_url = f"{web_url}/pipeline/{current['number']}"
    result = make_result(
        check,
        branch,
        normalize_woodpecker_status(current.get("status")),
        url=run_url or web_url,
        debug_url=url,
        revision=current.get("commit"),
        completed_at=current.get("finished") or current.get("updated") or current.get("created"),
        message=current.get("message"),
    )
    if previous:
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
    try:
        builds = [
            build for build in jenkins_builds(job_url, check, timeout)
            if jenkins_matches_branch(build, check, branch)
        ]
    except RECOVERABLE_PROVIDER_ERRORS as exc:
        return jenkins_badge_check(check, branch, job_url, timeout, api_error=exc)
    if not builds:
        badge_result = jenkins_badge_check(check, branch, job_url, timeout)
        if badge_result["status"] != UNKNOWN or badge_result.get("status_label") == "Not run":
            return badge_result
        return make_result(check, branch, UNKNOWN, url=job_url, debug_url=job_url + "api/json", message="no matching Jenkins builds found")

    current = builds[0]
    previous = next((build for build in builds[1:] if not build.get("building")), None)
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
        stale["status"] = STALE
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
    stale["status"] = STALE
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


async def collect_status_async(config, selected_branch=None, include_eol=False, timeout=30):
    work = list(branch_checks(config, selected_branch, include_eol))
    semaphore = asyncio.Semaphore(min(default_concurrency(), max(1, len(work))))

    def collect_one(item):
        branch, check = item
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


def collect_status(config, selected_branch=None, include_eol=False, timeout=30):
    return asyncio.run(collect_status_async(config, selected_branch, include_eol, timeout))


def aggregate(config, results):
    branches = []
    by_branch = {}
    branch_order = {branch["name"]: branch for branch in config["branches"]}
    for result in results:
        by_branch.setdefault(result["branch"], []).append(result)

    for branch_name, checks in by_branch.items():
        required = [check for check in checks if check.get("required") and check["status"] not in (DISABLED, NOT_APPLICABLE)]
        if any(check["status"] == FAILURE for check in required):
            status = FAILURE
        elif any(check["status"] in (UNKNOWN, STALE) for check in required):
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
    }
    for check in required:
        counts[check["status"]] = counts.get(check["status"], 0) + 1
    counts["required"] = len(required)
    counts["informational"] = len(branch["checks"]) - len(required)
    return counts


def stale_status_label(base_status):
    return {
        SUCCESS: "Stale passed",
        FAILURE: "Stale failed",
        UNKNOWN: "Stale unknown",
    }.get(base_status, "Stale")


def stale_count_bucket(check):
    if check["status"] != STALE:
        return None
    base_status = check.get("stale_base_status")
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
        (counts[FAILURE], f"{counts[FAILURE]} stale-failed"),
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
        and check["status"] in (FAILURE, UNKNOWN, STALE, IN_PROGRESS, NOT_APPLICABLE)
        for check in branch["checks"]
    )
    status_order = {
        FAILURE: 0,
        UNKNOWN: 1,
        STALE: 1,
        IN_PROGRESS: 2,
        SUCCESS: 3,
        NOT_APPLICABLE: 4,
    }
    visible = []
    for index, check in enumerate(branch["checks"]):
        if (
            check["status"] in (FAILURE, UNKNOWN, STALE, IN_PROGRESS)
            or (check["status"] == NOT_APPLICABLE and check.get("provider") == "jenkins")
            or (
                has_visible_jenkins_problem
                and check.get("provider") == "jenkins"
                and check["status"] == SUCCESS
            )
        ) and check["status"] != DISABLED:
            visible.append((index, check))
    visible.sort(key=lambda item: (status_order.get(item[1]["status"], 99), item[0]))
    return [check for index, check in visible]


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
    if any(status in (IN_PROGRESS, UNKNOWN, STALE) for status in statuses):
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


def overall_status(branches):
    statuses = [branch["status"] for branch in branches]
    if any(status == FAILURE for status in statuses):
        return FAILURE
    if any(status in (UNKNOWN, STALE) for status in statuses):
        return UNKNOWN
    if any(status == IN_PROGRESS for status in statuses):
        return IN_PROGRESS
    if statuses:
        return SUCCESS
    return NOT_APPLICABLE


def required_failures(branches):
    failures = []
    for branch in branches:
        checks = [
            check for check in branch["checks"]
            if check.get("required") and check["status"] == FAILURE
        ]
        if checks:
            failures.append((branch["label"], checks))
    return failures


def html_status_label(status):
    return {
        SUCCESS: "Passing",
        FAILURE: "Failing",
        IN_PROGRESS: "Running",
        UNKNOWN: "Unknown",
        STALE: "Stale",
        DISABLED: "Disabled",
        NOT_APPLICABLE: "Not applicable",
    }.get(status, "Unknown")


def status_mark(status):
    return {
        SUCCESS: "✓",
        FAILURE: "✕",
        IN_PROGRESS: "↻",
        UNKNOWN: "?",
        STALE: "!",
        DISABLED: "–",
        NOT_APPLICABLE: "–",
    }.get(status, "?")


def html_status_pill(status, label=None):
    label = html.escape(label or html_status_label(status))
    mark = html.escape(status_mark(status))
    return f"<span class='status-pill'><span aria-hidden='true'>{mark} </span><span>{label}</span></span>"


def html_branch_progress(branch):
    counts = check_counts(branch)
    stale_counts = stale_check_counts(branch)
    total = counts["required"]
    if not total:
        return ""
    segments = [
        (SUCCESS, counts[SUCCESS], "OK"),
        (FAILURE, counts[FAILURE], "failing"),
        (IN_PROGRESS, counts[IN_PROGRESS], "running"),
        (UNKNOWN, counts[UNKNOWN], "unknown"),
        ("stale-passed", stale_counts[SUCCESS], "stale-passed"),
        ("stale-failed", stale_counts[FAILURE], "stale-failed"),
        ("stale-unknown", stale_counts[UNKNOWN], "stale-unknown"),
    ]
    labels = [f"{count} {label}" for status, count, label in segments if count]
    pieces = []
    for status, count, label in segments:
        if not count:
            continue
        width = count / total * 100
        pieces.append(
            f"<span class='progress-segment segment-{html.escape(status)}' "
            f"style='width:{width:.3f}%' title='{html.escape(f'{count} {label}')}'></span>"
        )
    aria = html.escape("; ".join(labels) or "no required checks")
    return f"<div class='branch-progress' role='img' aria-label='{aria}'>{''.join(pieces)}</div>"


def html_branch_summary(branch):
    counts = check_counts(branch)
    stale_counts = stale_check_counts(branch)
    parts = []
    if counts[SUCCESS]:
        parts.append((SUCCESS, f"{counts[SUCCESS]} OK"))
    if counts[FAILURE]:
        parts.append((FAILURE, plural(counts[FAILURE], "failure")))
    if counts[IN_PROGRESS]:
        parts.append((IN_PROGRESS, f"{counts[IN_PROGRESS]} running"))
    if counts[UNKNOWN]:
        parts.append((UNKNOWN, f"{counts[UNKNOWN]} unknown"))
    if stale_counts[SUCCESS]:
        parts.append(("stale-passed", f"{stale_counts[SUCCESS]} stale-passed"))
    if stale_counts[FAILURE]:
        parts.append(("stale-failed", f"{stale_counts[FAILURE]} stale-failed"))
    if stale_counts[UNKNOWN]:
        parts.append(("stale-unknown", f"{stale_counts[UNKNOWN]} stale-unknown"))
    if not parts:
        return html.escape(summary_text(branch))
    return " ".join(
        f"<span class='summary-chip chip-{html.escape(status)}'>{html.escape(text)}</span>"
        for status, text in parts
    )


def html_id(value):
    return "branch-" + "".join(char if char.isalnum() else "-" for char in value.lower()).strip("-")


def html_time(value):
    age = age_text(value)
    if not age:
        return ""
    timestamp = parse_time(value)
    if not timestamp:
        return html.escape(age)
    iso = timestamp.isoformat(timespec="seconds")
    return f"<time datetime='{html.escape(iso)}' title='{html.escape(iso)}'>{html.escape(age)}</time>"


def revision_url(revision):
    if not revision:
        return ""
    return f"https://gitea.osgeo.org/postgis/postgis/commit/{urllib.parse.quote(revision)}"


def html_revision(check):
    revision = check.get("revision") or ""
    if not revision:
        return ""
    text = revision[:12]
    if check.get("revision_distance"):
        text = f"{text} ({check['revision_distance']})"
    href = revision_url(revision)
    return f"<a class='revision-link' href='{html.escape(href)}'>{html.escape(text)}</a>"


def terminal_pad(text, width):
    pad = max(1, width - len(text))
    return "<span class='terminal-pad'>" + ("&nbsp;" * pad) + "</span>"


def html_check_rows(checks):
    detail_rows = []
    terminal_sep = "<span class='terminal-sep'> | </span>"
    for check in checks:
        link = result_url(check)
        check_name = html.escape(check["check"])
        check_html = f"<a href='{html.escape(link)}'>{check_name}</a>" if link else check_name
        status_label = check.get("status_label") or ("Passing" if check["status"] == SUCCESS else None)
        status_text = status_label or html_status_label(check["status"])
        status_text = f"{status_mark(check['status'])} {status_text}"
        status = html_status_pill(check["status"], status_label)
        previous = check.get("previous_completed_status")
        if previous and check["status"] == IN_PROGRESS:
            status_text = f"{status_text} previous: {previous}"
            status = f"{status} <span class='previous-note'>previous: {html.escape(previous)}</span>"
        message = " ".join(str(check.get("message") or "").split())
        check_classes = [f"status-{check['status']}"]
        if check["status"] == STALE:
            check_classes.append(f"stale-{stale_count_bucket(check)}")
        check_status = html.escape(" ".join(check_classes))
        revision = html_revision(check)
        revision_text = check.get("revision") or ""
        if revision_text:
            revision_text = revision_text[:12]
            if check.get("revision_distance"):
                revision_text = f"{revision_text} ({check['revision_distance']})"
        age = html_time(check.get("completed_at"))
        age_text_value = age_text(check.get("completed_at")) if check.get("completed_at") else ""
        detail_rows.append(
            f"<tr class='{check_status}'>"
            f"<td class='check-name'>{check_html}{terminal_pad(check['check'], 27)}</td>"
            f"<td>{terminal_sep}{status}{terminal_pad(status_text, 30)}</td>"
            f"<td class='revision-cell'>{terminal_sep}{revision}{terminal_pad(revision_text, 44)}</td>"
            f"<td class='age-cell'>{terminal_sep}{age}{terminal_pad(age_text_value, 8)}</td>"
            f"<td class='message-cell'>{terminal_sep}{html.escape(message)}</td>"
            "</tr>"
        )
    return "".join(detail_rows)


def html_check_table(problem_checks, passing_checks=None):
    rows = [html_check_rows(problem_checks)]
    if passing_checks:
        rows.append(html_check_rows(passing_checks))
    return (
        "<div class='table-wrap'><table class='check-table'>"
        "<thead><tr><th>Check</th><th><span class='terminal-sep'> | </span>Status</th>"
        "<th><span class='terminal-sep'> | </span>Revision</th><th><span class='terminal-sep'> | </span>Age</th>"
        "<th><span class='terminal-sep'> | </span>Message</th></tr></thead><tbody>"
        + "".join(rows)
        + "</tbody></table></div>"
    )


def html_required_failures(branches):
    failures = required_failures(branches)
    if not failures:
        return ""

    groups = []
    for branch_label, checks in failures:
        check_links = []
        for check in checks:
            check_name = html.escape(check["check"])
            link = result_url(check)
            check_links.append(
                f"<a href='{html.escape(link)}'>{check_name}</a>" if link else check_name
            )
        groups.append(
            f"<strong>{html.escape(branch_label)}</strong> — {', '.join(check_links)}"
        )

    return (
        " <span aria-label='Required CI failures'>· <strong>Required:</strong> "
        f"{' · '.join(groups)}</span>"
    )


def render_html(data):
    generated = html.escape(data["generated_at"])
    page_status = overall_status(data["branches"])
    page_summary_text = summary_text({"status": page_status, "checks": [
        check for branch in data["branches"] for check in branch["checks"]
    ]})
    page_summary = html.escape(page_summary_text)
    failure_attribution = html_required_failures(data["branches"])
    rows = []
    details = []
    for branch in data["branches"]:
        branch_status = html.escape(branch["status"])
        branch_label = html.escape(branch["label"])
        branch_id = html.escape(html_id(branch["name"]))
        branch_summary = html_branch_summary(branch)
        rows.append(
            f"<tr class='component-row status-{branch_status}' role='link' tabindex='0' "
            f"onclick=\"location.hash='{branch_id}'\" "
            f"onkeydown=\"if(event.key==='Enter'||event.key===' '){{event.preventDefault();location.hash='{branch_id}';}}\">"
            f"<td><span class='status-dot' aria-hidden='true'></span><strong>{branch_label}</strong></td>"
            f"<td><span class='terminal-sep'> | </span>{html_status_pill(branch['status'])}</td>"
            f"<td><span class='terminal-sep'> | </span><div class='summary-metrics'>"
            f"{html_branch_progress(branch)}<span class='branch-summary'>{branch_summary}</span></div></td>"
            "</tr>"
        )
        branch_details = interesting_checks(branch)
        passing_checks = [
            check for check in branch["checks"]
            if check["status"] == SUCCESS and check not in branch_details
        ]
        if branch_details or passing_checks:
            details.append(
                f"<details id='{branch_id}' class='detail-section status-{branch_status}' open><summary>"
                "<span class='status-dot' aria-hidden='true'></span>"
                f"<span>{branch_label} checks</span>"
                "<span class='disclosure-mark' aria-hidden='true'></span>"
                f"</summary>{html_check_table(branch_details, passing_checks)}</details>"
            )

    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>PostGIS CI status</title>
<link rel="icon" href="data:,">
<style>
:root {{
  color-scheme: light;
  --bg: #f6f8f7;
  --panel: #ffffff;
  --text: #24292e;
  --muted: #667085;
  --line: #dfe5e2;
  --header: #f3f6f4;
  --brand: #336791;
  --brand-strong: #2f5f47;
  --success: #1a7f37;
  --failure: #cf222e;
  --running: #0969da;
  --unknown: #9a6700;
  --disabled: #6e7781;
}}
* {{ box-sizing: border-box; }}
body {{
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font: 15px/1.5 system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}}
.page {{
  max-width: 1040px;
  margin: 0 auto;
  padding: 32px 20px 40px;
}}
.masthead {{
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 18px;
}}
.brand {{
  color: var(--brand);
  font-weight: 700;
  margin: 0 0 4px;
}}
h1 {{
  font-size: 28px;
  line-height: 1.2;
  margin: 0;
}}
.generated {{
  color: var(--muted);
  margin: 6px 0 0;
  font-size: 13px;
}}
.status-banner {{
  display: flex;
  align-items: center;
  gap: 12px;
  background: var(--panel);
  border: 1px solid var(--line);
  border-left: 5px solid var(--unknown);
  border-radius: 8px;
  padding: 14px 16px;
  margin-bottom: 16px;
}}
.status-banner.status-success {{ border-left-color: var(--success); }}
.status-banner.status-failure {{ border-left-color: var(--failure); }}
.status-banner.status-in_progress {{ border-left-color: var(--running); }}
.status-banner.status-unknown, .status-banner.status-stale {{ border-left-color: var(--unknown); }}
.status-banner.status-disabled, .status-banner.status-not_applicable {{ border-left-color: var(--disabled); }}
.banner-title {{
  font-size: 17px;
  font-weight: 650;
}}
.banner-summary {{
  color: var(--muted);
  font-size: 14px;
}}
.panel {{
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 8px;
  margin-top: 16px;
  overflow: hidden;
}}
.panel-heading {{
  background: var(--header);
  border-bottom: 1px solid var(--line);
  padding: 12px 16px;
  font-weight: 650;
}}
.component-list {{ width: 100%; }}
.component-row {{
  border-bottom: 1px solid var(--line);
  cursor: pointer;
}}
.component-row:hover {{
  background: #fbfcfb;
}}
.component-row:focus {{
  outline: 2px solid var(--brand);
  outline-offset: -2px;
}}
.component-row:last-child {{ border-bottom: 0; }}
.component-row td {{
  padding: 12px 16px;
  vertical-align: middle;
}}
.component-row td:first-child {{
  width: 24%;
  white-space: nowrap;
}}
.component-row td:nth-child(2) {{
  width: 120px;
  white-space: nowrap;
}}
.component-row strong {{
  margin-left: 10px;
}}
.summary-metrics {{
  display: inline-flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}}
.branch-summary {{
  white-space: nowrap;
}}
.summary-chip {{
  display: inline-block;
  border-radius: 4px;
  padding: 1px 5px;
  margin: 1px 2px 1px 0;
  font-size: 13px;
  line-height: 1.35;
}}
.chip-success {{
  color: var(--success);
  background: #f0fff4;
}}
.chip-failure {{
  color: var(--failure);
  background: #fff5f6;
}}
.chip-in_progress {{
  color: var(--running);
  background: #f0f7ff;
}}
.chip-unknown, .chip-stale {{
  color: var(--unknown);
  background: #fff8e5;
}}
.chip-stale-passed {{
  color: #5b6519;
  background: #f5f8e8;
}}
.chip-stale-failed {{
  color: #9a4f00;
  background: #fff3e0;
}}
.chip-stale-unknown {{
  color: var(--unknown);
  background: #fff8e5;
}}
.branch-progress {{
  display: flex;
  height: 5px;
  width: 170px;
  flex: 0 0 170px;
  overflow: hidden;
  border-radius: 999px;
  background: #eef2f1;
  opacity: 0.72;
}}
.progress-segment {{
  min-width: 2px;
}}
.segment-success {{
  background: #3f9d61;
}}
.segment-failure {{
  background: #d8757d;
}}
.segment-in_progress {{
  background: #70a7db;
}}
.segment-unknown, .segment-stale, .segment-stale-unknown {{
  background: #c8a24b;
}}
.segment-stale-passed {{
  background: #a4b85d;
}}
.segment-stale-failed {{
  background: #d48a3a;
}}
.status-dot {{
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background: var(--unknown);
  display: inline-block;
}}
.status-success .status-dot {{ background: var(--success); }}
.status-failure .status-dot {{ background: var(--failure); }}
.status-in_progress .status-dot {{ background: var(--running); }}
.status-unknown .status-dot, .status-stale .status-dot {{ background: var(--unknown); }}
.status-disabled .status-dot, .status-not_applicable .status-dot {{ background: var(--disabled); }}
.status-pill {{
  display: inline-flex;
  align-items: center;
  gap: 5px;
  width: max-content;
  border: 1px solid var(--line);
  border-radius: 999px;
  padding: 2px 9px;
  color: var(--muted);
  background: #ffffff;
  font-size: 12px;
  font-weight: 650;
  white-space: nowrap;
}}
.status-success .status-pill {{ color: var(--success); border-color: #b7dfc1; background: #f0fff4; }}
.status-failure .status-pill {{ color: var(--failure); border-color: #ffc9cf; background: #fff5f6; }}
.status-in_progress .status-pill {{ color: var(--running); border-color: #bfdbfe; background: #f0f7ff; }}
.status-unknown .status-pill, .status-stale .status-pill {{ color: var(--unknown); border-color: #f1d08a; background: #fff8e5; }}
.status-stale.stale-success .status-pill {{ color: #5b6519; border-color: #d8df9f; background: #f5f8e8; }}
.status-stale.stale-failure .status-pill {{ color: #9a4f00; border-color: #efc07a; background: #fff3e0; }}
tr.status-success .status-pill {{ color: var(--success); border-color: #b7dfc1; background: #f0fff4; }}
tr.status-failure .status-pill {{ color: var(--failure); border-color: #ffc9cf; background: #fff5f6; }}
tr.status-in_progress .status-pill {{ color: var(--running); border-color: #bfdbfe; background: #f0f7ff; }}
tr.status-unknown .status-pill, tr.status-stale .status-pill {{ color: var(--unknown); border-color: #f1d08a; background: #fff8e5; }}
.detail-section {{
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 8px;
  margin-top: 14px;
  scroll-margin-top: 18px;
}}
.detail-section summary {{
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 10px;
  list-style: none;
  padding: 12px 16px;
  font-weight: 650;
}}
.detail-section summary::-webkit-details-marker {{ display: none; }}
.disclosure-mark {{
  margin-left: auto;
  color: var(--muted);
  font-size: 18px;
  line-height: 1;
}}
.detail-section[open] .disclosure-mark::before {{ content: "⌃"; }}
.detail-section:not([open]) .disclosure-mark::before {{ content: "⌄"; }}
.table-wrap {{ overflow-x: auto; border-top: 1px solid var(--line); }}
table {{
  border-collapse: collapse;
  width: 100%;
  font-size: 13px;
}}
.check-table {{
  min-width: 940px;
  table-layout: fixed;
}}
.check-table th:nth-child(1), .check-table td:nth-child(1) {{
  width: 180px;
}}
.check-table th:nth-child(2), .check-table td:nth-child(2) {{
  width: 140px;
}}
.check-table th:nth-child(3), .check-table td:nth-child(3) {{
  width: 220px;
}}
.check-table th:nth-child(4), .check-table td:nth-child(4) {{
  width: 80px;
}}
th, td {{
  border-bottom: 1px solid var(--line);
  padding: 9px 12px;
  text-align: left;
  vertical-align: top;
}}
tr:last-child td {{ border-bottom: 0; }}
th {{
  background: var(--header);
  color: var(--muted);
  font-weight: 650;
}}
.check-name {{ font-weight: 600; }}
.previous-note {{
  display: block;
  margin-top: 4px;
  color: var(--muted);
  font-size: 12px;
  white-space: nowrap;
}}
.revision-link {{
  white-space: normal;
  overflow-wrap: anywhere;
}}
.revision-cell {{
  max-width: 210px;
}}
.age-cell {{
  white-space: nowrap;
}}
.message-cell {{
  min-width: 260px;
}}
.terminal-sep {{
  display: none;
}}
.terminal-pad {{
  display: none;
}}
time {{
  white-space: nowrap;
}}
a {{ color: var(--brand); text-decoration-thickness: 1px; text-underline-offset: 2px; }}
a:hover {{ color: var(--brand-strong); }}
.footer {{
  margin-top: 18px;
  color: var(--muted);
  font-size: 13px;
}}
@media (max-width: 700px) {{
  .page {{ padding: 24px 14px 32px; }}
  .masthead {{ display: block; }}
  h1 {{ font-size: 24px; }}
  .component-row td {{ padding: 10px 12px; }}
  .status-banner {{ align-items: flex-start; }}
}}
</style>
</head>
<body>
<main class="page">
<header class="masthead">
<div>
<p class="brand">PostGIS</p>
<h1>CI status</h1>
<p class="generated">Generated {generated} · <a class="raw-json-link" href="status.json">Raw JSON</a></p>
</div>
</header>
<section class="status-banner status-{html.escape(page_status)}" aria-label="Overall CI status">
<span class="status-dot" aria-hidden="true"></span>
<div>
<div class="banner-title"><span aria-hidden="true">{html.escape(status_mark(page_status))}</span> {html.escape(html_status_label(page_status))}</div>
<div class="banner-summary">{page_summary}{failure_attribution}</div>
</div>
</section>
<section class="panel" aria-label="Supported branch status">
<div class="panel-heading">Supported branches</div>
<table class="component-list"><thead><tr><th>Branch</th><th><span class="terminal-sep"> | </span>Status</th><th><span class="terminal-sep"> | </span>Summary</th></tr></thead><tbody>
{''.join(rows)}
</tbody></table>
</section>
{''.join(details)}
</main>
</body>
</html>
"""


def write_html_output(data, output_dir):
    output_dir = pathlib.Path(output_dir)
    json_text = json.dumps(data, indent=2, sort_keys=True) + "\n"
    write_atomic(output_dir / "status.json", json_text)
    write_atomic(output_dir / "index.html", render_html(data))


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


def positive_int(value):
    try:
        parsed = int(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("must be an integer") from exc
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be greater than 0")
    return parsed


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Report PostGIS CI status")
    parser.add_argument("legacy_mode", nargs="?", choices=("html",), help=argparse.SUPPRESS)
    parser.add_argument("--branch", help="check one branch name or label")
    parser.add_argument("--config", default=str(pathlib.Path(__file__).with_suffix(".json")))
    parser.add_argument("--format", choices=("terminal", "json", "html"), default="terminal", help="output format")
    parser.add_argument("--output-dir", default="ci-status")
    parser.add_argument("--json", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--no-color", action="store_true")
    parser.add_argument("--verbose", action="store_true", help="show all checks, including passing checks")
    parser.add_argument("--include-eol", action="store_true", help="include configured EOL branches")
    parser.add_argument("--timeout", type=positive_int, default=30, help="per-request timeout in seconds")
    args = parser.parse_args(argv)
    if args.legacy_mode and args.format != "terminal":
        parser.error("legacy html mode cannot be combined with --format")
    if args.json and args.format != "terminal":
        parser.error("--json cannot be combined with --format")
    if args.legacy_mode:
        args.format = "html"
    if args.json:
        args.format = "json"
    return args


def main(argv=None):
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        config = load_config(args.config)
        data = collect_status(config, args.branch, args.include_eol, args.timeout)
        if args.format == "html":
            write_html_output(data, args.output_dir)
            return 0
        if args.format == "json":
            print(json.dumps(data, indent=2, sort_keys=True))
        else:
            use_color = not args.no_color and sys.stdout.isatty()
            print_terminal(data, use_color=use_color, verbose=args.verbose)
        return exit_code_for_terminal(data)
    except ConfigError as exc:
        print(f"ci-status: {exc}", file=sys.stderr)
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
