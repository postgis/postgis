"""Refresh source-derived support matrix metadata with cached fallback."""

from __future__ import annotations

import datetime as dt
import html
import json
import re
import subprocess
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

from . import DEFAULT_CACHE, DEFAULT_MATRIX, REPOSITORY_ROOT

ROOT = REPOSITORY_ROOT
USER_AGENT = "PostGIS support matrix updater (curl/8.5.0 compatible)"
SOURCE_DATE = dt.datetime.now(dt.timezone.utc).date().isoformat()
CACHE_SCHEMA_VERSION = 1

GENERATED_SOURCE_PREFIXES = (
    "postgresql-versioning-",
    "postgis-configure-ac-",
    "geos-download-",
    "proj-download-",
    "gdal-download-",
    "sfcgal-gitlab-releases-",
    "postgis-ci-winnie-deps-",
    "postgis-ci-winnie-package-",
    "postgis-release-docs-ci-",
    "postgis-release-configure-ac-",
    "postgis-source-gates-",
    "repology-",
    "github-tags-",
)

URLS = {
    "postgresql": "https://www.postgresql.org/support/versioning/",
    "geos": "https://libgeos.org/usage/download/",
    "proj": "https://download.osgeo.org/proj/",
    "gdal": "https://gdal.org/en/stable/download.html",
    "sfcgal": "https://gitlab.com/api/v4/projects/sfcgal%2FSFCGAL/releases?per_page=100",
}

AUXILIARY_URLS = {
    "gdal-past": "https://gdal.org/en/stable/download_past.html",
}

REPOLOGY_PROJECTS = {
    "postgis": "postgis",
    "postgresql": "postgresql",
    "geos": "geos",
    "proj": "proj",
    "gdal": "gdal",
    "sfcgal": "sfcgal",
    "libxml2": "libxml2",
    "libiconv": "libiconv",
    "gmp": "gmp",
    "protobuf-c": "protobuf-c",
    "json-c": "json-c",
    "zlib": "zlib",
    "lz4": "lz4",
    "sqlite": "sqlite",
    "boost": "boost",
    "cgal": "cgal",
    "cunit": "cunit-original",
    "gettext": "gettext",
    "wagyu": "wagyu",
    "ryu": "ryu",
    "docbook-xsl": "docbook-xsl",
    "docbook-xml": "docbook-xml",
    "libxslt": "libxslt",
    "xmlto": "xmlto",
    "dblatex": "dblatex",
    "imagemagick": "imagemagick",
}

GITHUB_TAG_PROJECTS = {
    "wagyu": {
        "repo": "mapbox/wagyu",
        "pattern": r"^\d+\.\d+\.\d+$",
    },
    "ryu": {
        "repo": "ulfjack/ryu",
        "pattern": r"^v?\d+\.\d+$",
    },
    "flatgeobuf": {
        "repo": "flatgeobuf/flatgeobuf",
        "pattern": r"^\d+\.\d+\.\d+$",
    },
    "uthash": {
        "repo": "troydhanson/uthash",
        "pattern": r"^v?\d+\.\d+\.\d+$",
    },
}

SOURCE_GATE_PATHS = (
    "libpgcommon",
    "liblwgeom",
    "postgis",
    "raster",
    "sfcgal",
    "loader",
)
SOURCE_GATE_MACROS = {
    "POSTGIS_PGSQL_VERSION": "postgresql",
    "POSTGIS_GEOS_VERSION": "geos",
    "POSTGIS_PROJ_VERSION": "proj",
    "POSTGIS_GDAL_VERSION": "gdal",
    "GDAL_VERSION_NUM": "gdal",
    "POSTGIS_SFCGAL_VERSION": "sfcgal",
    "HAVE_SFCGAL": "sfcgal",
    "HAVE_LIBJSON": "json-c",
    "JSON_C_VERSION_NUM": "json-c",
    "JSON_C_VERSION": "json-c",
    "HAVE_LIBPROTOBUF": "protobuf-c",
    "HAVE_ICONV": "iconv",
    "HAVE_ICONVCTL": "iconv",
    "PROJ_GEODESIC": "proj",
}
SOURCE_GATE_CLASSIFICATION = {
    ("liblwgeom/liblwgeom.h.in", "POSTGIS_GEOS_VERSION>=31100"): (
        "feature",
        "GEOS 3.11 exposes concave hull support declarations.",
    ),
    ("liblwgeom/lwgeom_geos.c", "POSTGIS_GEOS_VERSION<31100"): (
        "feature",
        "GEOS 3.11 adds directed line merge support.",
    ),
    ("liblwgeom/lwgeom_geos.c", "POSTGIS_GEOS_VERSION>=31100"): (
        "feature",
        "GEOS 3.11 adds concave hull of polygons support.",
    ),
    ("liblwgeom/lwgeom_geos.c", "POSTGIS_GEOS_VERSION>=31200"): (
        "performance",
        "GEOS 3.12 adds GEOSPreparedIntersectsXY fast point-in-polygon sampling.",
    ),
    ("liblwgeom/lwgeom_geos.h", "POSTGIS_GEOS_VERSION<31300"): (
        "compatibility",
        "GEOS before 3.13 needs a local GEOSMessageHandler typedef.",
    ),
    ("liblwgeom/lwgeom_geos_cluster.c", "POSTGIS_GEOS_VERSION>=31300"): (
        "performance",
        "GEOS 3.13 enables STRtree-assisted relate-pattern clustering.",
    ),
    ("liblwgeom/lwgeom_geos_split.c", "POSTGIS_GEOS_VERSION>=31500"): (
        "behavior",
        "GEOS 3.15 adds explicit line and polygon split support and GEOSSplit-specific errors.",
    ),
    ("liblwgeom/lwgeom_sfcgal.c", "POSTGIS_SFCGAL_VERSION>=10308"): (
        "feature",
        "SFCGAL 1.3.8 adds measured-coordinate detection and extraction.",
    ),
    ("liblwgeom/lwgeom_sfcgal.c", "POSTGIS_SFCGAL_VERSION>=10400"): (
        "diagnostic",
        "SFCGAL 1.4 adds full version metadata including CGAL and Boost.",
    ),
    ("liblwgeom/lwgeom_sfcgal.c", "POSTGIS_SFCGAL_VERSION>=10500"): (
        "feature",
        "SFCGAL 1.5 adds XYM/XYZM point construction.",
    ),
    ("liblwgeom/lwgeom_sfcgal.c", "POSTGIS_SFCGAL_VERSION>=20100"): (
        "compatibility",
        "SFCGAL 2.1 renamed patch/geometry collection APIs.",
    ),
    ("liblwgeom/lwgeom_sfcgal.c", "POSTGIS_SFCGAL_VERSION>=20300"): (
        "feature",
        "SFCGAL 2.3 adds NURBSCURVE type mapping and bidirectional conversion.",
    ),
    (
        "liblwgeom/lwin_geojson.c",
        "!defined(JSON_C_VERSION_NUM)||JSON_C_VERSION_NUM<JSON_C_VERSION_013",
    ): (
        "compatibility",
        "json-c before 0.13 needs the private json_object header.",
    ),
    ("liblwgeom/lwin_geojson.c", "HAVE_LIBJSON"): (
        "optional",
        "GeoJSON input parsing is unavailable without JSON-C.",
    ),
    ("liblwgeom/lwin_geojson.c", "JSON_C_VERSION"): (
        "compatibility",
        "json-c before 0.10 needs json_tokener_error_desc compatibility glue.",
    ),
    ("liblwgeom/lwin_geojson.c", "defined(HAVE_LIBJSON)"): (
        "optional",
        "JSON-C enables GeoJSON input parsing.",
    ),
    ("liblwgeom/lwspheroid.c", "PROJ_GEODESIC"): (
        "behavior",
        "PROJ geodesic support selects Karney calculations; otherwise PostGIS uses local fallback routines.",
    ),
    ("loader/shp2pgsql-core.c", "HAVE_ICONVCTL"): (
        "feature",
        "iconvctl enables stricter character-conversion behavior in shp2pgsql when available.",
    ),
    ("postgis/geobuf.c", "definedHAVE_LIBPROTOBUF"): (
        "optional",
        "Geobuf support is compiled only with protobuf-c.",
    ),
    ("postgis/geobuf.h", "definedHAVE_LIBPROTOBUF"): (
        "optional",
        "Geobuf support is compiled only with protobuf-c.",
    ),
    ("postgis/geography_measurement.c", "PROJ_GEODESIC"): (
        "behavior",
        "PROJ geodesic support changes geography distance rounding and spheroid fallback behavior.",
    ),
    ("postgis/lwgeom_geos.c", "POSTGIS_GEOS_VERSION<31100"): (
        "feature",
        "GEOS 3.11 is required for directed ST_LineMerge, polygonal ST_ConcaveHull, and ST_TriangulatePolygon.",
    ),
    ("postgis/lwgeom_geos_predicates.c", "POSTGIS_GEOS_VERSION>=31300"): (
        "performance",
        "GEOS 3.13 enables prepared relate-pattern predicates.",
    ),
    ("postgis/lwgeom_in_geojson.c", "HAVE_LIBJSON"): (
        "optional",
        "GeoJSON input parsing is unavailable without JSON-C.",
    ),
    ("postgis/lwgeom_in_geojson.c", "defined(HAVE_LIBJSON)"): (
        "optional",
        "JSON-C enables GeoJSON input parsing.",
    ),
    ("postgis/lwgeom_in_geojson.c", "JSON_C_VERSION"): (
        "diagnostic",
        "json-c exposes its runtime version when JSON_C_VERSION is available.",
    ),
    ("postgis/lwgeom_out_geobuf.c", "!(definedHAVE_LIBPROTOBUF)"): (
        "optional",
        "Geobuf output is unavailable without protobuf-c.",
    ),
    ("postgis/lwgeom_out_mvt.c", "HAVE_LIBPROTOBUF"): (
        "optional",
        "protobuf-c enables MVT output; unavailable paths fail when it is absent.",
    ),
    ("postgis/lwgeom_transform.c", "POSTGIS_PROJ_VERSION>=70100"): (
        "diagnostic",
        "PROJ 7.1 adds network and writable-directory fields to postgis_proj_version().",
    ),
    ("postgis/lwgeom_transform.c", "POSTGIS_PROJ_VERSION>=80100"): (
        "behavior",
        "PROJ 8.1 lets CRS search restrict projected CRS candidates to Earth.",
    ),
    ("postgis/lwgeom_window.c", "POSTGIS_GEOS_VERSION<31200"): (
        "feature",
        "GEOS 3.12 is required for coverage simplify and invalid-edge operations.",
    ),
    ("postgis/lwgeom_window.c", "POSTGIS_GEOS_VERSION<31300"): (
        "feature",
        "GEOS 3.13 is required for ST_ClusterRelateWin.",
    ),
    ("postgis/lwgeom_window.c", "POSTGIS_GEOS_VERSION<31400"): (
        "feature",
        "GEOS 3.14 is required for ST_CoverageClean.",
    ),
    ("postgis/lwgeom_window.c", "POSTGIS_GEOS_VERSION<31500"): (
        "feature",
        "GEOS 3.15 is required for coverage edges and minimum spanning trees.",
    ),
    ("postgis/lwgeom_window.c", "POSTGIS_GEOS_VERSION>=31200"): (
        "feature",
        "GEOS 3.12 is required for coverage window operations.",
    ),
    ("postgis/lwgeom_window.c", "POSTGIS_GEOS_VERSION>=31400"): (
        "feature",
        "GEOS 3.14 adds coverage-clean strategy and ST_CoverageClean support.",
    ),
    ("postgis/mvt.c", "HAVE_LIBPROTOBUF"): (
        "optional",
        "MVT support is compiled only with protobuf-c.",
    ),
    ("postgis/mvt.h", "HAVE_LIBPROTOBUF"): (
        "optional",
        "MVT support is compiled only with protobuf-c.",
    ),
    ("postgis/postgis.sql.in", "POSTGIS_GEOS_VERSION>=31100"): (
        "feature",
        "GEOS 3.11 switches ST_ConcaveHull to the C implementation.",
    ),
    ("postgis/postgis_libprotobuf.c", "HAVE_LIBPROTOBUF"): (
        "optional",
        "PostGIS reports protobuf support and version only when compiled with protobuf-c.",
    ),
    ("postgis/postgis_module.c", "HAVE_LIBPROTOBUF"): (
        "optional",
        "Wagyu interrupt integration is compiled only with protobuf-c/MVT support.",
    ),
    ("postgis/postgis_module.c", "POSTGIS_PROJ_VERSION>60000"): (
        "diagnostic",
        "PROJ 6 and later logs PROJ messages through PostgreSQL.",
    ),
    ("raster/rt_core/librtcore.h", "POSTGIS_GEOS_VERSION>=31400"): (
        "feature",
        "GEOS 3.14 exposes raster intersection-fraction declarations.",
    ),
    ("raster/rt_core/rt_gdal.c", "POSTGIS_GDAL_VERSION>=20400"): (
        "feature",
        "GDAL 2.4 adds polygon contour output; older GDAL only produces contour lines.",
    ),
    (
        "raster/rt_core/rt_raster.c",
        "GDAL_VERSION_NUM>=GDAL_COMPUTE_VERSION(3,14,0)",
    ): (
        "behavior",
        "GDAL 3.14 rasterizes curve line types as their linear counterparts.",
    ),
    ("raster/rt_core/rt_raster.c", "POSTGIS_GDAL_VERSION<30700"): (
        "feature",
        "GDAL before 3.7 lacks native signed 8-bit MEM band mapping.",
    ),
    ("raster/rt_core/rt_raster.c", "POSTGIS_GDAL_VERSION>10800"): (
        "behavior",
        "GDAL 1.9 and later supports half-pixel extent padding for point and line rasterization.",
    ),
    (
        "raster/rt_core/rt_spatial_relationship.c",
        "POSTGIS_GEOS_VERSION>=31400",
    ): (
        "feature",
        "GEOS 3.14 is required for raster intersection fractions.",
    ),
    ("raster/rt_core/rt_util.c", "POSTGIS_GDAL_VERSION>=30700"): (
        "feature",
        "GDAL 3.7 adds Int8 raster datatype mapping.",
    ),
    (
        "raster/rt_core/rt_util.c",
        "POSTGIS_GDAL_VERSION>=31100&&defined(GDT_Float16)",
    ): (
        "feature",
        "GDAL 3.11 adds Float16 raster datatype mapping when the GDAL header exposes it.",
    ),
    ("raster/rt_core/rt_warp.c", "POSTGIS_GDAL_VERSION>=30302"): (
        "behavior",
        "GDAL 3.3.2 changes UNIFIED_SRC_NODATA handling for raster warp.",
    ),
    (
        "raster/rt_pg/rtpg_geometry.c",
        "GDAL_VERSION_NUM>=GDAL_COMPUTE_VERSION(3,14,0)",
    ): (
        "behavior",
        "GDAL 3.14 rasterizes curve line types as their linear counterparts.",
    ),
    ("raster/rt_pg/rtpg_spatial_relationship.c", "POSTGIS_GEOS_VERSION<31400"): (
        "feature",
        "GEOS 3.14 is required for ST_IntersectionFractions.",
    ),
    ("raster/rt_pg/rtpostgis.c", "POSTGIS_GDAL_VERSION<20300"): (
        "compatibility",
        "GDAL before 2.3 uses an older driver registration path.",
    ),
}


class RefreshError(RuntimeError):
    """A source could not be fetched or parsed."""


def release_docs_cgal_contract(text: str) -> dict[str, str]:
    selected = re.search(r"^export\s+CGAL_VER=([^\s#]+)", text, re.MULTILINE)
    minimum = re.search(
        r'^require_minimum_version\s+CGAL\s+"\$\{[^}]+\}"\s+([0-9]+(?:\.[0-9]+)*)\s*$',
        text,
        re.MULTILINE,
    )
    if not selected or not minimum:
        raise RefreshError("release-docs CGAL contract format changed")
    return {
        "selected": selected.group(1),
        "minimum": minimum.group(1),
    }


def shell_assignment_values(text: str, variable: str) -> list[str]:
    pattern = re.compile(
        rf"^\s*(?:export\s+)?{re.escape(variable)}="
        r"(?P<quote>['\"]?)(?P<value>[^'\"\s;#]+)(?P=quote)"
        r"\s*;?\s*(?:#.*)?$",
        re.MULTILINE,
    )
    return list(dict.fromkeys(match.group("value") for match in pattern.finditer(text)))


def winnie_dependency_pins(common_text: str, package_text: str) -> dict[str, dict[str, Any]]:
    variables = {
        "zlib": "ZLIB_VER",
        "lz4": "LZ4_VER",
        "boost": "BOOST_VER",
    }
    pins: dict[str, dict[str, Any]] = {}
    for dependency, variable in variables.items():
        values = shell_assignment_values(common_text, variable)
        if len(values) != 1:
            raise RefreshError(f"Winnie {variable} assignments changed: {values or 'none'}")
        pins[dependency] = {
            "value": values[0],
            "context": "Winnie default",
        }

    boost_path_values = shell_assignment_values(common_text, "BOOST_VER_WU")
    expected_boost_path = pins["boost"]["value"].replace(".", "_")
    if boost_path_values != [expected_boost_path]:
        raise RefreshError(
            f"Winnie BOOST_VER_WU does not match BOOST_VER: {boost_path_values or 'none'} vs {expected_boost_path}"
        )

    package_boost = shell_assignment_values(package_text, "BOOST_VER")
    package_boost_paths = shell_assignment_values(package_text, "BOOST_VER_WU")
    if len(package_boost) > 1 or len(package_boost_paths) > 1:
        raise RefreshError(
            "Winnie package Boost assignments changed: "
            f"BOOST_VER={package_boost or 'none'}, "
            f"BOOST_VER_WU={package_boost_paths or 'none'}"
        )
    if bool(package_boost) != bool(package_boost_paths):
        raise RefreshError("Winnie package Boost version and path override must appear together")
    if package_boost:
        expected_package_path = package_boost[0].replace(".", "_")
        if package_boost_paths != [expected_package_path]:
            raise RefreshError(
                "Winnie package BOOST_VER_WU does not match BOOST_VER: "
                f"{package_boost_paths or 'none'} vs {expected_package_path}"
            )
        pins["boost"]["overrides"] = [
            {
                "value": package_boost[0],
                "context": "SFCGAL package override",
                "source": f"postgis-ci-winnie-package-{SOURCE_DATE}",
            }
        ]
    return pins


def load_json(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def validated_cache(cache: dict[str, Any]) -> dict[str, Any]:
    if cache.get("schema_version") != CACHE_SCHEMA_VERSION:
        raise RefreshError(f"unsupported compatibility cache schema {cache.get('schema_version')!r}")

    generated = cache.get("generated")
    if not isinstance(generated, dict):
        raise RefreshError("compatibility cache has no dependency version metadata")
    for key in URLS:
        if not isinstance(generated.get(key), list) or not generated[key]:
            raise RefreshError(f"compatibility cache has no {key} release lines")

    packaged = cache.get("packaged_versions")
    if not isinstance(packaged, dict):
        raise RefreshError("compatibility cache has no Repology metadata")
    for key in REPOLOGY_PROJECTS:
        if not isinstance(packaged.get(key), dict) or not packaged[key].get("newest"):
            raise RefreshError(f"compatibility cache has no packaged {key} release")

    vendored = cache.get("vendored_inventory")
    if not isinstance(vendored, dict):
        raise RefreshError("compatibility cache has no vendored dependency metadata")
    for key in GITHUB_TAG_PROJECTS:
        if not isinstance(vendored.get(key), dict):
            raise RefreshError(f"compatibility cache has no vendored {key} record")

    if not isinstance(cache.get("source_feature_gates"), list) or not cache["source_feature_gates"]:
        raise RefreshError("compatibility cache has no source feature gates")
    if not isinstance(cache.get("compatibility_sources"), list) or not cache["compatibility_sources"]:
        raise RefreshError("compatibility cache has no source definitions")
    warnings = cache.get("update_warnings")
    if not isinstance(warnings, list) or any(
        not isinstance(warning, dict) or not all(warning.get(key) for key in ("source", "message", "fallback"))
        for warning in warnings
    ):
        raise RefreshError("compatibility cache has malformed update warnings")
    return cache


def fetch_text(url: str) -> str:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            charset = response.headers.get_content_charset() or "utf-8"
            return response.read().decode(charset, "replace")
    except (urllib.error.URLError, TimeoutError) as exc:
        raise RefreshError(f"fetch failed for {url}: {exc}") from exc


def fetch_json(url: str) -> Any:
    try:
        return json.loads(fetch_text(url))
    except json.JSONDecodeError as exc:
        raise RefreshError(f"JSON changed for {url}: {exc}") from exc


def clean_html(text: str) -> str:
    text = re.sub(r"<script\b.*?</script>", " ", text, flags=re.I | re.S)
    text = re.sub(r"<style\b.*?</style>", " ", text, flags=re.I | re.S)
    text = re.sub(r"<[^>]+>", " ", text)
    return re.sub(r"\s+", " ", html.unescape(text)).strip()


def iso_date(value: str) -> str:
    value = value.strip()
    for fmt in ("%B %d, %Y", "%Y/%m/%d", "%Y-%m-%d"):
        try:
            return dt.datetime.strptime(value, fmt).date().isoformat()
        except ValueError:
            pass
    raise RefreshError(f"unrecognized date {value!r}")


def version_key(version: str) -> tuple[int, ...]:
    parts = [int(part) for part in re.findall(r"\d+", version)]
    while len(parts) > 2 and parts[-1] == 0:
        parts.pop()
    return tuple(parts)


def numeric_release_key(version: str) -> tuple[int, ...] | None:
    match = re.fullmatch(r"[vV]?(\d+(?:\.\d+)+)", version.strip())
    if not match:
        return None
    parts = [int(part) for part in match.group(1).split(".")]
    while len(parts) > 2 and parts[-1] == 0:
        parts.pop()
    return tuple(parts)


def major_minor(version: str) -> str:
    parts = re.findall(r"\d+", version)
    if len(parts) < 2:
        raise RefreshError(f"not enough version components in {version!r}")
    return f"{int(parts[0])}.{int(parts[1])}"


def repology_api_url(project: str) -> str:
    return f"https://repology.org/api/v1/project/{project}"


def repology_page_url(project: str) -> str:
    return f"https://repology.org/project/{project}/versions"


def github_tags_url(repo: str) -> str:
    return f"https://api.github.com/repos/{repo}/tags?per_page=100"


def github_tag_page_url(repo: str, tag: str) -> str:
    return f"https://github.com/{repo}/releases/tag/{tag}"


def is_release_like(version: str) -> bool:
    if version.upper() == "HEAD" or version == "9999":
        return False
    return bool(re.match(r"^\d+(?:[._~-]?\d+)+(?:[._~-]?(?:alpha|beta|rc)\d*)?$", version, re.I))


def is_development_version(version: str) -> bool:
    return bool(re.search(r"(?:alpha|beta|rc|dev|HEAD|9999)", version, re.I))


def parse_repology_project(payload: str, key: str, project: str) -> dict[str, Any]:
    try:
        rows = json.loads(payload)
    except json.JSONDecodeError as exc:
        raise RefreshError(f"Repology {project} JSON changed: {exc}") from exc
    if not isinstance(rows, list):
        raise RefreshError(f"Repology {project} response is not a list")

    grouped: dict[str, dict[str, Any]] = {}
    for row in rows:
        if not isinstance(row, dict):
            continue
        version = str(row.get("version") or "").strip()
        status = str(row.get("status") or "").strip()
        repo = str(row.get("repo") or "").strip()
        if not version or not status or status in {"ignored", "untrusted"}:
            continue
        if not repo or not is_release_like(version):
            continue
        bucket = grouped.setdefault(
            version,
            {
                "version": version,
                "status_repos": {},
                "repos": set(),
            },
        )
        bucket["status_repos"].setdefault(status, set()).add(repo)
        bucket["repos"].add(repo)

    merged: dict[tuple[str, Any], dict[str, Any]] = {}
    for item in grouped.values():
        numeric_key = numeric_release_key(item["version"])
        merge_key: tuple[str, Any] = (
            ("numeric", numeric_key) if numeric_key is not None else ("literal", item["version"])
        )
        bucket = merged.get(merge_key)
        if bucket is None:
            merged[merge_key] = item
            continue
        if (len(item["version"]), item["version"]) < (
            len(bucket["version"]),
            bucket["version"],
        ):
            bucket["version"] = item["version"]
        bucket["repos"].update(item["repos"])
        for status, repositories in item["status_repos"].items():
            bucket["status_repos"].setdefault(status, set()).update(repositories)

    versions = list(merged.values())
    if not versions:
        raise RefreshError(f"Repology {project} has no parseable package versions")

    newest = sorted(
        (
            item
            for item in versions
            if item["status_repos"].get("newest") and not is_development_version(item["version"])
        ),
        key=lambda item: version_key(item["version"]),
        reverse=True,
    )
    development = sorted(
        (item for item in versions if is_development_version(item["version"]) and item["version"] != "9999"),
        key=lambda item: version_key(item["version"]),
        reverse=True,
    )

    def compact(item: dict[str, Any]) -> dict[str, Any]:
        return {
            "version": item["version"],
            "status_counts": {
                status: len(repositories) for status, repositories in sorted(item["status_repos"].items())
            },
            "repository_count": len(item["repos"]),
            "repos": sorted(item["repos"])[:8],
        }

    result: dict[str, Any] = {
        "project": project,
        "source": f"repology-{project}-{SOURCE_DATE}",
        "url": repology_page_url(project),
        "newest": [compact(item) for item in newest[:4]],
        "development": [compact(item) for item in development[:4]],
    }
    if not result["newest"]:
        raise RefreshError(f"Repology {project} has no stable newest package version")
    return result


def refresh_repology(cache: dict[str, Any], warnings: list[dict[str, str]]) -> dict[str, Any]:
    cached = cache.get("packaged_versions", {})
    packaged: dict[str, Any] = {}
    for key, project in REPOLOGY_PROJECTS.items():
        try:
            packaged[key] = parse_repology_project(fetch_text(repology_api_url(project)), key, project)
        except RefreshError as exc:
            warnings.append(
                {
                    "source": f"repology:{project}",
                    "message": str(exc),
                    "fallback": "cache",
                }
            )
            if key not in cached:
                raise RefreshError(f"{exc}; no cached Repology data for {project}") from exc
            packaged[key] = cached[key]
    return packaged


def normalize_version(version: str | None) -> str | None:
    if not version:
        return None
    return re.sub(r"^[vV]", "", str(version).strip())


def vendored_dependency_records() -> dict[str, dict[str, str]]:
    path = ROOT / "doc" / "development" / "release" / "dependencies.md"
    if not path.exists():
        return {}
    records: dict[str, dict[str, str]] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|") or line.startswith("|-") or "Dependency" in line:
            continue
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        if len(cells) < 4:
            continue
        key = cells[0].lower()
        if key == "ryū":
            key = "ryu"
        records[key] = {
            "local": cells[1],
            "upstream": re.sub(r"[<>]", "", cells[2]),
            "notes": cells[3],
        }
    return records


def local_vendored_metadata() -> dict[str, dict[str, Any]]:
    records = vendored_dependency_records()
    wagyu = (ROOT / "deps" / "wagyu" / "include" / "mapbox" / "geometry" / "wagyu" / "wagyu.hpp").read_text(
        encoding="utf-8"
    )
    wagyu_parts = {
        match.group(1): match.group(2)
        for match in re.finditer(r"#define\s+WAGYU_(MAJOR|MINOR|PATCH)_VERSION\s+(\d+)", wagyu)
    }
    uthash = (ROOT / "deps" / "uthash" / "include" / "uthash.h").read_text(encoding="utf-8")
    uthash_match = re.search(r"#define\s+UTHASH_VERSION\s+([0-9.]+)", uthash)
    flatbuffers = (ROOT / "deps" / "flatgeobuf" / "include" / "flatbuffers" / "base.h").read_text(encoding="utf-8")
    flatbuffers_parts = {
        match.group(1): match.group(2)
        for match in re.finditer(r"#define\s+FLATBUFFERS_VERSION_(MAJOR|MINOR|REVISION)\s+(\d+)", flatbuffers)
    }
    ryu_record = records.get("ryu", {})
    ryu_version_match = re.search(r"\bv?(\d+(?:\.\d+)+)", ryu_record.get("local", ""))
    flatgeobuf_record = records.get("flatgeobuf", {})
    flatgeobuf_version_match = re.search(r"\b(\d+(?:\.\d+)+)", flatgeobuf_record.get("local", ""))
    return {
        "wagyu": {
            "vendored_version": ".".join(wagyu_parts[key] for key in ("MAJOR", "MINOR", "PATCH")),
            "vendored_label": records.get("wagyu", {}).get("local"),
            "vendored_source": "deps/wagyu/include/mapbox/geometry/wagyu/wagyu.hpp",
            "note": records.get("wagyu", {}).get("notes") or "Used for MVT polygon clipping.",
        },
        "ryu": {
            "vendored_version": normalize_version(ryu_version_match.group(0)) if ryu_version_match else None,
            "vendored_label": ryu_record.get("local") or "PostGIS-local snapshot",
            "vendored_snapshot": ryu_record.get("local") or "PostGIS-local snapshot",
            "vendored_source": "deps/ryu/README.md",
            "note": ryu_record.get("notes") or "Vendored subset with PostGIS-local formatting changes.",
        },
        "flatgeobuf": {
            "vendored_version": normalize_version(flatgeobuf_version_match.group(0))
            if flatgeobuf_version_match
            else None,
            "vendored_label": flatgeobuf_record.get("local") or "FlatGeobuf version not recorded",
            "vendored_snapshot": flatgeobuf_record.get("local") or "FlatGeobuf version not recorded",
            "vendored_source": "deps/flatgeobuf/README.md",
            "embedded_versions": {
                "flatbuffers": ".".join(flatbuffers_parts[key] for key in ("MAJOR", "MINOR", "REVISION")),
            },
            "note": flatgeobuf_record.get("notes") or "Vendored FlatGeobuf and FlatBuffers source.",
            "maintenance_note": (
                "Regenerate the FlatGeobuf and FlatBuffers headers, preserve the "
                "unique FlatBuffers namespace and PostGIS big-endian fixes, then "
                "re-run the portability checks."
            ),
        },
        "uthash": {
            "vendored_version": uthash_match.group(1) if uthash_match else None,
            "vendored_label": records.get("uthash", {}).get("local"),
            "vendored_source": "deps/uthash/include/uthash.h",
            "note": records.get("uthash", {}).get("notes")
            or "Single-header hash table library with PostGIS-local HASH_FUNCTION rename.",
        },
    }


def parse_github_latest_tag(payload: Any, key: str, spec: dict[str, str]) -> dict[str, Any]:
    if not isinstance(payload, list):
        raise RefreshError(f"GitHub tags for {spec['repo']} response is not a list")
    pattern = re.compile(spec["pattern"])
    tags = [item for item in payload if isinstance(item, dict) and pattern.match(str(item.get("name") or ""))]
    if not tags:
        raise RefreshError(f"GitHub tags for {spec['repo']} have no matching release tags")
    tags.sort(key=lambda item: version_key(str(item.get("name"))), reverse=True)
    tag = str(tags[0]["name"])
    return {
        "tag": tag,
        "version": normalize_version(tag),
        "url": github_tag_page_url(spec["repo"], tag),
        "source": f"github-tags-{key}-{SOURCE_DATE}",
    }


def refresh_vendored_inventory(cache: dict[str, Any], warnings: list[dict[str, str]]) -> dict[str, Any]:
    local = local_vendored_metadata()
    cached = cache.get("vendored_inventory", {})
    result: dict[str, Any] = {}
    for key, metadata in local.items():
        item = {
            "id": key,
            **metadata,
        }
        spec = GITHUB_TAG_PROJECTS[key]
        try:
            upstream = parse_github_latest_tag(fetch_json(github_tags_url(spec["repo"])), key, spec)
        except RefreshError as exc:
            warnings.append(
                {
                    "source": f"github-tags:{spec['repo']}",
                    "message": str(exc),
                    "fallback": "cache",
                }
            )
            upstream = (cached.get(key) or {}).get("upstream_latest")
            if not upstream:
                upstream = {
                    "version": None,
                    "tag": None,
                    "url": f"https://github.com/{spec['repo']}",
                    "source": "unavailable",
                }
        item["upstream_latest"] = upstream
        vendored = normalize_version(item.get("vendored_version"))
        latest = normalize_version(upstream.get("version"))
        if vendored and latest:
            delta = version_key(latest) > version_key(vendored)
            item["status"] = "behind" if delta else "current"
        elif item.get("vendored_label") and latest:
            item["status"] = "tracked-unversioned"
        else:
            item["status"] = "unknown"
        result[key] = item
    return result


def parse_postgresql(page: str, existing: list[dict[str, Any]]) -> list[dict[str, Any]]:
    rows = re.findall(
        r"<tr>\s*<td>([^<]+)</td>\s*<td>([^<]+)</td>\s*<td>([^<]+)</td>\s*<td>([^<]+)</td>\s*<td>([^<]+)</td>",
        page,
        flags=re.I,
    )
    if len(rows) < 5:
        raise RefreshError("PostgreSQL version table format changed")
    parsed: list[dict[str, Any]] = []
    for version, current, supported, first, final in rows:
        item: dict[str, Any] = {
            "version": html.unescape(version).strip(),
            "lifecycle": "supported" if supported.strip().lower() == "yes" else "eol",
            "current_minor": html.unescape(current).strip(),
            "first_release_date": iso_date(html.unescape(first)),
            "final_release_date": iso_date(html.unescape(final)),
            "source": f"postgresql-versioning-{SOURCE_DATE}",
        }
        if item["lifecycle"] == "eol":
            item["eol"] = True
        parsed.append(item)
    development = [
        dict(item, source=f"postgresql-versioning-{SOURCE_DATE}")
        for item in existing
        if item.get("lifecycle") == "development"
    ]
    result = development + parsed
    if not any(item["version"] in {"18", "17", "16"} for item in result):
        raise RefreshError("PostgreSQL parser did not find expected supported majors")
    return result


def parse_geos(page: str, existing: list[dict[str, Any]]) -> list[dict[str, Any]]:
    text = clean_html(page)
    rows = re.findall(
        r"\b(3\.\d+\.\d+(?:[a-z]+\d*)?)\s+(\d{4}/\d{2}/\d{2})\s+(\d{4}/\d{2}/\d{2})\s+(\d{4}/\d{2}/\d{2})",
        text,
    )
    if len(rows) < 4:
        raise RefreshError("GEOS release table format changed")
    by_bucket: dict[str, dict[str, Any]] = {}
    today = dt.date.fromisoformat(SOURCE_DATE)
    for release, _release_date, first_date, final_date in rows:
        bucket = major_minor(release)
        stage = "beta" if re.search(r"[a-z]", release) else None
        final_iso = iso_date(final_date)
        final_day = dt.date.fromisoformat(final_iso)
        is_eol = final_day < today
        by_bucket[bucket] = {
            "version": bucket,
            "lifecycle": "development" if stage else ("eol" if is_eol else "supported"),
            **({"eol": True} if is_eol else {}),
            "current_minor": release,
            "first_release_date": iso_date(first_date),
            "final_release_date": final_iso,
            "source": f"geos-download-{SOURCE_DATE}",
            **({"release_stage": stage} if stage else {}),
        }
    for item in existing:
        if item.get("version") not in by_bucket:
            eol_item = dict(item)
            eol_item["lifecycle"] = "eol"
            eol_item["eol"] = True
            eol_item["source"] = f"geos-download-{SOURCE_DATE}"
            by_bucket[eol_item["version"]] = eol_item
    wanted = [item["version"] for item in existing]
    result = [by_bucket[version] for version in wanted if version in by_bucket]
    if "3.15" not in by_bucket or "3.14" not in by_bucket:
        raise RefreshError("GEOS parser did not find 3.15 and 3.14")
    return result


def release_links(page: str, prefix: str) -> list[tuple[str, str]]:
    if prefix == "proj":
        rows = re.findall(
            r'href="proj-([0-9]+\.[0-9]+\.[0-9]+)\.tar[^"]*"[^>]*>.*?<td class="date">(\d{4}-[A-Za-z]{3}-\d{2})',
            page,
            re.S,
        )
        if not rows:
            raise RefreshError("PROJ release links not found")
        deduped: dict[str, str] = {}
        for version, date in rows:
            deduped.setdefault(version, dt.datetime.strptime(date, "%Y-%b-%d").date().isoformat())
        return list(deduped.items())
    if prefix == "gdal":
        rows: list[tuple[str, str]] = re.findall(
            r'href="([0-9]+\.[0-9]+\.[0-9]+)/"[^>]*>.*?<td class="date">(\d{4}-[A-Za-z]{3}-\d{2})',
            page,
            re.S,
        )
        parsed = [(version, dt.datetime.strptime(date, "%Y-%b-%d").date().isoformat()) for version, date in rows]
        for date, version in re.findall(
            r"<strong>(\d{4}-\d{2}(?:-\d{2})?)</strong>.*?gdal-([0-9]+\.[0-9]+\.[0-9]+)\.tar",
            page,
            re.S,
        ):
            parsed.append((version, date if len(date) == 10 else f"{date}-01"))
        if not parsed:
            raise RefreshError("GDAL release links not found")
        deduped: dict[str, str] = {}
        for version, date in parsed:
            deduped.setdefault(version, date)
        return list(deduped.items())
    rows = re.findall(
        r"<strong>(\d{4}-\d{2}-\d{2})</strong>.*?" + re.escape(prefix) + r"-([0-9.]+)\.tar",
        page,
        re.S,
    )
    if not rows:
        raise RefreshError(f"{prefix.upper()} release links not found")
    return [(version, date) for date, version in rows]


def minor_line(version: str) -> str:
    major, minor, *_ = version_key(version)
    return f"{major}.{minor}"


def release_line_versions(
    page: str,
    prefix: str,
    existing: list[dict[str, Any]] | None = None,
) -> list[dict[str, Any]]:
    grouped: dict[str, list[tuple[str, str]]] = {}
    existing_by_version = {item.get("version"): item for item in existing or []}
    for version, date in release_links(page, prefix):
        line = minor_line(version)
        grouped.setdefault(line, []).append((version, date))
    if not grouped:
        raise RefreshError(f"{prefix.upper()} release lines not found")
    result: list[dict[str, Any]] = []
    ordered = sorted(grouped, key=version_key, reverse=True)
    for index, line in enumerate(ordered):
        releases = grouped[line]
        latest = max(releases, key=lambda row: version_key(row[0]))
        first = min(iso_date(date) for _version, date in releases)
        previous = existing_by_version.get(line, {})
        result.append(
            {
                "version": line,
                "lifecycle": "current" if index == 0 else previous.get("lifecycle", "historical"),
                **({"eol": True} if previous.get("eol") else {}),
                "current_minor": latest[0],
                "first_release_date": first,
                "source": f"{prefix.lower()}-download-{SOURCE_DATE}",
                **({"note": previous["note"]} if previous.get("note") else {}),
            }
        )
    return result


def parse_gdal(
    current_page: str,
    past_page: str,
    existing: list[dict[str, Any]] | None = None,
) -> list[dict[str, Any]]:
    # Require the current page to parse independently so a changed page cannot
    # silently leave the newest line pinned to the archive's initial release.
    release_links(current_page, "gdal")
    return release_line_versions(f"{current_page}\n{past_page}", "gdal", existing)


def parse_sfcgal(payload: str, existing: list[dict[str, Any]] | None = None) -> list[dict[str, Any]]:
    try:
        releases = json.loads(payload)
    except json.JSONDecodeError as exc:
        raise RefreshError(f"SFCGAL releases API JSON changed: {exc}") from exc
    grouped: dict[str, list[tuple[str, str]]] = {}
    existing_by_version = {item.get("version"): item for item in existing or []}
    for release in releases:
        tag = str(release.get("tag_name") or "").lstrip("v")
        if not re.match(r"^\d+\.\d+\.\d+", tag):
            continue
        released_at = str(release.get("released_at") or release.get("created_at") or "")[:10]
        if not released_at:
            continue
        grouped.setdefault(minor_line(tag), []).append((tag, released_at))
    if not grouped:
        raise RefreshError("SFCGAL release lines not found")
    result: list[dict[str, Any]] = []
    for index, line in enumerate(sorted(grouped, key=version_key, reverse=True)):
        releases = grouped[line]
        latest = max(releases, key=lambda row: version_key(row[0]))
        first = min(date for _version, date in releases)
        previous = existing_by_version.get(line, {})
        result.append(
            {
                "version": line,
                "lifecycle": "current" if index == 0 else previous.get("lifecycle", "historical"),
                **({"eol": True} if previous.get("eol") else {}),
                "current_minor": latest[0],
                "first_release_date": first,
                "source": f"sfcgal-gitlab-releases-{SOURCE_DATE}",
                **({"note": previous["note"]} if previous.get("note") else {}),
            }
        )
    return result


def configure_text(ref: str) -> str:
    paths = ["configure.ac", "configure.in"]
    if ref == "HEAD":
        for path in paths:
            source = ROOT / path
            if source.exists():
                return source.read_text(encoding="utf-8")
        raise RefreshError("cannot find configure.ac or configure.in at HEAD")
    errors = []
    for path in paths:
        result = subprocess.run(
            ["git", "show", f"{ref}:{path}"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if result.returncode == 0:
            return result.stdout
        errors.append(result.stderr.strip())
    raise RefreshError(f"cannot read configure.ac or configure.in at {ref}: {'; '.join(errors)}")


def configure_path_for_ref(ref: str) -> str:
    paths = ["configure.ac", "configure.in"]
    if ref == "HEAD":
        for path in paths:
            if (ROOT / path).exists():
                return path
    for path in paths:
        result = subprocess.run(
            ["git", "cat-file", "-e", f"{ref}:{path}"],
            cwd=ROOT,
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if result.returncode == 0:
            return path
    raise RefreshError(f"cannot find configure.ac or configure.in at {ref}")


def configure_minimums_for_ref(ref: str) -> dict[str, str]:
    text = configure_text(ref)
    patterns = {
        "geos": r"(?:GEOS_MIN_VERSION=|PostGIS requires GEOS >= )([0-9.]+)",
        "gdal": r"GDAL_MIN_VERSION=([0-9.]+)",
        "sfcgal": r"PostGIS requires SFCGAL >= ([0-9.]+)",
        "protobuf-c": r"libprotobuf-c >= ([0-9.]+)",
    }
    found: dict[str, str] = {}
    for key, pattern in patterns.items():
        match = re.search(pattern, text)
        if match:
            found[key] = match.group(1)
    proj = re.search(r"PostGIS requires PROJ >= ([0-9.]+)", text)
    if proj:
        found["proj"] = proj.group(1)
    elif "USE_PROJ" in text and "--with-proj" in text:
        found["proj"] = "4.5.0"
    return found


def pg_configure_number_to_version(value: int) -> str:
    if value >= 100 and value % 10 == 0:
        return str(value // 10)
    return f"{value // 10}.{value % 10}"


def previous_pg_configure_version(value: int) -> str:
    if value >= 100 and value % 10 == 0:
        return str(value // 10 - 1)
    major = value // 10
    minor = value % 10
    if minor > 0:
        return f"{major}.{minor - 1}"
    return str(major - 1)


def postgresql_range_from_configure_text(text: str) -> str | None:
    lower_values: list[int] = []
    upper_matches: list[tuple[str, int]] = []
    lines = text.splitlines()
    for index, line in enumerate(lines):
        match = re.search(r"POSTGIS_PGSQL_VERSION\s+-(lt|gt|ge)\s+(\d+)", line)
        if not match:
            continue
        block = "\n".join(lines[index : index + 4])
        if "AC_MSG_ERROR" not in block or "PostGIS requires PostgreSQL" not in block:
            continue
        operator, value_text = match.groups()
        value = int(value_text)
        if operator == "lt":
            lower_values.append(value)
        else:
            upper_matches.append((operator, value))
    if not lower_values:
        return None
    lower = max(lower_values)
    upper_versions = []
    for operator, value in upper_matches:
        if operator == "gt":
            upper_versions.append(pg_configure_number_to_version(value))
        else:
            upper_versions.append(previous_pg_configure_version(value))
    if not upper_versions:
        return None
    upper = min(upper_versions, key=version_key)
    return f"{pg_configure_number_to_version(lower)}-{upper}"


def postgresql_minimum_from_configure_text(text: str) -> str | None:
    lower_values = [int(value) for value in re.findall(r"POSTGIS_PGSQL_VERSION\s+-lt\s+(\d+)", text)]
    if not lower_values:
        return None
    return pg_configure_number_to_version(max(lower_values))


def source_gate_dependency(expression: str) -> str | None:
    for macro, dependency in SOURCE_GATE_MACROS.items():
        if macro in expression:
            return dependency
    return None


def source_gate_threshold(expression: str) -> str | None:
    match = re.search(r"POSTGIS_PGSQL_VERSION\s*(?:>=|>|<|<=)\s*(\d+)", expression)
    if match:
        value = int(match.group(1))
        return f"{value // 10}.{value % 10}"
    match = re.search(r"POSTGIS_(?:GEOS|PROJ|GDAL|SFCGAL)_VERSION\s*(?:>=|>|<|<=)\s*(\d+)", expression)
    if match:
        value = match.group(1)
        if len(value) >= 5:
            major = int(value[:-4])
            minor = int(value[-4:-2])
            patch = int(value[-2:])
            return f"{major}.{minor}.{patch}"
        if len(value) >= 3:
            major = int(value[:-2])
            minor = int(value[-2:])
            return f"{major}.{minor}.0"
    match = re.search(r"GDAL_COMPUTE_VERSION\((\d+),\s*(\d+),\s*(\d+)\)", expression)
    if match:
        return ".".join(match.groups())
    match = re.search(r"JSON_C_VERSION_(\d)(\d{2})", expression)
    if match:
        return f"{int(match.group(1))}.{int(match.group(2))}"
    if expression.strip() == "JSON_C_VERSION":
        return "0.10"
    if "HAVE_LIBJSON" in expression:
        return "optional"
    if "HAVE_LIBPROTOBUF" in expression:
        return "optional"
    if "HAVE_ICONV" in expression or "HAVE_ICONVCTL" in expression:
        return "optional"
    if "HAVE_SFCGAL" in expression:
        return "optional"
    if "PROJ_GEODESIC" in expression:
        return "4.9.0"
    return None


def source_gate_classification_key(rel: str, expression: str) -> tuple[str, str]:
    return rel, re.sub(r"\s+", "", expression)


def classify_source_gate(rel: str, expression: str) -> tuple[str, str] | None:
    normalized = source_gate_classification_key(rel, expression)[1]
    explicit = SOURCE_GATE_CLASSIFICATION.get((rel, normalized))
    if explicit:
        return explicit
    dependency = source_gate_dependency(normalized)
    if dependency == "postgresql":
        if ">=140" in normalized:
            return (
                "feature",
                "PostgreSQL 14 adds planner support function index pseudoconstant API arguments.",
            )
        if ">=150" in normalized:
            return ("feature", "PostgreSQL 15 enables GiST sort support declarations.")
        if ">150" in normalized:
            return (
                "compatibility",
                "PostgreSQL 16 and later uses the varatt.h include path for detoast helpers.",
            )
        if ">=160" in normalized:
            if "flatgeobuf" in rel:
                return (
                    "compatibility",
                    "PostgreSQL 16 changes datetime decode error reporting arguments.",
                )
            return (
                "compatibility",
                "PostgreSQL 16 changes memory-context and GUC helper APIs.",
            )
        if ">=170" in normalized:
            return (
                "compatibility",
                "PostgreSQL 17 changes ANALYZE statistics target access.",
            )
        if ">=180" in normalized:
            return (
                "compatibility",
                "PostgreSQL 18 changes vacuum_delay_point interrupt arguments.",
            )
        if "<190" in normalized or ">=190" in normalized:
            return (
                "compatibility",
                "PostgreSQL 19 changes composite JSON and detoast/header access APIs.",
            )
        if "<=130" in normalized:
            return (
                "compatibility",
                "PostgreSQL 13 and older need non-string hash handling for GeoJSON duplicate-property detection.",
            )
    if dependency == "sfcgal" and rel.startswith("sfcgal/"):
        if ">=10400" in normalized:
            return (
                "diagnostic",
                "SFCGAL 1.4 adds runtime full-version metadata and guarded SQL declarations.",
            )
        if "<10401" in normalized:
            return ("feature", "SFCGAL 1.4.1 is required for alpha-shape functions.")
        if "<10500" in normalized:
            return (
                "feature",
                "SFCGAL 1.5 is required for partition and visibility functions.",
            )
        if "<20000" in normalized:
            return (
                "feature",
                "SFCGAL 2.0 is required for newer CGAL-backed 3D and partition functions.",
            )
        if "<20100" in normalized:
            return (
                "compatibility",
                "SFCGAL 2.1 changes patch/geometry collection APIs.",
            )
        if "<20200" in normalized:
            return (
                "feature",
                "SFCGAL 2.2 is required for newer straight-skeleton and medial-axis options.",
            )
        if "<20300" in normalized or ">=20300" in normalized:
            if "SFCGAL_CGAL_VERSION_MAJOR" in normalized:
                return (
                    "feature",
                    "SFCGAL 2.3 plus CGAL 6 is required for polygon repair.",
                )
            return (
                "feature",
                "SFCGAL 2.3 is required for NURBSCURVE and projected medial-axis support.",
            )
    return None


def iter_source_gate_lines() -> list[dict[str, Any]]:
    pattern = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif)\s+(.*)")
    gates: list[dict[str, Any]] = []
    for base in SOURCE_GATE_PATHS:
        root = ROOT / base
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in {".c", ".h", ".in"}:
                continue
            if any(part in {"deps", "test", "cunit"} for part in path.relative_to(ROOT).parts):
                continue
            try:
                lines = path.read_text(encoding="utf-8").splitlines()
            except UnicodeDecodeError:
                continue
            index = 0
            while index < len(lines):
                lineno = index + 1
                logical_line = lines[index]
                while logical_line.rstrip().endswith("\\") and index + 1 < len(lines):
                    logical_line = logical_line.rstrip()[:-1] + " " + lines[index + 1].lstrip()
                    index += 1
                index += 1
                match = pattern.match(logical_line)
                if not match:
                    continue
                expression = match.group(2).strip()
                dependency = source_gate_dependency(expression)
                if not dependency:
                    continue
                rel = str(path.relative_to(ROOT))
                classification = classify_source_gate(rel, expression)
                gates.append(
                    {
                        "dependency": dependency,
                        "path": rel,
                        "line": lineno,
                        "expression": expression,
                        "threshold": source_gate_threshold(expression),
                        "class": classification[0] if classification else "unclassified",
                        "note": classification[1]
                        if classification
                        else "Unclassified dependency compile-time gate; classify before publishing.",
                        "source": f"postgis-source-gates-{SOURCE_DATE}",
                    }
                )
    return gates


def scan_source_feature_gates() -> list[dict[str, Any]]:
    gates = iter_source_gate_lines()
    unclassified = [gate for gate in gates if gate["class"] == "unclassified"]
    if unclassified:
        examples = ", ".join(f"{gate['path']}:{gate['line']}" for gate in unclassified[:8])
        raise RefreshError(f"unclassified dependency source gates: {examples}")
    observed = {source_gate_classification_key(gate["path"], gate["expression"]) for gate in gates}
    stale = set(SOURCE_GATE_CLASSIFICATION) - observed
    if stale:
        examples = ", ".join(f"{path}:{expression}" for path, expression in sorted(stale)[:8])
        raise RefreshError(f"stale dependency source-gate classifications: {examples}")
    return gates


def newest_release_tag_for_minor(minor: str) -> str | None:
    result = subprocess.run(
        ["git", "tag", "-l", f"{minor}.*"],
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        raise RefreshError(f"cannot list release tags for {minor}: {result.stderr.strip()}")
    tags = [tag.strip() for tag in result.stdout.splitlines() if tag.strip()]
    stable = [tag for tag in tags if re.match(rf"^{re.escape(minor)}\.\d+$", tag)]
    candidates = stable or [tag for tag in tags if re.match(rf"^{re.escape(minor)}\.\d+(?:alpha|beta|rc)\d*$", tag)]
    if not candidates:
        return None
    return max(candidates, key=version_key)


def refresh_series_dependencies(
    series: list[dict[str, Any]], postgresql_versions: list[dict[str, Any]]
) -> list[dict[str, Any]]:
    refreshed = json.loads(json.dumps(series))
    pg_lines = [item.get("version") for item in postgresql_versions if item.get("version")]
    latest_pg_line = max(pg_lines, key=version_key) if pg_lines else None
    for item in refreshed:
        latest = item.get("latest")
        if not latest:
            continue
        if item.get("branch") == "master":
            ref = "HEAD"
        elif latest.endswith(".x"):
            ref = newest_release_tag_for_minor(item.get("minor", "")) or latest
            if ref != latest:
                item["latest"] = ref
        else:
            ref = latest
        try:
            configure = configure_text(ref)
            configure_path = configure_path_for_ref(ref)
            minimums = configure_minimums_for_ref(ref)
        except RefreshError:
            continue
        pg_range = postgresql_range_from_configure_text(configure)
        if pg_range:
            postgresql = item.setdefault("postgresql", {})
            postgresql["supported"] = pg_range
            postgresql["source"] = f"tag:{ref}:{configure_path}"
            if postgresql.get("default_summary", "").startswith(("PostgreSQL ", "historical PostgreSQL ")):
                prefix = (
                    "historical PostgreSQL " if item.get("eol") or item.get("lifecycle") == "eol" else "PostgreSQL "
                )
                postgresql["default_summary"] = f"{prefix}{pg_range}"
        elif item.get("branch") == "master" and latest_pg_line:
            pg_minimum = postgresql_minimum_from_configure_text(configure)
            if pg_minimum:
                postgresql = item.setdefault("postgresql", {})
                postgresql["supported"] = f"{pg_minimum}-{latest_pg_line}"
                postgresql["source"] = f"tag:{ref}:{configure_path}"
                postgresql["default_summary"] = f"PostgreSQL {pg_minimum}-{latest_pg_line}"
        deps = item.setdefault("dependencies", {})
        for key in ("geos", "proj", "gdal", "sfcgal", "protobuf-c"):
            if key in minimums:
                deps.setdefault(key, {})["minimum"] = minimums[key]
                deps[key]["source"] = f"tag:{ref}:{configure_path}"
    return refreshed


def refresh_dependency_catalog(
    catalog: list[dict[str, Any]],
    current_series: dict[str, Any],
    warnings: list[dict[str, str]],
) -> list[dict[str, Any]]:
    current_dependencies = current_series.get("dependencies", {})
    current = {key: value.get("minimum") for key, value in current_dependencies.items() if value.get("minimum")}
    sources = {key: value.get("source") for key, value in current_dependencies.items() if value.get("source")}
    pg_supported = str(current_series.get("postgresql", {}).get("supported") or "")
    if pg_supported:
        current["postgresql"] = pg_supported.split("-", 1)[0]
    configure = configure_text("HEAD")
    configure_minimums = configure_minimums_for_ref("HEAD")
    current.update(configure_minimums)
    sources.update({key: f"postgis-configure-ac-{SOURCE_DATE}" for key in configure_minimums})
    pg_minimum = postgresql_minimum_from_configure_text(configure)
    if pg_minimum:
        current["postgresql"] = pg_minimum
        sources["postgresql"] = f"postgis-configure-ac-{SOURCE_DATE}"
    mapping = {
        "postgresql-server-dev": "postgresql",
        "geos": "geos",
        "proj": "proj",
        "gdal": "gdal",
        "sfcgal": "sfcgal",
        "protobuf-c": "protobuf-c",
        "json-c": "json-c",
        "libxml2": "libxml2",
    }
    refreshed = json.loads(json.dumps(catalog))
    for item in refreshed:
        key = mapping.get(item.get("id"))
        if key and key in current:
            item["minimum"] = current[key]
            if sources.get(key):
                item["source"] = sources[key]
    try:
        winnie_pins = winnie_dependency_pins(
            (ROOT / "ci" / "winnie" / "winnie_common.sh").read_text(encoding="utf-8"),
            (ROOT / "ci" / "winnie" / "package_postgis.sh").read_text(encoding="utf-8"),
        )
    except (OSError, RefreshError) as exc:
        warnings.append(
            {
                "source": "winnie-dependencies",
                "message": str(exc),
                "fallback": "cached dependency catalog",
            }
        )
    else:
        for item in refreshed:
            pin = winnie_pins.get(str(item.get("id") or ""))
            if not pin:
                continue
            item["minimum"] = None
            item["pin"] = {
                **pin,
                "source": f"postgis-ci-winnie-deps-{SOURCE_DATE}",
                "maintenance_note": (
                    "Verify or build the matching Windows dependency bundle before changing the CI selection."
                ),
            }
            item["source"] = f"postgis-ci-winnie-deps-{SOURCE_DATE}"
    try:
        cgal_contract = release_docs_cgal_contract(
            (ROOT / "ci" / "debbie" / "postgis_release_docs.sh").read_text(encoding="utf-8")
        )
    except (OSError, RefreshError) as exc:
        warnings.append(
            {
                "source": "release-docs-ci",
                "message": str(exc),
                "fallback": "cached dependency catalog",
            }
        )
    else:
        cgal = next((item for item in refreshed if item.get("id") == "cgal"), None)
        if cgal:
            cgal["requirement"] = "transitive through SFCGAL"
            cgal["minimum"] = None
            cgal["release_docs_ci"] = {
                **cgal_contract,
                "source": f"postgis-release-docs-ci-{SOURCE_DATE}",
            }
            cgal["source"] = f"postgis-release-docs-ci-{SOURCE_DATE}"
    return refreshed


def source_gate_summary(gates: list[dict[str, Any]], dependency: str, classes: set[str]) -> str | None:
    selected = [gate for gate in gates if gate.get("dependency") == dependency and gate.get("class") in classes]
    if not selected:
        return None
    thresholds = sorted(
        {gate.get("threshold") for gate in selected if gate.get("threshold") and gate.get("threshold") != "optional"},
        key=version_key,
    )
    class_counts: dict[str, int] = {}
    for gate in selected:
        class_counts[gate["class"]] = class_counts.get(gate["class"], 0) + 1
    pieces = []
    if thresholds:
        pieces.append("thresholds " + ", ".join(thresholds))
    pieces.extend(f"{name} {count}" for name, count in sorted(class_counts.items()))
    return "; ".join(pieces)


def resolve_compatibility_edge_overlaps(data: dict[str, Any]) -> None:
    for _dependency, edges in data.get("compatibility_edges", {}).items():
        priority = {
            "ci": 4,
            "supported": 4,
            "feature-limited": 3,
            "known-compatible": 2,
            "historical": 2,
            "unsupported": 1,
        }
        best: dict[tuple[str, str], int] = {}
        for edge in edges:
            rank = priority.get(edge.get("status"), 0)
            for version in edge.get("versions", []):
                key = (edge.get("postgis", ""), version)
                best[key] = max(best.get(key, 0), rank)
        for edge in edges:
            rank = priority.get(edge.get("status"), 0)
            edge["versions"] = [
                version
                for version in edge.get("versions", [])
                if best.get((edge.get("postgis", ""), version), rank) <= rank
            ]
        edges[:] = [edge for edge in edges if edge.get("versions") or edge.get("status") == "not-applicable"]


def ensure_supported_dependency_edges(data: dict[str, Any]) -> None:
    dependency_keys = {"geos", "proj", "gdal", "sfcgal"}
    version_meta = {
        key: values for key, values in data.get("dependency_versions", {}).items() if key in dependency_keys
    }
    if not version_meta:
        return
    edge_statuses: dict[tuple[str, str], set[str]] = {}
    for dependency, edges in data.get("compatibility_edges", {}).items():
        for edge in edges:
            edge_statuses.setdefault((dependency, edge.get("postgis", "")), set()).add(edge.get("status", ""))
    for series in data.get("postgis_series", []):
        if series.get("eol") or series.get("lifecycle") == "eol":
            continue
        for dependency, meta_items in version_meta.items():
            dependency_info = (series.get("dependencies") or {}).get(dependency) or {}
            minimum = dependency_info.get("minimum")
            if not minimum or "supported" in edge_statuses.get((dependency, series.get("minor", "")), set()):
                continue
            threshold = dependency_info.get("feature_complete") or minimum
            candidate = next(
                (
                    item["version"]
                    for item in meta_items
                    if version_key(item["version"]) >= version_key(threshold)
                    and not (item.get("lifecycle") == "development" and series.get("lifecycle") != "development")
                ),
                None,
            )
            if not candidate:
                continue
            data.setdefault("compatibility_edges", {}).setdefault(dependency, []).append(
                {
                    "postgis": series["minor"],
                    "versions": [candidate],
                    "status": "supported",
                    "note": f"Latest dependency line satisfying the PostGIS {series['minor']} recorded {dependency} minimum.",
                    "source": dependency_info.get("source") or f"postgis-release-configure-ac-{SOURCE_DATE}",
                }
            )
            edge_statuses.setdefault((dependency, series["minor"]), set()).add("supported")


def enrich_series_with_source_gates(series: list[dict[str, Any]], gates: list[dict[str, Any]]) -> list[dict[str, Any]]:
    enriched = json.loads(json.dumps(series))
    feature_summaries = {
        "postgresql": "PostgreSQL 15 and later has the full PostGIS feature set recorded by current source gates: PostgreSQL 14 adds planner support-function API arguments, and PostgreSQL 15 enables GiST sort support declarations. PostgreSQL 16, 17, 18, and 19 gates are compatibility shims for PostgreSQL API changes, not PostGIS feature degradations on PostgreSQL 15.",
        "geos": "GEOS gates include 3.11 directed line merge/triangulate/concave-hull paths, 3.12 coverage operations, 3.13 prepared relate/cluster paths, 3.14 coverage-clean and raster intersection fractions, and 3.15 split/coverage-edge/minimum-spanning-tree functions.",
        "proj": "PROJ gates include GeographicLib geodesic behavior, PROJ 7.1 proj-version network diagnostics, and PROJ 8.1 CRS search filtering by celestial body.",
        "gdal": "GDAL gates include 2.4 polygon contour output, 3.3.2 raster warp nodata behavior, 3.7 signed Int8 raster datatype mapping, 3.11 Float16 mapping, and 3.14 curve rasterization behavior.",
        "sfcgal": "SFCGAL gates include 1.3.8 measured-coordinate support, 1.5 XYM/XYZM point construction, 2.1 API renames, and 2.3 NURBSCURVE support.",
        "json-c": "JSON-C is optional; GeoJSON input paths error out when PostGIS is compiled without it.",
        "protobuf-c": "protobuf-c is optional; MVT and Geobuf output paths error out when PostGIS is compiled without it.",
        "iconv": "iconvctl is optional and enables stricter shp2pgsql character-conversion behavior when available.",
    }
    feature_complete = {
        "geos": "3.15.0",
        "proj": "8.1.0",
        "sfcgal": "2.3.0",
    }
    for item in enriched:
        if item.get("branch") != "master":
            continue
        pg = item.setdefault("postgresql", {})
        pg["feature_note"] = feature_summaries["postgresql"]
        pg["feature_source"] = f"postgis-source-gates-{SOURCE_DATE}"
        pg_summary = source_gate_summary(
            gates,
            "postgresql",
            {"feature", "behavior", "performance", "optional", "compatibility"},
        )
        if pg_summary:
            pg["source_gate_summary"] = pg_summary
        deps = item.setdefault("dependencies", {})
        for dependency, note in feature_summaries.items():
            if dependency == "postgresql":
                continue
            if dependency not in deps:
                continue
            if dependency in feature_complete and item.get("branch") == "master":
                deps[dependency].setdefault("feature_complete", feature_complete[dependency])
            deps[dependency]["feature_note"] = note
            deps[dependency]["feature_source"] = f"postgis-source-gates-{SOURCE_DATE}"
            summary = source_gate_summary(
                gates,
                dependency,
                {"feature", "behavior", "performance", "optional", "compatibility"},
            )
            if summary:
                deps[dependency]["source_gate_summary"] = summary
    return enriched


def source_definitions() -> list[dict[str, str]]:
    sources = [
        {
            "id": f"postgresql-versioning-{SOURCE_DATE}",
            "url": URLS["postgresql"],
            "title": "PostgreSQL versioning policy and release support table",
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"postgis-configure-ac-{SOURCE_DATE}",
            "title": "PostGIS configure.ac dependency checks",
            "url": "https://gitea.osgeo.org/postgis/postgis/src/branch/master/configure.ac",
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"geos-download-{SOURCE_DATE}",
            "title": "GEOS download and EOL table",
            "url": URLS["geos"],
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"proj-download-{SOURCE_DATE}",
            "title": "PROJ download page",
            "url": URLS["proj"],
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"gdal-download-{SOURCE_DATE}",
            "title": "GDAL current and past releases",
            "url": URLS["gdal"],
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"sfcgal-gitlab-releases-{SOURCE_DATE}",
            "title": "SFCGAL releases",
            "url": "https://gitlab.com/sfcgal/SFCGAL/-/releases",
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"postgis-ci-winnie-deps-{SOURCE_DATE}",
            "title": "PostGIS Winnie packaging dependency versions",
            "url": "https://gitea.osgeo.org/postgis/postgis/src/branch/master/ci/winnie/winnie_common.sh",
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"postgis-ci-winnie-package-{SOURCE_DATE}",
            "title": "PostGIS Winnie package assembly script",
            "url": "https://gitea.osgeo.org/postgis/postgis/src/branch/master/ci/winnie/package_postgis.sh",
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"postgis-release-docs-ci-{SOURCE_DATE}",
            "title": "PostGIS release documentation CI dependency contract",
            "url": "https://gitea.osgeo.org/postgis/postgis/src/branch/master/ci/debbie/postgis_release_docs.sh",
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"postgis-release-configure-ac-{SOURCE_DATE}",
            "title": "PostGIS release configure.ac dependency checks",
            "url": "https://gitea.osgeo.org/postgis/postgis/tags",
            "as_of": SOURCE_DATE,
        },
        {
            "id": f"postgis-source-gates-{SOURCE_DATE}",
            "title": "PostGIS source compile-time dependency gates",
            "url": "https://gitea.osgeo.org/postgis/postgis/src/branch/master",
            "as_of": SOURCE_DATE,
        },
    ]
    sources.extend(
        {
            "id": f"repology-{project}-{SOURCE_DATE}",
            "title": f"Repology package metadata for {project}",
            "url": repology_page_url(project),
            "as_of": SOURCE_DATE,
        }
        for project in REPOLOGY_PROJECTS.values()
    )
    sources.extend(
        {
            "id": f"github-tags-{key}-{SOURCE_DATE}",
            "title": f"GitHub tags for vendored {key}",
            "url": f"https://github.com/{spec['repo']}/tags",
            "as_of": SOURCE_DATE,
        }
        for key, spec in GITHUB_TAG_PROJECTS.items()
    )
    return sources


def generated_source_definitions(sources: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [
        json.loads(json.dumps(item))
        for item in sources
        if str(item.get("id", "")).startswith(GENERATED_SOURCE_PREFIXES)
    ]


def cache_from_matrix(data: dict[str, Any]) -> dict[str, Any]:
    return validated_cache(
        {
            "schema_version": CACHE_SCHEMA_VERSION,
            "generated_at": dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds"),
            "generated": json.loads(json.dumps(data.get("dependency_versions", {}))),
            "packaged_versions": json.loads(json.dumps(data.get("packaged_versions", {}))),
            "vendored_inventory": json.loads(json.dumps(data.get("vendored_inventory", {}))),
            "source_feature_gates": json.loads(json.dumps(data.get("source_feature_gates", []))),
            "compatibility_sources": generated_source_definitions(data.get("compatibility_sources", [])),
            "update_warnings": json.loads(json.dumps(data.get("update_warnings", []))),
        }
    )


def use_cached(cache: dict[str, Any], key: str, warning: str) -> Any:
    generated = cache.get("generated", {})
    if key not in generated:
        raise RefreshError(f"{warning}; no cached {key} available")
    return generated[key]


def apply_cache(matrix: dict[str, Any], cache: dict[str, Any]) -> dict[str, Any]:
    cache = validated_cache(cache)
    generated = cache.get("generated")
    generated_sources = cache.get("compatibility_sources")

    updated = json.loads(json.dumps(matrix))
    warnings = json.loads(json.dumps(cache.get("update_warnings", [])))
    updated["dependency_versions"] = json.loads(json.dumps(generated))
    updated["packaged_versions"] = json.loads(json.dumps(cache.get("packaged_versions", {})))
    updated["vendored_inventory"] = json.loads(json.dumps(cache.get("vendored_inventory", {})))
    source_feature_gates = json.loads(json.dumps(cache.get("source_feature_gates", [])))
    updated["source_feature_gates"] = source_feature_gates
    updated["compatibility_sources"] = [
        item
        for item in updated.get("compatibility_sources", [])
        if not str(item.get("id", "")).startswith(GENERATED_SOURCE_PREFIXES)
    ] + json.loads(json.dumps(generated_sources))

    ensure_supported_dependency_edges(updated)
    resolve_compatibility_edge_overlaps(updated)
    updated["postgis_series"] = enrich_series_with_source_gates(
        refresh_series_dependencies(
            updated.get("postgis_series", []),
            generated.get("postgresql", []),
        ),
        source_feature_gates,
    )
    updated["dependency_catalog"] = refresh_dependency_catalog(
        updated.get("dependency_catalog", []),
        updated.get("postgis_series", [{}])[0],
        warnings,
    )
    updated["update_warnings"] = warnings
    return updated


def build_update(matrix: dict[str, Any], cache: dict[str, Any]) -> dict[str, Any]:
    cache = validated_cache(cache)
    warnings: list[dict[str, str]] = []
    generated: dict[str, Any] = {}
    fetch_urls = {**URLS, **AUXILIARY_URLS}
    fetches = {key: None for key in fetch_urls}
    for key, url in fetch_urls.items():
        try:
            fetches[key] = fetch_text(url)
        except RefreshError as exc:
            warnings.append({"source": key, "message": str(exc), "fallback": "cache"})

    existing_versions = cache.get("generated", {})
    parsers = {
        "postgresql": (
            ("postgresql",),
            lambda pages: parse_postgresql(pages[0], existing_versions.get("postgresql", [])),
        ),
        "geos": (
            ("geos",),
            lambda pages: parse_geos(pages[0], existing_versions.get("geos", [])),
        ),
        "proj": (
            ("proj",),
            lambda pages: release_line_versions(pages[0], "proj", existing_versions.get("proj", [])),
        ),
        "gdal": (
            ("gdal", "gdal-past"),
            lambda pages: parse_gdal(pages[0], pages[1], existing_versions.get("gdal", [])),
        ),
        "sfcgal": (
            ("sfcgal",),
            lambda pages: parse_sfcgal(pages[0], existing_versions.get("sfcgal", [])),
        ),
    }
    for key, (source_keys, parser) in parsers.items():
        if any(fetches[source] is None for source in source_keys):
            generated[key] = use_cached(cache, key, f"{key} fetch failed")
            continue
        try:
            generated[key] = parser([fetches[source] or "" for source in source_keys])
        except RefreshError as exc:
            warnings.append({"source": key, "message": str(exc), "fallback": "cache"})
            generated[key] = use_cached(cache, key, str(exc))

    packaged_versions = refresh_repology(cache, warnings)
    vendored_inventory = refresh_vendored_inventory(cache, warnings)
    try:
        source_feature_gates = scan_source_feature_gates()
    except RefreshError as exc:
        warnings.append({"source": "source-gates", "message": str(exc), "fallback": "cache"})
        source_feature_gates = cache.get("source_feature_gates", [])
        if not source_feature_gates:
            raise RefreshError(f"{exc}; no cached source feature gates available")

    next_cache = {
        "schema_version": CACHE_SCHEMA_VERSION,
        "generated": generated,
        "packaged_versions": packaged_versions,
        "vendored_inventory": vendored_inventory,
        "source_feature_gates": source_feature_gates,
        "compatibility_sources": source_definitions(),
        "update_warnings": warnings,
    }
    return apply_cache(matrix, next_cache)
