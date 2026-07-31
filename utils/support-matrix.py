#!/usr/bin/env python3
"""Validate and export the PostGIS compatibility support matrix."""

from __future__ import annotations

import argparse
import datetime as dt
import importlib.util
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

try:
    from utils.support_matrix_payload import (
        build_payload_model,
        validate_payload_model,
    )
except ModuleNotFoundError:
    from support_matrix_payload import build_payload_model, validate_payload_model


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MATRIX = ROOT / "doc" / "development" / "compatibility" / "data" / "matrix.json"
DEFAULT_CACHE = ROOT / "doc" / "development" / "compatibility" / "data" / "cache.json"
DEFAULT_CI_CONFIG = ROOT / "utils" / "ci-status.json"
GENERATED_MATRIX_KEYS = {
    "dependency_versions",
    "packaged_versions",
    "vendored_inventory",
    "source_feature_gates",
    "update_warnings",
}
STATUS_VALUES = {
    "supported",
    "tested",
    "feature-limited",
    "known-compatible",
    "packaged",
    "historical",
    "unsupported",
    "not-applicable",
    "unknown",
}
CATALOG_REQUIREMENTS = {
    "postgresql-server-dev": "postgresql",
    "geos": "geos",
    "proj": "proj",
    "gdal": "gdal",
    "sfcgal": "sfcgal",
    "protobuf-c": "protobuf-c",
    "json-c": "json-c",
    "libxml2": "libxml2",
}
COMPATIBILITY_STATUS_PRECEDENCE = {
    "ci": 4,
    "tested": 4,
    "supported": 4,
    "feature-limited": 3,
    "known-compatible": 2,
    "historical": 2,
    "packaged": 2,
    "unsupported": 1,
    "unknown": 0,
    "not-applicable": 0,
}
LIBRARY_DIMENSIONS = {"geos", "proj", "gdal", "sfcgal"}


def load_json(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def version_config() -> dict[str, str]:
    values: dict[str, str] = {}
    for line in (ROOT / "Version.config").read_text(encoding="utf-8").splitlines():
        if "=" not in line or line.lstrip().startswith("#"):
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()
    return values


def current_dev_version() -> tuple[str, str]:
    values = version_config()
    major = values["POSTGIS_MAJOR_VERSION"]
    minor = values["POSTGIS_MINOR_VERSION"]
    micro = values["POSTGIS_MICRO_VERSION"]
    return f"{major}.{minor}.{micro}", f"{major}.{minor}"


def configure_minimums_from_text(text: str) -> dict[str, str]:
    checks = {
        "postgresql": (
            r"POSTGIS_PGSQL_VERSION -lt (\d+)",
            lambda value: f"{int(value) // 10}",
        ),
        "geos": (
            r"(?:GEOS_MIN_VERSION=|PostGIS requires GEOS >= )([0-9.]+)",
            lambda value: value,
        ),
        "proj": (r"PostGIS requires PROJ >= ([0-9.]+)", lambda value: value),
        "gdal": (r"GDAL_MIN_VERSION=([0-9.]+)", lambda value: value),
        "sfcgal": (r"PostGIS requires SFCGAL >= ([0-9.]+)", lambda value: value),
        "protobuf-c": (r"libprotobuf-c >= ([0-9.]+)", lambda value: value),
    }
    found: dict[str, str] = {}
    for key, (pattern, convert) in checks.items():
        match = re.search(pattern, text)
        if match:
            found[key] = convert(match.group(1))
    return found


def configure_minimums() -> dict[str, str]:
    return configure_minimums_from_text((ROOT / "configure.ac").read_text(encoding="utf-8"))


def upgradeable_versions() -> list[str]:
    text = (ROOT / "extensions" / "upgradeable_versions.mk").read_text(encoding="utf-8")
    versions = re.findall(r"\b\d+\.\d+\.\d+(?:[a-z]+\d*)?\b", text)
    seen = set()
    unique: list[str] = []
    for version in versions:
        if version in seen:
            continue
        seen.add(version)
        unique.append(version)
    return unique


def version_number_key(version: str) -> tuple[int, ...]:
    parts = [int(part) for part in re.findall(r"\d+", version)]
    while len(parts) > 2 and parts[-1] == 0:
        parts.pop()
    return tuple(parts)


def version_sort_key(version: str) -> tuple[tuple[int, ...], str]:
    return version_number_key(version), version


def version_at_least(version: str, minimum: str) -> bool:
    return version_number_key(version) >= version_number_key(minimum)


def release_sort_key(item: dict[str, Any]) -> tuple[str, tuple[tuple[int, ...], str]]:
    return item.get("release_date") or "", version_sort_key(item.get("version", ""))


def minor_for_version(version: str) -> str | None:
    match = re.match(r"^(\d+\.\d+)\.", version)
    return match.group(1) if match else None


def git_text(revision: str, path: str) -> str | None:
    result = subprocess.run(
        ["git", "show", f"{revision}:{path}"],
        cwd=ROOT,
        check=False,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    if result.returncode != 0:
        return None
    return result.stdout


def git_commit_date(revision: str) -> str | None:
    result = subprocess.run(
        ["git", "log", "-1", "--format=%cs", revision],
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    if result.returncode != 0:
        return None
    return result.stdout.strip() or None


def git_version_tags() -> list[str]:
    result = subprocess.run(
        ["git", "tag", "--list"],
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    if result.returncode != 0:
        return []
    return [tag for tag in result.stdout.splitlines() if re.match(r"^\d+\.\d+\.\d+(?:[a-z]+\d*)?$", tag)]


def news_sections(text: str) -> list[tuple[str, str]]:
    headers = list(re.finditer(r"^PostGIS\s+(\d+\.\d+(?:\.\S+)?)\s*$", text, flags=re.M))
    sections: list[tuple[str, str]] = []
    for index, header in enumerate(headers):
        end = headers[index + 1].start() if index + 1 < len(headers) else len(text)
        sections.append((header.group(1), text[header.start() : end]))
    return sections


def news_version_key(version: str) -> str:
    key = re.sub(r"dev$", "", version, flags=re.I)
    if re.match(r"^\d+\.\d+$", key):
        return f"{key}.0"
    return key


def news_release_date(text: str, version: str) -> str | None:
    for section_version, section_text in news_sections(text):
        if news_version_key(section_version).lower() != version.lower():
            continue
        match = re.search(r"^(\d{4})[/-](\d{2})[/-](\d{2})\s*$", section_text, flags=re.M)
        if match:
            return f"{match.group(1)}-{match.group(2)}-{match.group(3)}"
        return None
    return None


def readme_release_date(text: str) -> str | None:
    match = re.search(r"^VERSION:\s+\S+\s+\((\d{4})/(\d{2})/(\d{2})\)", text, flags=re.M)
    if not match:
        return None
    return f"{match.group(1)}-{match.group(2)}-{match.group(3)}"


def postgresql_range_from_text(text: str) -> str | None:
    patterns = (
        r"PostgreSQL\s+([0-9.]+(?:beta\d+)?\s*-\s*[0-9.]+(?:beta\d+)?)\s+required",
        r"requires PostgreSQL\s+([0-9.]+(?:beta\d+)?\s*-\s*[0-9.]+(?:beta\d+)?)",
        r"works for PostgreSQL\s+([0-9.]+)\s+thru PostgreSQL\s+([0-9.]+)",
    )
    for pattern in patterns:
        match = re.search(pattern, text, flags=re.I)
        if not match:
            continue
        if len(match.groups()) == 2:
            return f"{match.group(1)}-{match.group(2)}"
        return re.sub(r"\s+", "", match.group(1))
    return None


def postgresql_range_from_legacy_makefile(text: str) -> str | None:
    if not text:
        return None
    versions: list[str] = []
    if "USE_PG72" in text:
        versions.extend(["7.1", "7.2"])
    for value in re.findall(r"USE_VERSION=(\d+)", text):
        if value == "75":
            versions.append("7.5dev")
        elif value.startswith("7") and len(value) == 2:
            versions.append(f"7.{value[1]}")
        elif value.startswith("8") and len(value) == 2:
            versions.append(f"8.{value[1]}")
    ordered = ["7.1", "7.2", "7.3", "7.4", "7.5dev", "8.0", "8.1", "8.2"]
    present = [version for version in ordered if version in set(versions)]
    if not present:
        return None
    return f"{present[0]}-{present[-1]}" if len(present) > 1 else present[0]


def postgresql_range_from_news_detail(text: str, version: str) -> tuple[str, str] | None:
    sections = news_sections(text)
    if not sections:
        found = postgresql_range_from_text(text)
        return (found, "NEWS full file") if found else None
    minor = minor_for_version(version)
    for index, (section_version, section_text) in enumerate(sections):
        if news_version_key(section_version).lower() != version.lower():
            continue
        exact = postgresql_range_from_text(section_text)
        if exact:
            return exact, f"NEWS section {section_version}"
        for next_version, next_text in sections[index + 1 :]:
            if minor_for_version(news_version_key(next_version)) != minor:
                break
            inherited = postgresql_range_from_text(next_text)
            if inherited:
                return inherited, f"NEWS inherited from {next_version}"
        return None
    found = postgresql_range_from_text(text)
    return (found, "NEWS full file") if found else None


def postgis_patch_releases(series: list[dict[str, Any]]) -> list[dict[str, Any]]:
    by_minor = {item.get("minor"): item for item in series}
    versions = set(upgradeable_versions())
    versions.update(git_version_tags())
    for item in series:
        latest = item.get("latest")
        if latest and minor_for_version(latest):
            versions.add(latest)

    patches: list[dict[str, Any]] = []
    current_news = (ROOT / "NEWS").read_text(encoding="utf-8")
    for version in sorted(versions, key=version_sort_key, reverse=True):
        minor = minor_for_version(version)
        if not minor or minor not in by_minor:
            continue
        news = git_text(version, "NEWS") or ""
        configure = git_text(version, "configure.ac") or ""
        makefile = git_text(version, "Makefile") or ""
        readme = git_text(version, "README.postgis") or ""
        dependencies = {
            key: {"minimum": value, "source": f"tag:{version}:configure.ac"}
            for key, value in configure_minimums_from_text(configure).items()
            if value
        }
        legacy_makefile_range = postgresql_range_from_legacy_makefile(makefile)
        if legacy_makefile_range and version.startswith(("0.7.", "0.8.", "0.9.")):
            pg_range = legacy_makefile_range
            pg_source = f"tag:{version}:Makefile USE_VERSION"
        else:
            pg_detail = postgresql_range_from_news_detail(news, version)
            if pg_detail:
                pg_range, pg_source = pg_detail
                pg_source = f"tag:{version}:{pg_source}"
            else:
                pg_range = by_minor[minor].get("postgresql", {}).get("supported")
                pg_source = "postgis_series.supported fallback"
        patches.append(
            {
                "version": version,
                "minor": minor,
                "release_date": (
                    news_release_date(news, version)
                    or news_release_date(current_news, version)
                    or readme_release_date(readme)
                    or git_commit_date(version)
                ),
                "postgresql": {"supported": pg_range, "source": pg_source},
                "dependencies": dependencies,
                "source": f"tag:{version}",
            }
        )
    return sorted(patches, key=release_sort_key, reverse=True)


def ci_branches() -> dict[str, str]:
    config = load_json(DEFAULT_CI_CONFIG)
    return {branch["version"]: branch["name"] for branch in config.get("branches", [])}


def git_revision() -> str | None:
    result = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    if result.returncode != 0:
        return None
    return result.stdout.strip()


def compatibility_conflicts(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    series_by_minor = {item.get("minor"): item for item in data.get("postgis_series", [])}

    for dimension, edges in data.get("compatibility_edges", {}).items():
        claims: dict[tuple[str, str], dict[str, list[str]]] = {}
        for edge in edges if isinstance(edges, list) else []:
            postgis = edge.get("postgis")
            status = edge.get("status")
            source = edge.get("source", "unknown-source")
            if not postgis or status not in STATUS_VALUES:
                continue
            for version in edge.get("versions", []):
                claims.setdefault((postgis, version), {}).setdefault(status, []).append(source)

        for (postgis, version), statuses in sorted(claims.items()):
            if len(statuses) < 2:
                continue
            ranked = sorted(
                statuses,
                key=lambda status: (
                    COMPATIBILITY_STATUS_PRECEDENCE.get(status, -1),
                    status,
                ),
                reverse=True,
            )
            best = COMPATIBILITY_STATUS_PRECEDENCE.get(ranked[0], -1)
            lower = [status for status in ranked[1:] if COMPATIBILITY_STATUS_PRECEDENCE.get(status, -1) < best]
            same_rank = [
                status
                for status in ranked[1:]
                if COMPATIBILITY_STATUS_PRECEDENCE.get(status, -1) == best and status != ranked[0]
            ]
            if lower or same_rank:
                detail = ", ".join(
                    f"{status} from {'/'.join(sorted(set(sources)))}" for status, sources in sorted(statuses.items())
                )
                errors.append(
                    f"compatibility_edges.{dimension}: conflicting claims for PostGIS {postgis} "
                    f"and {dimension} {version}: {detail}"
                )

    for dimension in sorted(LIBRARY_DIMENSIONS):
        versions = data.get("dependency_versions", {}).get(dimension, [])
        if not versions:
            continue
        edge_status: dict[tuple[str, str], str] = {}
        for edge in data.get("compatibility_edges", {}).get(dimension, []):
            status = edge.get("status")
            rank = COMPATIBILITY_STATUS_PRECEDENCE.get(status, -1)
            for version in edge.get("versions", []):
                key = (edge.get("postgis"), version)
                if rank >= COMPATIBILITY_STATUS_PRECEDENCE.get(edge_status.get(key), -1):
                    edge_status[key] = status

        for minor, series in sorted(
            ((minor, series) for minor, series in series_by_minor.items() if minor),
            key=lambda item: version_sort_key(str(item[0])),
        ):
            if not minor or series.get("eol") or series.get("lifecycle") == "eol":
                continue
            dependency = (series.get("dependencies") or {}).get(dimension) or {}
            minimum = dependency.get("minimum")
            if not minimum:
                continue
            statuses: list[str] = []
            for item in versions:
                if item.get("lifecycle") == "development" and series.get("lifecycle") != "development":
                    continue
                version = item.get("version")
                if not version:
                    continue
                status = edge_status.get((minor, version))
                if not status:
                    if version_at_least(version, dependency.get("feature_complete") or minimum):
                        status = "supported"
                    elif version_at_least(version, minimum):
                        status = "feature-limited"
                    else:
                        status = "unsupported"
                statuses.append(status)
            if statuses and "supported" not in statuses:
                errors.append(
                    f"compatibility_edges.{dimension}: PostGIS {minor} has dependency minimum "
                    f"{minimum} but no resolved supported version; statuses={','.join(sorted(set(statuses)))}"
                )

    return errors


def validate_matrix(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    current_version, current_minor = current_dev_version()
    project = data.get("project", {})
    if project.get("current_development_version") != current_version:
        errors.append(
            "project.current_development_version does not match Version.config "
            f"({project.get('current_development_version')} != {current_version})"
        )
    if project.get("current_development_minor") != current_minor:
        errors.append(
            "project.current_development_minor does not match Version.config "
            f"({project.get('current_development_minor')} != {current_minor})"
        )

    branches = ci_branches()
    source_mins = configure_minimums()
    series = data.get("postgis_series", [])
    if not isinstance(series, list) or not series:
        errors.append("postgis_series must be a non-empty list")
        return errors

    for item in series:
        minor = item.get("minor")
        status = item.get("status")
        if status not in STATUS_VALUES:
            errors.append(f"{minor}: invalid status {status!r}")
        branch = item.get("branch")
        if minor in branches and branches[minor] != branch:
            errors.append(f"{minor}: branch {branch!r} does not match utils/ci-status.json {branches[minor]!r}")
        pg = item.get("postgresql", {})
        for override in pg.get("minor_overrides", []):
            if override.get("status") not in STATUS_VALUES:
                errors.append(f"{minor}: invalid PostgreSQL override status {override.get('status')!r}")

    known_series = {item.get("minor") for item in series}
    known_versions = {
        dimension: {item.get("version") for item in versions}
        for dimension, versions in data.get("dependency_versions", {}).items()
    }
    for item in series:
        minor = item.get("minor")
        for override in item.get("postgresql", {}).get("minor_overrides", []):
            for version in override.get("versions", []):
                if version not in known_versions.get("postgresql", set()):
                    errors.append(f"{minor}: unknown PostgreSQL override version {version!r}")
    for dimension, edges in data.get("compatibility_edges", {}).items():
        if not isinstance(edges, list):
            errors.append(f"compatibility_edges.{dimension} must be a list")
            continue
        for edge in edges:
            status = edge.get("status")
            if status not in STATUS_VALUES:
                errors.append(f"compatibility_edges.{dimension}: invalid status {status!r}")
            if not edge.get("postgis") or not edge.get("versions"):
                errors.append(f"compatibility_edges.{dimension}: edge must name postgis and versions")
            if edge.get("postgis") not in known_series:
                errors.append(f"compatibility_edges.{dimension}: unknown PostGIS series {edge.get('postgis')!r}")
            for version in edge.get("versions", []):
                if version not in known_versions.get(dimension, set()):
                    errors.append(f"compatibility_edges.{dimension}: unknown {dimension} version {version!r}")
    errors.extend(compatibility_conflicts(data))

    current = series[0]
    deps = current.get("dependencies", {})
    current_mins = {
        dependency: details.get("minimum") for dependency, details in deps.items() if details.get("minimum")
    }
    current_mins.update(source_mins)
    if current.get("minor") != current_minor:
        errors.append(f"first postgis_series row should describe current minor {current_minor}")
    if current.get("postgresql", {}).get("supported", "").split("-", 1)[0] != source_mins.get("postgresql"):
        errors.append("current PostgreSQL support range does not start at the configure.ac minimum")
    for dependency in ("geos", "proj", "gdal", "sfcgal", "protobuf-c"):
        minimum = deps.get(dependency, {}).get("minimum")
        expected = source_mins.get(dependency)
        if expected and minimum != expected:
            errors.append(f"current {dependency} minimum {minimum!r} does not match source {expected!r}")

    catalog = {item.get("id"): item for item in data.get("dependency_catalog", [])}
    for catalog_id, source_key in CATALOG_REQUIREMENTS.items():
        item = catalog.get(catalog_id)
        if not item:
            errors.append(f"dependency_catalog is missing {catalog_id}")
            continue
        expected_minimum = current_mins.get(source_key)
        if not expected_minimum:
            errors.append(f"dependency_catalog.{catalog_id} has no current-series minimum")
            continue
        if item.get("minimum") != expected_minimum:
            errors.append(
                f"dependency_catalog.{catalog_id} minimum {item.get('minimum')!r} "
                f"does not match current support data {expected_minimum!r}"
            )
    for item in data.get("dependency_catalog", []):
        if not item.get("label") or not item.get("requirement") or not item.get("source"):
            errors.append(f"dependency_catalog.{item.get('id')}: label, requirement, and source are required")
        if "in Winnie defaults" in str(item.get("minimum") or ""):
            errors.append(f"dependency_catalog.{item.get('id')}: Winnie selections must use a structured pin")
        pin = item.get("pin")
        if pin and not all(pin.get(key) for key in ("value", "context", "source")):
            errors.append(f"dependency_catalog.{item.get('id')}: pin requires value, context, and source")
        for override in (pin or {}).get("overrides", []):
            if not all(override.get(key) for key in ("value", "context", "source")):
                errors.append(f"dependency_catalog.{item.get('id')}: pin override requires value, context, and source")

    pseudo_version = re.compile(r"(?:\+|\.x$)")
    for dimension, versions in data.get("dependency_versions", {}).items():
        seen: set[str] = set()
        for item in versions:
            version = str(item.get("version") or "")
            if not version:
                errors.append(f"dependency_versions.{dimension}: version is required")
            elif version in seen:
                errors.append(f"dependency_versions.{dimension}: duplicate version {version}")
            elif pseudo_version.search(version):
                errors.append(
                    f"dependency_versions.{dimension}: expand pseudo-version {version!r} to real release lines"
                )
            seen.add(version)

    for collection in ("compatibility_sources", "sources"):
        for source in data.get(collection, []):
            source_id = str(source.get("id") or "")
            if source_id.startswith("trac-"):
                errors.append(f"{collection}.{source_id}: Trac tables are archaeology, not authority")
            url = str(source.get("url") or "")
            if not re.match(r"https?://", url) or re.search(r"https?://(?:api\.|[^/]+/api/)", url):
                errors.append(f"{collection}.{source_id}: link to a human-readable source page")
    for dimension, edges in data.get("compatibility_edges", {}).items():
        for edge in edges:
            if str(edge.get("source") or "").startswith("trac-"):
                errors.append(f"compatibility_edges.{dimension}.{edge.get('postgis')}: Trac source is not allowed")
    plus_shorthand = re.compile(r"\b(?:PostGIS|PostgreSQL|GEOS|PROJ|GDAL|SFCGAL)? ?[0-9]+(?:\.[0-9]+)?\+")

    def validate_text(value: Any, path: tuple[str, ...] = ()) -> None:
        if isinstance(value, dict):
            for key, child in value.items():
                validate_text(child, (*path, str(key)))
        elif isinstance(value, list):
            for index, child in enumerate(value):
                validate_text(child, (*path, str(index)))
        elif isinstance(value, str):
            if plus_shorthand.search(value):
                errors.append(f"{'.'.join(path)}: expand version shorthand in {value!r}")
            if path[-1:] in {("note",), ("default_summary",), ("feature_note",)} and "Trac" in value:
                errors.append(f"{'.'.join(path)}: user-facing compatibility text cites Trac")

    validate_text(data)

    known = data.get("upgrade_paths", {}).get("known_versions", [])
    actual = upgradeable_versions()
    if known and known != actual:
        errors.append("upgrade_paths.known_versions is stale; run support-matrix.py update")
    return errors


def validate_rendered(data: dict[str, Any]) -> list[str]:
    errors = validate_payload_model(data.get("browser", {}))
    source_dimensions = set(data.get("compatibility_edges", {}))
    browser_dimensions = [
        dimension.get("key") for dimension in data.get("browser", {}).get("matrix", {}).get("dimensions", [])
    ]
    if len(browser_dimensions) != len(set(browser_dimensions)):
        errors.append("browser.matrix.dimensions must be unique")
    if set(browser_dimensions) != source_dimensions:
        errors.append("browser.matrix.dimensions must cover every compatibility edge dimension")
    source_inventory_ids = {
        "postgis",
        *(item.get("id") for item in data.get("dependency_catalog", [])),
    }
    browser_inventory_ids = [
        item.get("id")
        for group in data.get("browser", {}).get("inventory", {}).get("groups", [])
        for item in group.get("items", [])
    ]
    if len(browser_inventory_ids) != len(set(browser_inventory_ids)):
        errors.append("browser.inventory item ids must be unique")
    if set(browser_inventory_ids) != source_inventory_ids:
        errors.append("browser.inventory must cover PostGIS and every dependency catalog item")
    for dimension, edges in data.get("compatibility_edges", {}).items():
        if any(edge.get("status") == "historical" for edge in edges):
            errors.append(f"compatibility_edges.{dimension}: rendered cells must not use lifecycle status historical")
    patch_versions = {patch.get("version") for patch in data.get("patch_releases", [])}
    for series in data.get("postgis_series", []):
        if series.get("latest") and series["latest"] not in patch_versions:
            errors.append(
                f"postgis_series.{series.get('minor')}: latest {series['latest']} "
                "is missing from generated patch releases"
            )
    return errors


def rendered(data: dict[str, Any], *, refresh_upgrades: bool) -> dict[str, Any]:
    result = json.loads(json.dumps(data, sort_keys=False))
    result["generated_at"] = dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")
    result["source_revision"] = git_revision()
    for edges in result.get("compatibility_edges", {}).values():
        for edge in edges:
            if edge.get("status") == "historical":
                edge["status"] = "known-compatible"
                note = edge.get("note")
                legacy_note = "Legacy/EOL context is carried by the row or column lifecycle, not by this compatibility intersection."
                edge["note"] = f"{legacy_note} {note}" if note else legacy_note
    if refresh_upgrades or not result.get("upgrade_paths", {}).get("known_versions"):
        result.setdefault("upgrade_paths", {})["known_versions"] = upgradeable_versions()
    result["patch_releases"] = postgis_patch_releases(result.get("postgis_series", []))
    result["browser"] = build_payload_model(result)
    errors = validate_rendered(result)
    if errors:
        raise ValueError("\n".join(errors))
    return result


def write_json(path: Path, data: dict[str, Any], *, compact: bool = False) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if compact:
        payload = json.dumps(data, separators=(",", ":"), sort_keys=False) + "\n"
    else:
        payload = json.dumps(data, indent=2, sort_keys=False) + "\n"
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=path.parent, delete=False) as handle:
        handle.write(payload)
        temp_name = handle.name
    Path(temp_name).replace(path)


def validated_output(data: dict[str, Any]) -> dict[str, Any]:
    errors = validate_matrix(data)
    if errors:
        raise ValueError("\n".join(errors))
    return rendered(data, refresh_upgrades=True)


def load_update_module() -> Any:
    path = ROOT / "utils" / "support_matrix_update.py"
    spec = importlib.util.spec_from_file_location("support_matrix_update", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_source_matrix(updater: Any | None = None) -> dict[str, Any]:
    updater = updater or load_update_module()
    matrix = load_json(DEFAULT_MATRIX)
    generated_keys = sorted(GENERATED_MATRIX_KEYS.intersection(matrix))
    if generated_keys:
        raise ValueError("matrix source contains generated sections: " + ", ".join(generated_keys))
    if matrix.get("upgrade_paths", {}).get("known_versions"):
        raise ValueError("matrix source contains generated upgrade versions")
    generated_sources = updater.generated_source_definitions(matrix.get("compatibility_sources", []))
    if generated_sources:
        raise ValueError("matrix source contains generated compatibility sources")
    return matrix


def load_cached_matrix(updater: Any | None = None) -> dict[str, Any]:
    updater = updater or load_update_module()
    matrix = load_source_matrix(updater)
    try:
        return updater.apply_cache(matrix, load_json(DEFAULT_CACHE))
    except updater.RefreshError as exc:
        raise ValueError(str(exc)) from exc


def print_errors(prefix: str, error: Exception) -> None:
    for line in str(error).splitlines():
        print(f"{prefix}: {line}", file=sys.stderr)


def update_matrix() -> int:
    updater = load_update_module()
    try:
        matrix = load_source_matrix(updater)
        cache = load_json(DEFAULT_CACHE)
        updated = updater.build_update(matrix, cache)
        updated.setdefault("upgrade_paths", {})["known_versions"] = upgradeable_versions()
        validated_output(updated)
        next_cache = updater.cache_from_matrix(updated)
    except (OSError, json.JSONDecodeError, updater.RefreshError, ValueError) as exc:
        print_errors("support-matrix update", exc)
        return 1

    write_json(DEFAULT_CACHE, next_cache, compact=True)
    for warning in updated.get("update_warnings", []):
        print(
            f"support-matrix update: warning: {warning['source']}: {warning['message']} (using {warning['fallback']})",
            file=sys.stderr,
        )
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("update", help="refresh source data with cached fallback")
    build = commands.add_parser("build", help="validate and build browser JSON")
    build.add_argument("output", type=Path)
    commands.add_parser("check", help="validate source data and generated compatibility payload")
    args = parser.parse_args(argv)

    if args.command == "update":
        return update_matrix()
    try:
        output = validated_output(load_cached_matrix())
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print_errors("support-matrix", exc)
        return 1
    if args.command == "build":
        write_json(args.output, output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
