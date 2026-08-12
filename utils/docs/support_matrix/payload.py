"""Build the presentation-ready compatibility payload from validated source data."""

from __future__ import annotations

import copy
import datetime as dt
import re
from typing import Any
from urllib.parse import quote


PAYLOAD_SCHEMA_VERSION = 1


def version_number_key(version: str) -> tuple[int, ...]:
    parts = [int(part) for part in re.findall(r"\d+", version)]
    while len(parts) > 2 and parts[-1] == 0:
        parts.pop()
    return tuple(parts)


def version_sort_key(version: str) -> tuple[tuple[int, ...], str]:
    return version_number_key(version), version


SOURCE_ID_DATE = re.compile(r"^(?P<stem>.+)-(?P<date>\d{4}-\d{2}-\d{2})$")
TAG_SOURCE_REFERENCE = re.compile(r"(?:^|;\s*)tag:(?P<ref>[^:;]+)(?::(?P<path>[^:;]+))?(?::(?P<context>[^;]+))?")
TREE_SOURCE_REFERENCE = re.compile(
    r"^(?P<path>(?:\.github|ci|doc|extensions|postgis|raster|sfcgal|topology)/[^:;]+|"
    r"configure\.ac|configure\.in|NEWS|Makefile)(?::(?P<context>.+))?$"
)
INTERNAL_SOURCE_REFERENCE = re.compile(
    r"(?<![\w/])(?:news-(?:sfcgal|geos-315|postgresql-[a-z-]+)|"
    r"postgis-(?:ci-dashboard|master-configure-ac|release-range-inference)|"
    r"(?:postgis|postgresql|geos|proj|gdal|sfcgal|repology|github-tags)"
    r"(?:-[a-z0-9]+)+-\d{4}(?:-\d{2}-\d{2})?)(?![\w/])"
)


def browser_source_detail(data: dict[str, Any], source: str | dict[str, Any]) -> dict[str, str]:
    if isinstance(source, dict):
        definition = source
        source_id = str(source.get("id") or "")
        referenced_date = str(source.get("as_of") or "")
    else:
        source_id = str(source or "")
        referenced_date = ""
        definitions = [
            *data.get("compatibility_sources", []),
            *data.get("sources", []),
        ]
        definition = next((item for item in definitions if item.get("id") == source_id), None)
        match = SOURCE_ID_DATE.fullmatch(source_id)
        if not definition and match:
            referenced_date = match.group("date")
            definition = next(
                (
                    item
                    for item in reversed(definitions)
                    if SOURCE_ID_DATE.fullmatch(str(item.get("id") or ""))
                    and SOURCE_ID_DATE.fullmatch(str(item["id"])).group("stem") == match.group("stem")
                ),
                None,
            )

    if not definition:
        tag_match = TAG_SOURCE_REFERENCE.search(source_id)
        if tag_match:
            revision = tag_match.group("ref")
            raw_path = str(tag_match.group("path") or "")
            path = next(
                (
                    candidate
                    for candidate in (
                        "configure.ac",
                        "configure.in",
                        "NEWS",
                        "Makefile",
                    )
                    if raw_path.startswith(candidate)
                ),
                raw_path,
            )
            revision_label = "development branch" if revision == "HEAD" else revision
            if path in {"configure.ac", "configure.in"}:
                title = f"PostGIS {revision_label} dependency checks"
            elif path == "NEWS":
                title = f"PostGIS {revision_label} release notes"
            elif path == "Makefile":
                title = f"PostGIS {revision_label} build configuration"
            elif path == "postgis/postgis.sql.in":
                title = f"PostGIS {revision_label} PostgreSQL feature declarations"
            elif path:
                title = f"PostGIS {revision_label} source: {path}"
            else:
                title = f"PostGIS {revision_label} source tree"
            base = (
                "https://gitea.osgeo.org/postgis/postgis/src/branch/master"
                if revision == "HEAD"
                else f"https://gitea.osgeo.org/postgis/postgis/src/tag/{quote(revision, safe='')}"
            )
            return {
                "value": title,
                "url": f"{base}/{quote(path, safe='/')}" if path else base,
            }

        tree_match = TREE_SOURCE_REFERENCE.fullmatch(source_id)
        if tree_match:
            path = tree_match.group("path")
            if path == "postgis/postgis.sql.in":
                title = "PostGIS development PostgreSQL feature declarations"
            else:
                title = f"PostGIS development source: {path}"
            return {
                "value": title,
                "url": (f"https://gitea.osgeo.org/postgis/postgis/src/branch/master/{quote(path, safe='/')}"),
            }

        if source_id.startswith("postgis_series.supported"):
            return {
                "value": "PostGIS release-series support metadata",
                "url": (
                    "https://gitea.osgeo.org/postgis/postgis/src/branch/master/"
                    "doc/development/compatibility/data/matrix.json"
                ),
            }
        return {"value": source_id}

    detail = {
        "value": str(definition.get("title") or source_id),
        "url": str(definition.get("url") or ""),
    }
    as_of = referenced_date or str(definition.get("as_of") or "")
    try:
        date = dt.date.fromisoformat(as_of)
    except ValueError:
        pass
    else:
        detail["meta"] = f"as of {date.day} {date:%b %Y}"
    return detail


def browser_source_display(data: dict[str, Any], source_id: str) -> str:
    detail = browser_source_detail(data, source_id)
    if detail.get("value") == source_id:
        return source_id
    parts = [str(detail.get("value") or "")]
    if detail.get("meta"):
        parts.append(f"({detail['meta']})")
    return " ".join(part for part in parts if part)


def browser_warning_source(data: dict[str, Any], source_id: str) -> dict[str, str]:
    if source_id.startswith("repology:"):
        project = source_id.split(":", 1)[1]
        return {
            "value": f"Repology package metadata for {project}",
            "url": f"https://repology.org/project/{quote(project, safe='')}/versions",
        }
    if source_id.startswith("github-tags:"):
        repository = source_id.split(":", 1)[1]
        return {
            "value": f"GitHub tags for {repository}",
            "url": f"https://github.com/{repository}/tags",
        }
    stems = {
        "postgresql": "postgresql-versioning",
        "geos": "geos-download",
        "proj": "proj-download",
        "gdal": "gdal-download",
        "gdal-past": "gdal-download",
        "sfcgal": "sfcgal-gitlab-releases",
        "winnie-dependencies": "postgis-ci-winnie-deps",
        "release-docs-ci": "postgis-release-docs-ci",
        "source-gates": "postgis-source-gates",
    }
    stem = stems.get(source_id)
    if stem:
        definitions = [
            *data.get("compatibility_sources", []),
            *data.get("sources", []),
        ]
        definition = next(
            (
                item
                for item in reversed(definitions)
                if str(item.get("id") or "") == stem or str(item.get("id") or "").startswith(f"{stem}-")
            ),
            None,
        )
        if definition:
            return browser_source_detail(data, definition)
    return browser_source_detail(data, source_id)


BROWSER_DIMENSIONS = (
    ("postgresql", "PostgreSQL"),
    ("geos", "GEOS"),
    ("proj", "PROJ"),
    ("gdal", "GDAL"),
    ("sfcgal", "SFCGAL"),
)
BROWSER_DIMENSION_LABELS = dict(BROWSER_DIMENSIONS)
BROWSER_STATUSES = {
    "supported": {"symbol": "W", "label": "works, supported"},
    "feature-limited": {"symbol": "L", "label": "works with limited features"},
    "known-compatible": {
        "symbol": "C",
        "label": "compatible, not a recommended target",
    },
    "packaged": {"symbol": "P", "label": "packaged downstream"},
    "unsupported": {"symbol": "X", "label": "known unsupported"},
    "not-applicable": {"symbol": "-", "label": "not applicable"},
    "unknown": {"symbol": "?", "label": "not recorded"},
}
PROJECT_URLS = {
    "postgis": "/development/source_code/",
    "postgresql": "https://www.postgresql.org/",
    "geos": "https://libgeos.org/",
    "proj": "https://proj.org/",
    "gdal": "https://gdal.org/",
    "sfcgal": "https://sfcgal.gitlab.io/SFCGAL/",
    "libxml2": "https://gitlab.gnome.org/GNOME/libxml2",
    "libiconv": "https://www.gnu.org/software/libiconv/",
    "gmp": "https://gmplib.org/",
    "protobuf-c": "https://github.com/protobuf-c/protobuf-c",
    "json-c": "https://github.com/json-c/json-c",
    "zlib": "https://zlib.net/",
    "lz4": "https://lz4.org/",
    "sqlite": "https://sqlite.org/",
    "boost": "https://www.boost.org/",
    "cgal": "https://www.cgal.org/",
    "cunit": "https://cunit.sourceforge.net/",
    "gettext": "https://www.gnu.org/software/gettext/",
    "gtk2": "https://www.gtk.org/",
    "wagyu": "https://github.com/mapbox/wagyu",
    "ryu": "https://github.com/ulfjack/ryu",
    "flatgeobuf": "https://flatgeobuf.org/",
    "uthash": "https://troydhanson.github.io/uthash/",
    "docbook-xsl": "https://docbook.sourceforge.net/",
    "docbook-xml": "https://docbook.org/",
    "libxslt": "https://gitlab.gnome.org/GNOME/libxslt",
    "xmlto": "https://pagure.io/xmlto/",
    "dblatex": "https://dblatex.sourceforge.net/",
    "imagemagick": "https://imagemagick.org/",
}
RELEASE_TAGS = {
    "postgresql": ("https://www.postgresql.org/docs/release/{version}/", ""),
    "geos": ("https://github.com/libgeos/geos/releases/tag/{version}", ""),
    "proj": ("https://github.com/OSGeo/PROJ/releases/tag/{version}", ""),
    "gdal": ("https://github.com/OSGeo/gdal/releases/tag/v{version}", ""),
    "sfcgal": ("https://gitlab.com/sfcgal/SFCGAL/-/releases/v{version}", ""),
}
DEPENDENCY_GROUPS = (
    (
        "build-runtime",
        "Build and runtime",
        "Projects that define the supported build and feature set.",
    ),
    (
        "vendored",
        "Vendored sources",
        "In-tree source versions compared with their upstream releases.",
    ),
    (
        "tooling",
        "Tooling and packaging",
        "CI, tests, documentation, translations, and Windows packaging.",
    ),
)


def compare_versions(left: str, right: str) -> int:
    left_key = version_number_key(str(left))
    right_key = version_number_key(str(right))
    return (left_key > right_key) - (left_key < right_key)


def browser_version_bounds(value: str) -> tuple[str, str]:
    parts = str(value or "").replace("+", "").split("-", 1)
    minimum = parts[0]
    maximum = parts[1] if len(parts) > 1 else minimum
    if len(parts) == 1 and re.fullmatch(r"\d+\.\d+", maximum):
        maximum += ".999"
    return minimum, maximum


def browser_postgresql_range(value: str) -> tuple[str, str]:
    cleaned = str(value or "").replace("PostgreSQL", "").replace("historical", "").strip()
    parts = cleaned.split("-", 1)
    return parts[0], parts[1] if len(parts) > 1 else parts[0]


def browser_in_postgresql_range(version: str, value: str) -> bool:
    normalized = re.sub(r"beta\d+", "", str(version), flags=re.IGNORECASE)
    minimum, maximum = browser_postgresql_range(value)
    if not minimum or not maximum:
        return False
    if "beta" in maximum.lower() and "beta" not in str(version).lower():
        if normalized == re.sub(r"beta\d+", "", maximum, flags=re.IGNORECASE):
            return False
    maximum = re.sub(r"beta\d+", "", maximum, flags=re.IGNORECASE)
    return compare_versions(normalized, minimum) >= 0 and compare_versions(normalized, maximum) <= 0


def browser_column_covers(column: str, version: str) -> bool:
    minimum, maximum = browser_version_bounds(column)
    return compare_versions(minimum, version) <= 0 and compare_versions(maximum, version) >= 0


def browser_is_eol(item: dict[str, Any] | None) -> bool:
    return bool(item) and bool(item.get("eol") or item.get("lifecycle") in {"eol", "historical"})


def browser_is_development(item: dict[str, Any] | None) -> bool:
    return bool(item) and bool(item.get("release_stage") == "development" or item.get("lifecycle") == "development")


def browser_status_key(status: str | None) -> str:
    if status == "historical":
        return "known-compatible"
    if status == "tested":
        return "supported"
    return status if status in BROWSER_STATUSES else "unknown"


def browser_release_age(value: str | None, current_year: int, *, month: bool = False) -> str:
    match = re.match(r"^(\d{4})(?:-(\d{2}))?", str(value or ""))
    if not match:
        return ""
    year = int(match.group(1))
    prefix = f"{year}-{match.group(2)}" if month and match.group(2) else str(year)
    age = current_year - year
    if age <= 0:
        return prefix
    unit = "year" if age == 1 else "years"
    return f"{prefix} / {age} {unit}"


def browser_project_key(key: str) -> str:
    return {"postgresql-server-dev": "postgresql"}.get(key, key)


def browser_gate_key(key: str) -> str:
    return {
        "postgresql-server-dev": "postgresql",
        "libiconv": "iconv",
    }.get(key, browser_project_key(key))


def browser_link(value: str, url: str, title: str = "") -> dict[str, str]:
    return {"value": value, "url": url, "title": title or value}


def browser_postgis_news_link(version: str) -> dict[str, str]:
    return browser_link(
        "NEWS",
        f"https://gitea.osgeo.org/postgis/postgis/src/tag/{quote(version, safe='')}/NEWS",
        f"PostGIS {version} release notes",
    )


def browser_postgis_branch_news_link(series: dict[str, Any]) -> dict[str, str]:
    minor = str(series.get("minor") or "")
    if series.get("lifecycle") == "development":
        return browser_link(
            "Branch NEWS",
            "https://gitea.osgeo.org/postgis/postgis/src/branch/master/NEWS",
            "PostGIS development branch NEWS",
        )
    if browser_is_eol(series) and series.get("latest"):
        return browser_link(
            "Latest NEWS",
            f"https://gitea.osgeo.org/postgis/postgis/src/tag/{quote(str(series['latest']), safe='')}/NEWS",
            f"PostGIS {series['latest']} release notes",
        )
    branch = f"stable-{minor}"
    return browser_link(
        "Branch NEWS",
        f"https://gitea.osgeo.org/postgis/postgis/src/branch/{quote(branch, safe='')}/NEWS",
        f"PostGIS {branch} NEWS",
    )


def browser_dependency_release_link(key: str, item: dict[str, Any]) -> dict[str, str] | None:
    template = RELEASE_TAGS.get(key)
    version = str(item.get("current_minor") or item.get("version") or "")
    if not template or not version:
        return None
    url_template, prefix = template
    if key == "postgresql" and not re.fullmatch(r"\d+(?:\.\d+)+", version):
        return None
    linked_version = f"{prefix}{version}"
    return browser_link(
        "Release notes",
        url_template.format(version=quote(linked_version, safe="")),
        f"{BROWSER_DIMENSION_LABELS.get(key, key)} {version} release notes",
    )


def browser_dependency_group(item: dict[str, Any]) -> str:
    if item.get("id") == "postgis" or item.get("kind") in {
        "core",
        "extension",
        "transitive",
        "feature",
        "project",
    }:
        return "build-runtime"
    if item.get("kind") == "vendored":
        return "vendored"
    return "tooling"


def browser_source_gate_summary(data: dict[str, Any], dependency: str) -> dict[str, str] | None:
    gates = [gate for gate in data.get("source_feature_gates", []) if gate.get("dependency") == dependency]
    if not gates:
        return None
    thresholds = sorted(
        {str(gate["threshold"]) for gate in gates if gate.get("threshold") and gate.get("threshold") != "optional"},
        key=version_sort_key,
    )
    counts: dict[str, int] = {}
    first_note = ""
    for gate in gates:
        if gate.get("class"):
            counts[gate["class"]] = counts.get(gate["class"], 0) + 1
        if not first_note and gate.get("note"):
            first_note = gate["note"]
    pieces = [f"thresholds {', '.join(thresholds)}"] if thresholds else []
    pieces.extend(f"{name} {count}" for name, count in sorted(counts.items()))
    return {
        "source_gate_summary": "; ".join(pieces),
        "feature_note": first_note,
    }


def browser_edge_indexes(
    data: dict[str, Any],
) -> dict[str, dict[tuple[str, str], dict[str, Any]]]:
    indexes: dict[str, dict[tuple[str, str], dict[str, Any]]] = {}
    for dimension, edges in data.get("compatibility_edges", {}).items():
        index: dict[tuple[str, str], dict[str, Any]] = {}
        for edge in edges:
            for version in edge.get("versions", []):
                index[(edge.get("postgis"), version)] = edge
        indexes[dimension] = index
    return indexes


def browser_patch_series(series: dict[str, Any], patch: dict[str, Any] | None) -> dict[str, Any]:
    if not patch:
        return series
    result = copy.deepcopy(series)
    result["patch_version"] = patch.get("version")
    result["patch_release_date"] = patch.get("release_date")
    result["patch_source"] = patch.get("source")
    result["postgresql"] = {
        **series.get("postgresql", {}),
        **patch.get("postgresql", {}),
    }
    result["dependencies"] = copy.deepcopy(series.get("dependencies", {}))
    for key, value in patch.get("dependencies", {}).items():
        result["dependencies"][key] = {
            **series.get("dependencies", {}).get(key, {}),
            **value,
        }
    return result


def browser_minor_override(series: dict[str, Any], version: str) -> dict[str, Any] | None:
    for override in series.get("postgresql", {}).get("minor_overrides", []):
        if version in override.get("versions", []):
            return override
    return None


def browser_cell_status(
    series: dict[str, Any],
    dimension: str,
    column: str,
    version_meta: dict[str, Any] | None,
    edge_indexes: dict[str, dict[tuple[str, str], dict[str, Any]]],
) -> tuple[str, dict[str, Any] | None]:
    edge = None
    if not (series.get("patch_version") and dimension == "postgresql"):
        edge = edge_indexes.get(dimension, {}).get((series.get("minor"), column))
    if edge:
        return browser_status_key(edge.get("status")), edge

    if dimension == "postgresql":
        override = None if series.get("patch_version") else browser_minor_override(series, column)
        if override:
            return browser_status_key(override.get("status")), override
        if browser_is_development(version_meta) and series.get("lifecycle") != "development":
            return "not-applicable", None
        postgresql = series.get("postgresql", {})
        if browser_in_postgresql_range(column, postgresql.get("supported", "")):
            feature_complete = postgresql.get("feature_complete")
            _, maximum = browser_version_bounds(column)
            if feature_complete and compare_versions(maximum, feature_complete) < 0:
                return "feature-limited", None
            return "supported", None
        return "not-applicable", None

    if browser_is_development(version_meta) and series.get("lifecycle") != "development":
        return "not-applicable", None
    dependency = series.get("dependencies", {}).get(dimension, {})
    minimum = dependency.get("minimum")
    if not minimum:
        return "not-applicable", None
    column_minimum, column_maximum = browser_version_bounds(column)
    if compare_versions(column_maximum, minimum) < 0:
        return "unsupported", None
    if compare_versions(column_minimum, minimum) < 0:
        return "feature-limited", None
    feature_complete = dependency.get("feature_complete")
    if feature_complete and compare_versions(column_minimum, feature_complete) < 0:
        return "feature-limited", None
    return "supported", None


def browser_cell_note(
    data: dict[str, Any],
    series: dict[str, Any],
    dimension: str,
    column: str,
    status: str,
    evidence: dict[str, Any] | None,
    version_meta: dict[str, Any] | None,
) -> str:
    if evidence:
        return str(evidence.get("note") or "")
    if browser_is_development(version_meta) and series.get("lifecycle") != "development":
        return "Development dependency column is shown only for the current development PostGIS line."
    if dimension == "postgresql":
        postgresql = series.get("postgresql", {})
        if status == "not-applicable":
            return "Outside the documented PostGIS/PostgreSQL support range."
        if status == "feature-limited":
            return str(postgresql.get("feature_note") or "")
        return ""
    dependency = series.get("dependencies", {}).get(dimension, {})
    return "; ".join(
        part
        for part in (
            f"minimum: {dependency['minimum']}" if dependency.get("minimum") else "",
            (f"feature-complete: {dependency['feature_complete']}" if dependency.get("feature_complete") else ""),
            dependency.get("feature_note"),
            (f"source gates: {dependency['source_gate_summary']}" if dependency.get("source_gate_summary") else ""),
        )
        if part
    )


def browser_cell_sources(
    data: dict[str, Any],
    series: dict[str, Any],
    dimension: str,
    status: str,
    evidence: dict[str, Any] | None,
) -> list[dict[str, str]]:
    source_ids = []
    if evidence and evidence.get("source"):
        source_ids.append(evidence["source"])
    elif dimension == "postgresql":
        postgresql = series.get("postgresql", {})
        if status == "feature-limited" and postgresql.get("feature_source"):
            source_ids.append(postgresql["feature_source"])
        if postgresql.get("source"):
            source_ids.append(postgresql["source"])
    else:
        dependency = series.get("dependencies", {}).get(dimension, {})
        if status == "feature-limited" and dependency.get("feature_source"):
            source_ids.append(dependency["feature_source"])
        if dependency.get("source"):
            source_ids.append(dependency["source"])

    details = []
    for source_id in source_ids:
        detail = browser_source_detail(data, source_id)
        if not detail.get("value"):
            continue
        identity = detail.get("url") or detail.get("value")
        if not any((item.get("url") or item.get("value")) == identity for item in details):
            details.append(detail)
    return details


def browser_cell(
    data: dict[str, Any],
    series: dict[str, Any],
    dimension: dict[str, Any],
    column: dict[str, Any],
    edge_indexes: dict[str, dict[tuple[str, str], dict[str, Any]]],
) -> dict[str, Any]:
    status, evidence = browser_cell_status(
        series,
        dimension["key"],
        column["key"],
        column.get("meta"),
        edge_indexes,
    )
    row_eol = browser_is_eol(series)
    display_status = "known-compatible" if status == "supported" and (row_eol or column["eol"]) else status
    note = browser_cell_note(
        data,
        series,
        dimension["key"],
        column["key"],
        status,
        evidence,
        column.get("meta"),
    )
    if display_status != status:
        note = "; ".join(
            part
            for part in (
                note,
                "Displayed as compatible because the row or column is End of Life (EOL).",
            )
            if part
        )
    return {
        "status": display_status,
        "note": note,
        "sources": browser_cell_sources(
            data,
            series,
            dimension["key"],
            status,
            evidence,
        ),
    }


def browser_column(
    data: dict[str, Any],
    key: str,
    label: str,
    item: dict[str, Any],
    current_year: int,
) -> dict[str, Any]:
    lifecycle = "End of Life (EOL)" if browser_is_eol(item) else str(item.get("lifecycle") or "n/a")
    final_label = ""
    if item.get("final_release_date"):
        prefix = "ended" if browser_is_eol(item) else "until"
        final_label = f"{prefix} {str(item['final_release_date'])[:4]}"
    detail = "; ".join(
        part
        for part in (
            f"{label} {item.get('version')}",
            f"current/latest minor: {item['current_minor']}" if item.get("current_minor") else "",
            f"first release: {item['first_release_date']}" if item.get("first_release_date") else "",
            f"final release: {item['final_release_date']}" if item.get("final_release_date") else "",
            item.get("note"),
        )
        if part
    )
    source = browser_source_detail(data, item.get("source") or "")
    release = browser_dependency_release_link(key, item)
    return {
        "key": item["version"],
        "label": item["version"],
        "eol": browser_is_eol(item),
        "development": browser_is_development(item),
        "lifecycle": str(item.get("lifecycle") or ""),
        "life_label": lifecycle,
        "date_label": browser_release_age(item.get("first_release_date"), current_year)
        or str(item.get("current_minor") or "n/a"),
        "final_label": final_label,
        "detail": detail,
        "release": release,
        "source": source if item.get("source") else None,
        "meta": item,
    }


def browser_supported_column_indexes(
    dimension: dict[str, Any],
    supported_series: list[dict[str, Any]],
) -> list[int]:
    visible: list[int] = []
    for index, column in enumerate(dimension["columns"]):
        meta = column.get("meta")
        if not meta:
            visible.append(index)
            continue
        if column["eol"]:
            continue
        if dimension["key"] == "postgresql" and column["development"]:
            if any(
                series.get("lifecycle") == "development"
                and browser_in_postgresql_range(column["key"], series.get("postgresql", {}).get("supported", ""))
                for series in supported_series
            ):
                visible.append(index)
            continue
        if not column["development"] and column["lifecycle"] in {
            "supported",
            "current",
            "recent",
        }:
            visible.append(index)
            continue
        if any(
            (series.get("dependencies", {}).get(dimension["key"], {}) or {}).get("feature_complete")
            and browser_column_covers(
                column["key"],
                series["dependencies"][dimension["key"]]["feature_complete"],
            )
            for series in supported_series
        ):
            visible.append(index)
    if visible:
        return visible
    return [
        index for index, column in enumerate(dimension["columns"]) if not column["development"] and not column["eol"]
    ]


def browser_lifecycle(series: dict[str, Any], current_year: int) -> list[dict[str, str]]:
    release_date = series.get("first_release_date")
    lifecycle = (
        "End of Life (EOL)" if browser_is_eol(series) else str(series.get("lifecycle") or series.get("status") or "n/a")
    )
    return [
        {
            "text": browser_release_age(release_date, current_year) or "date n/a",
            "title": f"First release: {release_date}" if release_date else "First release date is not recorded",
            "class_name": "",
        },
        {
            "text": lifecycle,
            "title": "End of Life release line" if browser_is_eol(series) else f"Lifecycle: {lifecycle}",
            "class_name": str(series.get("lifecycle") or series.get("status") or ""),
        },
    ]


def browser_matrix(data: dict[str, Any], current_year: int) -> dict[str, Any]:
    dimensions: list[dict[str, Any]] = []
    for key, label in BROWSER_DIMENSIONS:
        source_item = next(
            (item for item in data.get("dependency_versions", {}).get(key, []) if item.get("source")),
            {},
        )
        columns = [
            browser_column(data, key, label, item, current_year)
            for item in data.get("dependency_versions", {}).get(key, [])
        ]
        dimensions.append(
            {
                "key": key,
                "label": label,
                "source": browser_source_detail(data, source_item.get("source") or "") if source_item else None,
                "columns": columns,
            }
        )
    supported_series = [series for series in data.get("postgis_series", []) if not browser_is_eol(series)]
    for dimension in dimensions:
        dimension["visible"] = {
            "supported": browser_supported_column_indexes(dimension, supported_series),
            "history": list(range(len(dimension["columns"]))),
        }

    edge_indexes = browser_edge_indexes(data)
    patches_by_minor: dict[str, list[dict[str, Any]]] = {}
    for patch in data.get("patch_releases", []):
        patches_by_minor.setdefault(patch.get("minor"), []).append(patch)

    series_model: list[dict[str, Any]] = []
    for series in data.get("postgis_series", []):
        group = {
            "key": series["minor"],
            "label": series["minor"],
            "eol": browser_is_eol(series),
            "visible": {"supported": not browser_is_eol(series), "history": True},
            "lifecycle": browser_lifecycle(series, current_year),
            "source": browser_postgis_branch_news_link(series),
            "rows": {"series": [], "patches": []},
        }

        def make_row(patch: dict[str, Any] | None) -> dict[str, Any]:
            effective = browser_patch_series(series, patch)
            version = str(patch.get("version") if patch else series.get("latest") or "")
            row = {
                "key": patch.get("version") if patch else series["minor"],
                "kind": "patch" if patch else "series",
                "primary": version,
                "secondary": (
                    browser_release_age(patch.get("release_date"), current_year, month=True) or "patch" if patch else ""
                ),
                "eol": browser_is_eol(series),
                "release_date": str(patch.get("release_date") or "") if patch else "",
                "release": browser_postgis_news_link(version) if version else None,
                "source": browser_source_detail(data, str(patch.get("source") or "")) if patch else None,
                "cells": {},
            }
            for dimension in dimensions:
                row["cells"][dimension["key"]] = [
                    browser_cell(data, effective, dimension, column, edge_indexes) for column in dimension["columns"]
                ]
            return row

        base_row = make_row(None)
        group["rows"]["series"].append(base_row)
        for patch in patches_by_minor.get(series["minor"], []):
            patch_row = make_row(patch)
            overrides: dict[str, dict[int, dict[str, str]]] = {}
            for dimension in dimensions:
                key = dimension["key"]
                changed = {
                    str(index): cell
                    for index, cell in enumerate(patch_row["cells"][key])
                    if cell != base_row["cells"][key][index]
                }
                if changed:
                    overrides[key] = changed
            patch_row.pop("cells")
            patch_row["overrides"] = overrides
            group["rows"]["patches"].append(patch_row)
        series_model.append(group)

    for dimension in dimensions:
        for column in dimension["columns"]:
            column.pop("meta", None)

    notes: list[str] = []
    note_indexes: dict[str, int] = {}
    sources: list[dict[str, str]] = []
    source_indexes: dict[tuple[str, str, str], int] = {}

    def intern_cell(cell: dict[str, Any]) -> None:
        note = str(cell.pop("note", "") or "")
        if note:
            if note not in note_indexes:
                note_indexes[note] = len(notes)
                notes.append(note)
            cell["note"] = note_indexes[note]
        cell_sources = cell.pop("sources", [])
        indexes = []
        for source in cell_sources:
            key = (
                str(source.get("value") or ""),
                str(source.get("url") or ""),
                str(source.get("meta") or ""),
            )
            if key not in source_indexes:
                source_indexes[key] = len(sources)
                sources.append(source)
            indexes.append(source_indexes[key])
        if indexes:
            cell["sources"] = indexes

    for group in series_model:
        for row in group["rows"]["series"]:
            for cells in row["cells"].values():
                for cell in cells:
                    intern_cell(cell)
        for row in group["rows"]["patches"]:
            for overrides in row["overrides"].values():
                for cell in overrides.values():
                    intern_cell(cell)

    statuses = {
        key: {
            **value,
            "definition": data.get("status_definitions", {}).get(key, value["label"]),
        }
        for key, value in BROWSER_STATUSES.items()
    }
    return {
        "defaults": {
            "view": "overview",
            "coverage": "supported",
            "row_detail": "series",
        },
        "labels": {
            "row_axis": "PostGIS",
            "legend": "Legend",
            "patch_release_prefix": "PostGIS patch release: ",
            "source_prefix": "source: ",
        },
        "controls": {
            "groups": [
                {
                    "key": "view",
                    "label": "Matrix view",
                    "class_name": "segments",
                    "label_class": "label",
                    "options": [{"value": "overview", "label": "Overview"}]
                    + [{"value": item["key"], "label": item["label"]} for item in dimensions],
                },
                {
                    "key": "coverage",
                    "label": "Coverage",
                    "class_name": "toggles",
                    "label_class": "label",
                    "options": [
                        {"value": "supported", "label": "Supported"},
                        {"value": "history", "label": "Full history"},
                    ],
                },
                {
                    "key": "row_detail",
                    "label": "PostGIS rows",
                    "class_name": "row-detail",
                    "label_class": "caption",
                    "options": [
                        {"value": "series", "label": "Series"},
                        {"value": "patches", "label": "Patch releases"},
                    ],
                },
            ],
            "hint": (
                "Full history expands PostGIS series and dependency-version coverage; "
                "PostGIS row detail is controlled separately."
            ),
        },
        "statuses": statuses,
        "notes": notes,
        "sources": sources,
        "dimensions": dimensions,
        "series": series_model,
    }


def browser_status_count(item: dict[str, Any]) -> int:
    return sum(int(count or 0) for count in item.get("status_counts", {}).values())


def browser_repository_count(item: dict[str, Any]) -> int:
    return int(item.get("repository_count") or browser_status_count(item))


def browser_numeric_version_key(value: str) -> tuple[int, ...] | None:
    match = re.fullmatch(r"[vV]?(\d+(?:[._-]\d+)+)", str(value).strip())
    if not match:
        return None
    parts = [int(part) for part in re.split(r"[._-]", match.group(1))]
    while len(parts) > 2 and parts[-1] == 0:
        parts.pop()
    return tuple(parts)


def browser_version_equivalent(left: str, right: str) -> bool:
    left_key = browser_numeric_version_key(left)
    return left_key is not None and left_key == browser_numeric_version_key(right)


def browser_repology_url(packaged: dict[str, Any]) -> str:
    if packaged.get("project"):
        return f"https://repology.org/project/{quote(str(packaged['project']), safe='')}/versions"
    return str(packaged.get("url") or "").replace("/api/v1/project/", "/project/") + "/versions"


def browser_packaged_summary(
    data: dict[str, Any],
    key: str,
    recommended: str | None,
    include_matching: bool = True,
) -> dict[str, Any] | None:
    packaged = data.get("packaged_versions", {}).get(key, {})
    same_count = 0
    versions: list[dict[str, str]] = []
    for item in packaged.get("newest", []):
        if recommended and browser_version_equivalent(str(item.get("version")), recommended):
            same_count += browser_repository_count(item)
        elif item.get("version"):
            versions.append({"text": str(item["version"]), "kind": "newest"})
    versions.extend(
        {"text": str(item["version"]), "kind": "development"}
        for item in packaged.get("development", [])
        if item.get("version")
    )
    deduplicated: list[dict[str, str]] = []
    for item in versions:
        equivalent = next(
            (
                existing
                for existing in deduplicated
                if existing["kind"] == item["kind"]
                and (existing["text"] == item["text"] or browser_version_equivalent(existing["text"], item["text"]))
            ),
            None,
        )
        if equivalent:
            if len(item["text"]) < len(equivalent["text"]):
                equivalent["text"] = item["text"]
            continue
        deduplicated.append(item)
    pills = [{"text": f"same in {same_count} repositories", "kind": "same"}] if same_count and include_matching else []
    pills.extend(deduplicated[:4])
    if not pills:
        if packaged.get("newest") or packaged.get("development"):
            return None
        return {
            "url": "",
            "title": f"No package versions are available for {packaged.get('project') or key}",
            "label": "packages",
            "kind": "packages",
            "pills": [{"text": "unavailable", "kind": "unavailable"}],
        }
    return {
        "url": browser_repology_url(packaged) if packaged.get("url") else "",
        "title": f"Repology package versions for {packaged.get('project') or key}",
        "label": "packages",
        "kind": "packages",
        "pills": pills,
    }


def browser_newest_version(
    data: dict[str, Any],
    key: str,
    predicate: Any | None = None,
) -> dict[str, Any] | None:
    versions = [
        item for item in data.get("dependency_versions", {}).get(key, []) if predicate is None or predicate(item)
    ]
    return max(
        versions,
        key=lambda item: version_sort_key(str(item.get("current_minor") or item.get("version") or "")),
        default=None,
    )


def browser_newest_packaged_version(data: dict[str, Any], key: str) -> str:
    versions = data.get("packaged_versions", {}).get(key, {}).get("newest", [])
    values = [str(item.get("version") or "") for item in versions if item.get("version")]
    if not values:
        return ""
    newest = max(values, key=version_sort_key)
    equivalents = [value for value in values if browser_version_equivalent(value, newest)]
    return min(
        equivalents or [newest],
        key=lambda value: (
            not bool(re.fullmatch(r"[vV]?\d+(?:\.\d+)+", value)),
            len(value),
            value,
        ),
    )


def browser_pin_maintenance_action(
    data: dict[str, Any], item: dict[str, Any], packaged_key: str
) -> dict[str, Any] | None:
    pin = item.get("pin") or {}
    selected = str(pin.get("value") or "")
    candidate = browser_newest_packaged_version(data, packaged_key)
    if not selected or not candidate or compare_versions(candidate, selected) <= 0:
        return None

    selections = [
        {
            "label": str(pin.get("context") or "Selected version"),
            "value": selected,
            "source": pin.get("source") or item.get("source") or "",
        },
        *[
            {
                "label": str(override.get("context") or "Override"),
                "value": str(override.get("value") or ""),
                "source": override.get("source") or pin.get("source") or "",
            }
            for override in pin.get("overrides") or []
            if override.get("value")
        ],
    ]
    packaged = data.get("packaged_versions", {}).get(packaged_key, {})
    links = []
    for selection in selections:
        source = browser_source_detail(data, selection.pop("source"))
        if source.get("url") and not any(link["url"] == source["url"] for link in links):
            links.append(
                {
                    "label": selection["label"],
                    "url": source["url"],
                    "title": source.get("value", ""),
                }
            )
    repology_url = browser_repology_url(packaged) if packaged.get("url") else ""
    if repology_url:
        links.append(
            {
                "label": "Package versions",
                "url": repology_url,
                "title": f"Repology package versions for {packaged.get('project') or packaged_key}",
            }
        )
    return {
        "id": f"pin-{item.get('id')}",
        "component": str(item.get("label") or item.get("id") or "Dependency"),
        "title": "Review selected version",
        "summary": "; ".join(
            [
                *(f"{selection['label']} {selection['value']}" for selection in selections),
                f"newest packaged {candidate}",
            ]
        ),
        "guidance": str(
            pin.get("maintenance_note") or "Verify the newer version in the owning build before changing its selection."
        ),
        "links": links,
    }


def browser_vendored_maintenance_action(data: dict[str, Any], item: dict[str, Any]) -> dict[str, Any] | None:
    vendored = data.get("vendored_inventory", {}).get(item.get("id")) or {}
    status = str(vendored.get("status") or "")
    if status not in {"behind", "tracked-unversioned", "unknown"}:
        return None
    upstream = vendored.get("upstream_latest") or {}
    current = str(vendored.get("vendored_version") or vendored.get("vendored_label") or "version not recorded")
    latest = str(upstream.get("version") or "upstream version not recorded")
    title = "Update vendored source" if status == "behind" else "Record vendored source version"
    links = []
    if upstream.get("url"):
        links.append(
            {
                "label": "Upstream releases",
                "url": str(upstream["url"]),
                "title": f"Upstream releases for {item.get('label') or item.get('id')}",
            }
        )
    return {
        "id": f"vendored-{item.get('id')}",
        "component": str(item.get("label") or item.get("id") or "Vendored source"),
        "title": title,
        "summary": f"in tree {current}; upstream {latest}",
        "guidance": str(
            vendored.get("maintenance_note") or vendored.get("note") or "Review and refresh the vendored source."
        ),
        "links": links,
    }


def browser_maintenance(data: dict[str, Any]) -> dict[str, Any]:
    actions = []
    for item in data.get("dependency_catalog", []):
        packaged_key = browser_project_key(str(item.get("id") or ""))
        pin_action = browser_pin_maintenance_action(data, item, packaged_key)
        if pin_action:
            actions.append(pin_action)
        vendored_action = browser_vendored_maintenance_action(data, item)
        if vendored_action:
            actions.append(vendored_action)
    return {
        "title": "Maintainer actions",
        "intro": "Source-owned dependency selections that may need review.",
        "empty": "No dependency selections currently need review.",
        "items": actions,
    }


def browser_stable_postgis(data: dict[str, Any]) -> dict[str, Any]:
    stable = [
        item
        for item in data.get("postgis_series", [])
        if item.get("lifecycle") == "stable" and not browser_is_eol(item) and item.get("latest")
    ]
    return max(
        stable,
        key=lambda item: version_sort_key(str(item.get("latest") or item.get("minor") or "")),
        default=(data.get("postgis_series") or [{}])[0],
    )


def browser_recommended(
    data: dict[str, Any],
    item: dict[str, Any],
) -> dict[str, str]:
    item_id = str(item.get("id") or "")
    key = browser_gate_key(item_id)
    stable_postgis = browser_stable_postgis(data)
    if key == "postgis":
        value = str(stable_postgis.get("latest") or stable_postgis.get("minor") or "")
        patch = next(
            (patch for patch in data.get("patch_releases", []) if patch.get("version") == value),
            {},
        )
        release = browser_postgis_news_link(value) if value else {}
        return {
            "value": value,
            "detail": f"released {patch['release_date']}" if patch.get("release_date") else "",
            "url": release.get("url", ""),
            "title": release.get("title", ""),
        }
    if key == "postgresql":
        value = browser_newest_version(
            data,
            "postgresql",
            lambda candidate: (
                not browser_is_eol(candidate)
                and not browser_is_development(candidate)
                and browser_in_postgresql_range(
                    str(candidate.get("version")),
                    stable_postgis.get("postgresql", {}).get("supported", ""),
                )
            ),
        )
        if value:
            release = browser_dependency_release_link("postgresql", value) or {}
            return {
                "value": str(value.get("current_minor") or value.get("version") or ""),
                "detail": (f"major since {value['first_release_date']}" if value.get("first_release_date") else ""),
                "url": release.get("url", ""),
                "title": release.get("title", ""),
            }
    if key in {dimension[0] for dimension in BROWSER_DIMENSIONS[1:]}:
        value = browser_newest_version(
            data,
            key,
            lambda candidate: not browser_is_eol(candidate) and not browser_is_development(candidate),
        )
        if value:
            release = browser_dependency_release_link(key, value) or {}
            return {
                "value": str(value.get("current_minor") or value.get("version") or ""),
                "detail": (f"line since {value['first_release_date']}" if value.get("first_release_date") else ""),
                "url": release.get("url", ""),
                "title": release.get("title", ""),
            }
    pin = item.get("pin") or {}
    if pin.get("value"):
        return {
            "value": str(pin["value"]),
            "detail": str(pin.get("context") or "selected version"),
            "source": "pinned",
            "url": "",
            "title": "",
        }
    packaged = browser_newest_packaged_version(data, browser_project_key(item_id))
    fallback = str(packaged or item.get("minimum") or "")
    packaged_data = data.get("packaged_versions", {}).get(browser_project_key(item_id), {})
    packaged_match = next(
        (
            candidate
            for candidate in packaged_data.get("newest", [])
            if packaged and browser_version_equivalent(str(candidate.get("version") or ""), packaged)
        ),
        None,
    )
    repository_count = browser_repository_count(packaged_match or {})
    repository_label = "repository" if repository_count == 1 else "repositories"
    return {
        "value": fallback or "not specified",
        "detail": (
            f"latest packaged in {repository_count} {repository_label}"
            if packaged and repository_count
            else "latest packaged"
            if packaged
            else "minimum"
            if fallback
            else ""
        ),
        "source": "packaged" if packaged else "minimum",
        "url": browser_repology_url(packaged_data) if packaged_data.get("url") else "",
        "title": (f"Repology package versions for {packaged_data.get('project') or item_id}" if packaged else ""),
    }


def browser_vendored_summary(data: dict[str, Any], key: str) -> dict[str, Any] | None:
    item = data.get("vendored_inventory", {}).get(key)
    if not item:
        return None
    upstream = item.get("upstream_latest", {})
    version = item.get("vendored_version") or item.get("vendored_label") or "version not recorded"
    if (
        item.get("vendored_snapshot")
        and item.get("vendored_snapshot") != "snapshot"
        and not item.get("vendored_version")
        and not item.get("vendored_label")
    ):
        version += f" {item['vendored_snapshot']}"
    if item.get("status") == "current" and upstream.get("version"):
        state = f"in tree; matches upstream {upstream['version']}"
    elif item.get("status") == "behind" and upstream.get("version"):
        state = f"upstream {upstream['version']} available"
    elif item.get("status") == "tracked-unversioned" and upstream.get("version"):
        state = f"upstream {upstream['version']}"
    else:
        state = "; ".join(
            part
            for part in (
                item.get("status") or "status unknown",
                f"upstream {upstream['version']}" if upstream.get("version") else "",
            )
            if part
        )
    for name, embedded in item.get("embedded_versions", {}).items():
        state += f"; {name} {embedded}"
    return {
        "type": "vendored",
        "url": str(upstream.get("url") or ""),
        "title": "; ".join(
            part
            for part in (
                f"vendored source: {item['vendored_source']}" if item.get("vendored_source") else "",
                (
                    f"upstream source: {browser_source_display(data, str(upstream['source']))}"
                    if upstream.get("source")
                    else ""
                ),
                item.get("note"),
            )
            if part
        ),
        "version": str(version),
        "state": state,
        "status": str(item.get("status") or "unknown"),
    }


def browser_dependency_inventory(data: dict[str, Any]) -> dict[str, Any]:
    stable_postgis = browser_stable_postgis(data)
    rows = [
        {
            "id": "postgis",
            "label": "PostGIS",
            "kind": "project",
            "requirement": "stable release line",
            "used_for": ["extension release baseline"],
            "source": {
                "title": "PostGIS release and support policy",
                "url": data.get("policy_url", ""),
                "as_of": str(data.get("generated_at") or "")[:10],
            },
        },
        *data.get("dependency_catalog", []),
    ]
    grouped: dict[str, list[dict[str, Any]]] = {}
    for item in rows:
        gate_key = browser_gate_key(str(item.get("id") or ""))
        recommendation = browser_recommended(data, item)
        gate = (
            stable_postgis.get("postgresql", {})
            if gate_key == "postgresql"
            else stable_postgis.get("dependencies", {}).get(gate_key)
        ) or browser_source_gate_summary(data, gate_key)
        if item.get("kind") == "vendored":
            primary = browser_vendored_summary(data, str(item["id"])) or {
                "type": "text",
                "value": "in-tree source",
            }
        else:
            primary = {
                "type": "version",
                **recommendation,
            }
        packaged_key = browser_project_key(str(item.get("id") or ""))
        secondary = browser_packaged_summary(
            data,
            packaged_key,
            None if item.get("kind") == "vendored" else recommendation["value"],
            include_matching=recommendation.get("source") != "packaged",
        )
        details = []
        requirement = "; ".join(
            part
            for part in (
                str(item.get("requirement") or "n/a"),
                f"minimum {item['minimum']}" if item.get("minimum") else "",
            )
            if part
        )
        for label, value, class_name in (
            ("Requirement", requirement, ""),
            ("Used for", "; ".join(item.get("used_for", [])), ""),
            (
                "Source gates",
                gate.get("source_gate_summary") if gate else "",
                "gates",
            ),
            ("Feature note", gate.get("feature_note") if gate else "", "gates"),
            ("Detected by", "; ".join(item.get("detected_by", [])), "evidence"),
        ):
            if value:
                details.append({"label": label, "value": value, "class_name": class_name})
        release_docs_ci = item.get("release_docs_ci") or {}
        if release_docs_ci:
            selected = str(release_docs_ci.get("selected") or "")
            minimum = str(release_docs_ci.get("minimum") or "")
            details.append(
                {
                    "label": "Release docs CI",
                    "value": " and ".join(
                        part
                        for part in (
                            f"uses {selected}" if selected else "",
                            (f"rejects versions older than {minimum}" if minimum else ""),
                        )
                        if part
                    ),
                    "class_name": "evidence",
                }
            )
        pin = item.get("pin") or {}
        for override in pin.get("overrides") or []:
            override_source = browser_source_detail(data, override.get("source") or "")
            details.append(
                {
                    "label": str(override.get("context") or "Winnie override"),
                    "value": str(override.get("value") or "not specified"),
                    "class_name": "attention",
                    "url": override_source.get("url", ""),
                    "meta": override_source.get("meta", ""),
                }
            )
        source = browser_source_detail(data, item.get("source") or "")
        if source.get("value"):
            details.append(
                {
                    "label": "Source",
                    "class_name": "evidence",
                    **source,
                }
            )
        vendored = data.get("vendored_inventory", {}).get(item.get("id"))
        if vendored:
            if vendored.get("vendored_source"):
                details.append(
                    {
                        "label": "In-tree source",
                        "value": vendored["vendored_source"],
                        "class_name": "evidence",
                    }
                )
            if vendored.get("note"):
                details.append(
                    {
                        "label": "Version note",
                        "value": vendored["note"],
                        "class_name": "evidence",
                    }
                )
        row = {
            "id": item["id"],
            "label": item.get("label") or item["id"],
            "kind": item.get("kind") or "dependency",
            "url": PROJECT_URLS.get(browser_project_key(str(item["id"])), ""),
            "role": "; ".join(item.get("used_for", [])) or str(item.get("requirement") or "n/a"),
            "summary_title": "Show requirement and source evidence",
            "primary": primary,
            "secondary": secondary,
            "details": details,
        }
        grouped.setdefault(browser_dependency_group(item), []).append(row)

    groups = []
    for key, label, note in DEPENDENCY_GROUPS:
        items = grouped.get(key, [])
        if items:
            groups.append({"key": key, "label": label, "note": note, "items": items})
    return {
        "title": "Dependency inventory",
        "intro": (
            "Recommended targets, vendored source versions, and package freshness "
            "from configure.ac, CI scripts, release tags, and Repology."
        ),
        "snapshot": f"{len(rows)} components · refreshed {str(data.get('generated_at') or '')[:10]}",
        "groups": groups,
    }


def build_payload_model(data: dict[str, Any]) -> dict[str, Any]:
    generated = dt.datetime.fromisoformat(str(data["generated_at"]).replace("Z", "+00:00"))
    warnings = [
        {
            "title": "Compatibility metadata warning",
            "source": browser_warning_source(data, str(warning.get("source") or "unknown")),
            "message": (
                f"{warning.get('message') or 'unknown error'}; using {warning.get('fallback') or 'cached data'}."
            ),
        }
        for warning in data.get("update_warnings", [])
    ]
    return {
        "schema_version": PAYLOAD_SCHEMA_VERSION,
        "meta": f"Generated from {data.get('source_repository') or 'unknown source'} at {data['generated_at']}.",
        "warnings": warnings,
        "matrix": browser_matrix(data, generated.year),
        "inventory": browser_dependency_inventory(data),
        "maintenance": browser_maintenance(data),
        "provenance": {
            "text": (
                f"Upgrade paths: {len(data.get('upgrade_paths', {}).get('known_versions', []))} known source versions."
            ),
            "raw_label": "Raw compatibility JSON",
        },
    }


def validate_payload_model(model: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    if model.get("schema_version") != PAYLOAD_SCHEMA_VERSION:
        errors.append(f"browser.schema_version must be {PAYLOAD_SCHEMA_VERSION}")
    matrix = model.get("matrix", {})
    dimensions = matrix.get("dimensions", [])
    series = matrix.get("series", [])
    statuses = matrix.get("statuses", {})
    notes = matrix.get("notes", [])
    sources = matrix.get("sources", [])
    labels = matrix.get("labels", {})

    def validate_display_text(value: Any, path: tuple[str, ...] = ()) -> None:
        if isinstance(value, dict):
            for key, child in value.items():
                if key != "url":
                    validate_display_text(child, (*path, str(key)))
        elif isinstance(value, list):
            for index, child in enumerate(value):
                validate_display_text(child, (*path, str(index)))
        elif isinstance(value, str):
            match = INTERNAL_SOURCE_REFERENCE.search(value)
            if match:
                errors.append(f"browser.{'.'.join(path)} exposes internal source id {match.group(0)!r}")

    validate_display_text(model)

    def validate_link(link: Any, path: str, *, required: bool = False) -> None:
        if not link:
            if required:
                errors.append(f"{path}: link is required")
            return
        if not isinstance(link, dict):
            errors.append(f"{path}: link must be an object")
            return
        if not link.get("value") or not str(link.get("url") or "").startswith(("http://", "https://", "/")):
            errors.append(f"{path}: human label and URL are required")

    for index, source in enumerate(sources):
        if not source.get("value") or not str(source.get("url") or "").startswith(("http://", "https://")):
            errors.append(f"browser.matrix.sources.{index}: human label and URL are required")
    for index, warning in enumerate(model.get("warnings", [])):
        source = warning.get("source") or {}
        if not source.get("value") or not str(source.get("url") or "").startswith(("http://", "https://")):
            errors.append(f"browser.warnings.{index}.source: human label and URL are required")
    for key in ("row_axis", "legend", "patch_release_prefix", "source_prefix"):
        if not labels.get(key):
            errors.append(f"browser.matrix.labels.{key} is required")
    controls = matrix.get("controls", {})
    control_groups = controls.get("groups", [])
    control_keys = [group.get("key") for group in control_groups]
    if not control_groups:
        errors.append("browser.matrix.controls.groups must not be empty")
    if len(set(control_keys)) != len(control_keys):
        errors.append("browser.matrix.controls group keys must be unique")
    defaults = matrix.get("defaults", {})
    for group in control_groups:
        key = group.get("key")
        values = [option.get("value") for option in group.get("options", [])]
        if not key or not values:
            errors.append("browser.matrix.controls groups require a key and options")
        elif defaults.get(key) not in values:
            errors.append(f"browser.matrix.defaults.{key} must match a generated control option")
    for key in defaults:
        if key not in control_keys:
            errors.append(f"browser.matrix.defaults.{key} has no generated control")
    for group in model.get("inventory", {}).get("groups", []):
        for item in group.get("items", []):
            item_sources = [fact for fact in item.get("details", []) if fact.get("label") == "Source"]
            if len(item_sources) != 1:
                errors.append(f"browser.inventory.{item.get('id')}: expected one Source detail")
                continue
            source = item_sources[0]
            if not source.get("url") or SOURCE_ID_DATE.fullmatch(str(source.get("value") or "")):
                errors.append(f"browser.inventory.{item.get('id')}: Source must be a linked human label")
    maintenance_ids = []
    for item in model.get("maintenance", {}).get("items", []):
        maintenance_ids.append(item.get("id"))
        if not item.get("component") or not item.get("summary") or not item.get("guidance"):
            errors.append(f"browser.maintenance.{item.get('id')}: component, summary, and guidance are required")
        for link in item.get("links", []):
            if not link.get("label") or not str(link.get("url") or "").startswith(("http://", "https://")):
                errors.append(f"browser.maintenance.{item.get('id')}: links require a human label and URL")
    if len(maintenance_ids) != len(set(maintenance_ids)):
        errors.append("browser.maintenance item ids must be unique")
    if not dimensions:
        errors.append("browser.matrix.dimensions must not be empty")
    if not series:
        errors.append("browser.matrix.series must not be empty")
    for dimension in dimensions:
        columns = dimension.get("columns", [])
        if not columns:
            errors.append(f"browser.matrix.{dimension.get('key')}: columns must not be empty")
        for coverage in ("supported", "history"):
            visible = dimension.get("visible", {}).get(coverage, [])
            if any(not isinstance(index, int) or index < 0 or index >= len(columns) for index in visible):
                errors.append(f"browser.matrix.{dimension.get('key')}.{coverage}: invalid column index")
        for column in columns:
            validate_link(
                column.get("release"),
                f"browser.matrix.{dimension.get('key')}.{column.get('key')}.release",
            )
            source = column.get("source") or {}
            if not source.get("value") or not str(source.get("url") or "").startswith(("http://", "https://")):
                errors.append(
                    f"browser.matrix.{dimension.get('key')}.{column.get('key')}.source: "
                    "human label and URL are required"
                )
    column_counts = {dimension.get("key"): len(dimension.get("columns", [])) for dimension in dimensions}
    for group in series:
        validate_link(group.get("source"), f"browser.matrix.{group.get('key')}.source", required=True)
        base_rows = group.get("rows", {}).get("series", [])
        if len(base_rows) != 1:
            errors.append(f"browser.matrix.{group.get('key')}: expected one series row")
            continue
        base = base_rows[0]
        for dimension, count in column_counts.items():
            cells = base.get("cells", {}).get(dimension)
            if not isinstance(cells, list) or len(cells) != count:
                errors.append(f"browser.matrix.{group.get('key')}.{dimension}: expected {count} base cells")
                continue
            for cell in cells:
                if cell.get("status") not in statuses:
                    errors.append(
                        f"browser.matrix.{group.get('key')}.{dimension}: unknown status {cell.get('status')!r}"
                    )
                if "note" in cell and (
                    not isinstance(cell["note"], int) or cell["note"] < 0 or cell["note"] >= len(notes)
                ):
                    errors.append(f"browser.matrix.{group.get('key')}.{dimension}: invalid note index")
                if any(
                    not isinstance(index, int) or index < 0 or index >= len(sources)
                    for index in cell.get("sources", [])
                ):
                    errors.append(f"browser.matrix.{group.get('key')}.{dimension}: invalid source index")
        for row in group.get("rows", {}).get("patches", []):
            if "cells" in row:
                errors.append(
                    f"browser.matrix.{group.get('key')}.{row.get('key')}: "
                    "patch rows must contain sparse overrides, not full cells"
                )
            for dimension, overrides in row.get("overrides", {}).items():
                if dimension not in column_counts:
                    errors.append(
                        f"browser.matrix.{group.get('key')}.{row.get('key')}: unknown override dimension {dimension}"
                    )
                    continue
                for index, cell in overrides.items():
                    if (
                        not isinstance(index, str)
                        or not index.isdecimal()
                        or str(int(index)) != index
                        or int(index) >= column_counts[dimension]
                    ):
                        errors.append(
                            f"browser.matrix.{group.get('key')}.{row.get('key')}.{dimension}: "
                            f"invalid override index {index}"
                        )
                    if cell.get("status") not in statuses:
                        errors.append(
                            f"browser.matrix.{group.get('key')}.{row.get('key')}.{dimension}: "
                            f"unknown status {cell.get('status')!r}"
                        )
                    if "note" in cell and (
                        not isinstance(cell["note"], int) or cell["note"] < 0 or cell["note"] >= len(notes)
                    ):
                        errors.append(
                            f"browser.matrix.{group.get('key')}.{row.get('key')}.{dimension}: invalid note index"
                        )
                    if any(
                        not isinstance(source_index, int) or source_index < 0 or source_index >= len(sources)
                        for source_index in cell.get("sources", [])
                    ):
                        errors.append(
                            f"browser.matrix.{group.get('key')}.{row.get('key')}.{dimension}: invalid source index"
                        )
        for row in group.get("rows", {}).get("patches", []):
            source = row.get("source") or {}
            if not source.get("value") or not str(source.get("url") or "").startswith(("http://", "https://")):
                errors.append(
                    f"browser.matrix.{group.get('key')}.{row.get('key')}.source: human label and URL are required"
                )
        for row in [*base_rows, *group.get("rows", {}).get("patches", [])]:
            validate_link(row.get("release"), f"browser.matrix.{group.get('key')}.{row.get('key')}.release")
    inventory = model.get("inventory", {})
    items = [item for group in inventory.get("groups", []) for item in group.get("items", [])]
    if not items:
        errors.append("browser.inventory must not be empty")
    if len({item.get("id") for item in items}) != len(items):
        errors.append("browser.inventory item ids must be unique")
    for item in items:
        if not item.get("url"):
            errors.append(f"browser.inventory.{item.get('id')}: project URL is required")
        if not item.get("primary"):
            errors.append(f"browser.inventory.{item.get('id')}: primary value is required")
        if not (item.get("secondary") or {}).get("pills") and item.get("primary", {}).get("source") != "packaged":
            errors.append(f"browser.inventory.{item.get('id')}: package summary is required")
    return errors
