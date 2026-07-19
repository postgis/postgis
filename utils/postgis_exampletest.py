#!/usr/bin/env python3

import argparse
import base64
from concurrent.futures import ThreadPoolExecutor, as_completed
import html
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from xml_tree import parse as parse_xml


DOCBOOK_NS = "http://docbook.org/ns/docbook"
XML_NS = "http://www.w3.org/XML/1998/namespace"
FORCE_ROLE = "example-test"
EXTERNAL_STATE_ROLE = "requires-external-state"
DOCUMENTED_OUTPUT_ROLE = "documented-output"
CAPABILITY_ROLE_RE = re.compile(
    r"^requires-(geos|proj|sfcgal|cgal|protobuf|raster)-(\d+(?:\.\d+)*)$", re.I
)
GEOMETRY_OUTPUT_PRECISION_ROLE_RE = re.compile(r"^geometry-output-precision-(\d+)$")
CAPABILITY_VERSION_QUERIES = {
    "geos": "SELECT PostGIS_GEOS_Version()",
    "proj": "SELECT PostGIS_PROJ_Version()",
    "sfcgal": "SELECT postgis_sfcgal_version()",
    "cgal": "SELECT postgis_sfcgal_full_version()",
    "protobuf": "SELECT postgis_libprotobuf_version()",
    "raster": (
        "SELECT CASE WHEN EXISTS ("
        "SELECT 1 FROM pg_proc WHERE proname = 'st_makeemptyraster'"
        ") THEN '1.0.0' ELSE '' END"
    ),
}
FUNCTION_CAPABILITY_REQUIREMENTS = (
    ("raster", (1, 0, 0), re.compile(
        r"\b(?:ST_(?:AddBand|AsPNG|AsRaster|Band|Clip|ColorMap|ConvexHull|"
        r"FromGDALRaster|GDALDrivers|Grayscale|MapAlgebra|MakeEmptyRaster|"
        r"MinConvexHull|Pixel|Polygon|Raster|Resample|Rescale|Resize|"
        r"SetBand|SetM|SetValue|SetValues|SetZ|Transform|Value)|PostGIS_Raster_)",
        re.I,
    )),
    ("geos", (3, 11, 0), re.compile(
        r"\bST_(?:DelaunayTriangles|LargestEmptyCircle|LineMerge|"
        r"MaximumInscribedCircle|OffsetCurve|OrientedEnvelope|"
        r"SimplifyPolygonHull|SimplifyPreserveTopology|TriangulatePolygon|"
        r"VoronoiPolygons)\s*\(",
        re.I,
    )),
    ("geos", (3, 12, 0), re.compile(r"\bST_Coverage(?:InvalidEdges|Simplify)\s*\(", re.I)),
    ("geos", (3, 13, 0), re.compile(r"\bST_ClusterRelateWin\s*\(", re.I)),
    ("geos", (3, 14, 0), re.compile(r"\bST_CoverageClean\s*\(", re.I)),
    ("geos", (3, 15, 0), re.compile(r"\bST_CoverageEdges\s*\(", re.I)),
    ("sfcgal", (2, 0, 0), re.compile(r"\bCG_3DBuffer\s*\(", re.I)),
    ("sfcgal", (2, 1, 0), re.compile(
        r"\bCG_(?:3DAlphaWrapping|3DDifference|3DUnion|AlphaShape|"
        r"ApproximateMedialAxis|ConstrainedDelaunayTriangles|"
        r"ExtrudeStraightSkeleton|OptimalAlphaShape|Simplify|"
        r"StraightSkeletonPartition|Volume)\s*\(",
        re.I,
    )),
    ("sfcgal", (2, 3, 0), re.compile(
        r"\bCG_(?:Generate(?:Flat|Hipped|Gable|Skillion)?Roof|PolygonRepair|"
        r"NurbsCurve(?:Interpolate|Approximate|Derivative))\s*\(",
        re.I,
    )),
    ("cgal", (6, 0, 0), re.compile(r"\bCG_PolygonRepair\s*\(", re.I)),
)
ENVIRONMENT_CHECKS = (
    {
        "label": "PROJ grid au_icsm_GDA94_GDA2020_conformal_and_distortion.tif",
        "query": """SELECT ST_AsText(ST_TransformPipeline(
    'SRID=4939;POINT(143.0 -37.0)'::geometry,
    'urn:ogc:def:coordinateOperation:EPSG::8447'
))""",
        "expected": [["POINT(143.0000063280214 -36.999986718287545)"]],
        "hint": "Install the PROJ data package containing au_icsm_GDA94_GDA2020_conformal_and_distortion.tif, or fetch it with projsync so the PostgreSQL/PostGIS PROJ search path can find it.",
    },
)


class QueryRows(list):
    def __init__(self, rows, headers=None, types=None, visuals=None):
        super().__init__(rows)
        self.headers = headers or []
        self.types = types or []
        self.visuals = visuals or []
WKT_TYPES = (
    "POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|"
    "GEOMETRYCOLLECTION|CIRCULARSTRING|COMPOUNDCURVE|CURVEPOLYGON|"
    "MULTICURVE|MULTISURFACE|TIN|TRIANGLE|POLYHEDRALSURFACE"
    "|NURBSCURVE"
)
WKT_DIMENSION_SUFFIX = r"(?:\s*(?:ZM|Z|M)|ZM|Z|M)?"
WKT_START_RE = re.compile(rf"(?:SRID\s*=\s*\d+\s*;\s*)?(?:{WKT_TYPES}){WKT_DIMENSION_SUFFIX}\s*\(", re.I)
WKT_VALUE_RE = re.compile(
    rf"^\s*(?:SRID\s*=\s*\d+\s*;\s*)?(?:{WKT_TYPES}){WKT_DIMENSION_SUFFIX}\s*(?:\(|EMPTY\s*$)",
    re.I,
)
WKT_COMPONENT_RE = re.compile(
    rf"\b({WKT_TYPES}){WKT_DIMENSION_SUFFIX}\s*(?=\(|EMPTY\b)",
    re.I,
)
HEXWKB_RE = re.compile(r"^(?:\\x)?([0-9A-Fa-f]+)$")
GEOMETRY_OUTPUT_TYPE_RE = re.compile(r"^(?:geometry|geography)(?:\s*\([^)]*\))?$", re.I)
WKB_TYPE_NAMES = {
    1: "POINT",
    2: "LINESTRING",
    3: "POLYGON",
    4: "MULTIPOINT",
    5: "MULTILINESTRING",
    6: "MULTIPOLYGON",
    7: "GEOMETRYCOLLECTION",
    8: "CIRCULARSTRING",
    9: "COMPOUNDCURVE",
    10: "CURVEPOLYGON",
    11: "MULTICURVE",
    12: "MULTISURFACE",
    15: "POLYHEDRALSURFACE",
    16: "TIN",
    17: "TRIANGLE",
    21: "NURBSCURVE",
}
UNORDERED_AREAL_TYPES = ("POLYGON", "MULTIPOLYGON", "TRIANGLE")
UNORDERED_SURFACE_TYPES = ("POLYHEDRALSURFACE", "TIN")
EWKB_Z_FLAG = 0x80000000
EWKB_M_FLAG = 0x40000000
EWKB_SRID_FLAG = 0x20000000
EWKB_BBOX_FLAG = 0x10000000
EWKB_TYPE_MASK = 0x0FFFFFFF
NUMBER_PATTERN = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
MAKE_ENVELOPE_RE = re.compile(
    rf"\bST_MakeEnvelope\s*\(\s*({NUMBER_PATTERN})\s*,\s*({NUMBER_PATTERN})\s*,\s*"
    rf"({NUMBER_PATTERN})\s*,\s*({NUMBER_PATTERN})(?:\s*,\s*(-?\d+))?\s*\)",
    re.I,
)
POINT_CONSTRUCTOR_RE = re.compile(
    rf"\bST_(?:Make)?Point\s*\(\s*({NUMBER_PATTERN})\s*,\s*({NUMBER_PATTERN})\s*\)",
    re.I,
)
GEOGRAPHY_INPUT_RE = re.compile(
    r"::\s*geography\b|\bST_(?:Geog|Geography)FromText\s*\(",
    re.I,
)
VISUAL_ROLE = "visual-primary"
VISUAL_SKIP_ROLE = "visual-skip"
VISUAL_DIRECTION_ROLE = "visual-direction"
VISUAL_OUTPUT_ONLY_ROLE = "visual-output-only"
VISUAL_HIDE_OUTPUT_AREA_VERTICES_ROLE = "visual-hide-output-area-vertices"
VISUAL_SEPARATE_OUTPUT_ROLE = "visual-separate-output"
VISUAL_ROW_PANELS_ROLE = "visual-row-panels"
VISUAL_MATRIX_3X3_ROLE = "visual-matrix-3x3"
VISUAL_OVERLAY_ROLE = "visual-overlay"
SVG_PALETTES = {
    "Code": ("#2878b8", "#59a4d8", "#0f5f9c"),
    "Output": ("#a62c2b", "#d95f3d", "#ef8a47"),
}
POINT_TYPES = {"POINT", "MULTIPOINT"}
CLUSTER_COLUMN_NAMES = {"cluster", "cluster_id", "cid", "category"}
LABEL_COLUMN_NAMES = {"label", "name", "title", "description", "place", "places", "parcels"}
MARKER_SCALE_COLUMN_NAME = "marker_scale"
MAX_VISUAL_GEOMETRIES = 16
MAX_VISUAL_WKT_BYTES = 100000
VALUE_DECIMAL_DIGITS = 12
VALUE_TOLERANCE = 10 ** -VALUE_DECIMAL_DIGITS
NONVISUAL_SINGLE_INPUT_RE = re.compile(
    r"\b(?:ST_(?:AS(?:TEXT|EWKT|BINARY|EWKB|HEXEWKB|GML|KML|GEOJSON|X3D|"
    r"MARC21|GEOBUF|TWKB|ENCODEDPOLYLINE|LATLONTEXT)|GEOHASH|SRID|GEOMETRYTYPE|"
    r"COORDDIM|NDIMS|ZMFLAG|MEMSIZE|NUMGEOMETRIES|NUMPOINTS|NPOINTS|NUMRINGS|"
    r"NUMPATCHES|X|Y|Z|M)|CG_3DAREA)\s*\(",
    re.I,
)
DECIMAL_RE = re.compile(r"(?<![A-Za-z_])-?(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?")
VERSION_FUNCTION_RE = re.compile(
    r"\b(?:"
    r"PostGIS_Full_Version|postgis_full_version|PostGIS_GEOS_Version|"
    r"PostGIS_GEOS_Compiled_Version|PostGIS_LibXML_Version|"
    r"PostGIS_LibJSON_Version|PostGIS_PROJ_Version|"
    r"PostGIS_PROJ_Compiled_Version|PostGIS_Wagyu_Version|"
    r"PostGIS_Lib_Build_Date|PostGIS_Lib_Version|"
    r"PostGIS_Scripts_Build_Date|PostGIS_Scripts_Installed|"
    r"PostGIS_Scripts_Released|PostGIS_Version"
    r")\s*\(",
    re.I,
)
CATALOG_QUERY_RE = re.compile(r"\bpg_available_extensions\b", re.I)


class ExampleTester:
    def __init__(self, xml_file):
        self.index = parse_xml(xml_file)
        self.doc = self.index.tree

    def node_text(self, node):
        return "".join(node.itertext()).replace("\r\n", "\n").replace("\r", "\n")

    def has_role(self, node, role):
        return role in node.get("role", "").split()

    def geometry_output_precision(self, node):
        if node is None:
            return None
        for role in node.get("role", "").split():
            match = GEOMETRY_OUTPUT_PRECISION_ROLE_RE.fullmatch(role)
            if match:
                return int(match.group(1))
        return None

    def capability_requirements(self, node):
        requirements = []
        for role in node.get("role", "").split():
            match = CAPABILITY_ROLE_RE.fullmatch(role)
            if match:
                requirements.append({
                    "name": match.group(1).lower(),
                    "minimum": tuple(int(part) for part in match.group(2).split(".")),
                    "role": role,
                })
        return requirements

    def inferred_capability_requirements(self, query):
        requirements = []
        if query is None:
            return requirements
        for name, minimum, pattern in FUNCTION_CAPABILITY_REQUIREMENTS:
            if pattern.search(query):
                requirements.append({
                    "name": name,
                    "minimum": minimum,
                    "role": f"auto-requires-{name}-" + ".".join(str(part) for part in minimum),
                })
        return requirements

    def merge_capability_requirements(self, requirements):
        merged = {}
        for requirement in requirements:
            name = requirement["name"]
            current = merged.get(name)
            if current is None or requirement["minimum"] > current["minimum"]:
                merged[name] = requirement
        return list(merged.values())

    def runtime_capabilities(self, database, names):
        capabilities = {}
        for name in sorted(names):
            query = CAPABILITY_VERSION_QUERIES[name]
            try:
                value = self.run_psql_scalar(database, query)
            except RuntimeError:
                capabilities[name] = ()
                continue
            capabilities[name] = self.capability_version(name, value)
        return capabilities

    def capability_version(self, name, value):
        if name == "cgal":
            match = re.search(r"\bCGAL\b[^0-9]*(\d+(?:\.\d+)+)", value, re.I)
        else:
            match = re.search(r"\d+(?:\.\d+)+", value)
        if not match:
            return ()
        version_text = match.group(1 if name == "cgal" else 0)
        return tuple(int(part) for part in version_text.split("."))

    def requirements_satisfied(self, requirements, capabilities):
        for requirement in requirements:
            current = capabilities.get(requirement["name"], ())
            minimum = requirement["minimum"]
            width = max(len(current), len(minimum))
            if current + (0,) * (width - len(current)) < minimum + (0,) * (width - len(minimum)):
                return False
        return True

    def source_label(self, node):
        refentry = next(
            (ancestor for ancestor in self.index.ancestors(node) if ancestor.tag == f"{{{DOCBOOK_NS}}}refentry"),
            None,
        )
        section = next(
            (
                ancestor for ancestor in self.index.ancestors(node)
                if ancestor.tag == f"{{{DOCBOOK_NS}}}section"
            ),
            None,
        )
        line = self.index.line(node) or "unknown"
        refentry_id = refentry.get(f"{{{XML_NS}}}id") if refentry is not None else None
        section_id = section.get(f"{{{XML_NS}}}id") if section is not None else None
        if section is not None and not section_id:
            title = section.find(f"{{{DOCBOOK_NS}}}title")
            if title is not None:
                section_id = re.sub(r"[^A-Za-z0-9_.-]+", "_", self.node_text(title).strip()).strip("_")
        label = refentry_id or section_id or f"line_{line}"
        return f"{label}:{line}"

    def obvious_skip_reason(self, text):
        if re.search(r"SELECT\s+'<\?xml", text, re.I):
            return "document-output"
        if re.search(
            r"\b(sometable|gps_points|boston_polys|trajectories|fe_edges|"
            r"mytable|myrasters|mytestraster|dummy_rast|Projected\.tif|/home/raster|"
            r"towns|va2005|bc_roads|o_4_boston|"
            r"bc_municipality|nyc_|ma\.|features\.|aerials\.|"
            r"ma_topo|topo_boston_test|city_data|tt|"
            r"tiger\.|loader_|raster2pgsql|shp2pgsql|psql\b|wget\b|curl\b|"
            r"write_file|topology\.topologysummary|topology\.createtopology|"
            r"topology\.st_inittopogeo)\b",
            text,
            re.I,
        ):
            return "external-context"
        if re.search(
            r"\b(?:DropGeometryColumn|DropGeometryTable|Find_SRID|"
            r"PostGIS_Extensions_Upgrade|"
            r"PostGIS_GDAL_Version|PostGIS_Liblwgeom_Version|"
            r"PostGIS_Raster_Lib_Build_Date|PostGIS_Raster_Lib_Version|"
            r"PostGIS_Raster_Scripts_Installed|ST_GDALDrivers|ST_FromGDALRaster"
            r")\s*\(",
            text,
            re.I,
        ):
            return "external-context"
        if (
            "..." in text
            or re.search(r"\.\.(?:[),]|\s*$)", text, re.M)
            or re.search(r"(?:AD INFINITUM|--ETC--)", text, re.I)
        ):
            return "placeholder-output"
        if re.search(r"\b(NOTICE|ERROR):", text):
            return "notice-or-error"
        return ""

    def looks_sql(self, text):
        return re.search(r"\b(SELECT|WITH|CREATE|INSERT|UPDATE|DELETE|ALTER|DROP)\b", text, re.I)

    def looks_query(self, text):
        return re.search(r"\b(SELECT|WITH)\b", text, re.I)

    def has_legacy_inband_expected(self, text):
        return (
            re.search(r"--\s*(result|output|wkt collect)\s*--", text, re.I)
            or re.search(r"^\s*\S.*\n\s*[-+]{3,}\s*\n", text, re.S | re.M)
            or re.search(r"\(\d+ rows?\)", text)
        )

    def following_screen(self, node):
        sibling = self.index.next_sibling(node)
        if sibling is not None and sibling.tag == f"{{{DOCBOOK_NS}}}screen":
            return sibling
        return None

    def visual_location(self, screen):
        refentry = next(
            (ancestor for ancestor in self.index.ancestors(screen)
             if ancestor.tag == f"{{{DOCBOOK_NS}}}refentry"),
            None,
        )
        if refentry is None:
            screens = [
                candidate for candidate in self.doc.getroot().iter(f"{{{DOCBOOK_NS}}}screen")
                if not any(
                    ancestor.tag == f"{{{DOCBOOK_NS}}}refentry"
                    for ancestor in self.index.ancestors(candidate)
                )
            ]
            return "", screens.index(screen) + 1
        screens = list(refentry.iter(f"{{{DOCBOOK_NS}}}screen"))
        return refentry.get(f"{{{XML_NS}}}id", ""), screens.index(screen) + 1

    def visual_id(self, screen):
        explicit_id = screen.get(f"{{{XML_NS}}}id")
        if explicit_id:
            return explicit_id
        refentry, ordinal = self.visual_location(screen)
        slug = re.sub(r"[^a-z0-9]+", "-", refentry.lower()).strip("-") or "manual"
        return f"visual-{slug}-{ordinal:02d}"

    def geometry_type(self, value):
        match = WKT_START_RE.search(value or "")
        if not match:
            return None
        prefix = re.sub(r"^\s*SRID\s*=\s*\d+\s*;\s*", "", value, flags=re.I)
        match = re.match(r"([A-Z]+)", prefix.strip(), re.I)
        return match.group(1).upper() if match else None

    def hexwkb_geometry(self, value, allow_bytea_prefix=True):
        """Return the normalized HEXWKB payload and root type, without parsing coordinates."""
        if not isinstance(value, str):
            return None
        text = value.strip()
        match = HEXWKB_RE.fullmatch(text)
        if not match or (text.startswith("\\x") and not allow_bytea_prefix):
            return None
        payload = match.group(1)
        if len(payload) < 10 or len(payload) % 2:
            return None
        header = bytes.fromhex(payload[:10])
        byte_order = header[0]
        if byte_order not in (0, 1):
            return None
        type_word = int.from_bytes(header[1:5], "little" if byte_order == 1 else "big")
        if type_word & EWKB_BBOX_FLAG:
            return None
        encoded_type = type_word & EWKB_TYPE_MASK
        dimension_offset = encoded_type // 1000
        if dimension_offset in (1, 2, 3):
            base_type = encoded_type % 1000
        else:
            base_type = encoded_type
        geometry_type = WKB_TYPE_NAMES.get(base_type)
        if geometry_type is None:
            return None
        has_z = bool(type_word & EWKB_Z_FLAG) or dimension_offset in (1, 3)
        has_m = bool(type_word & EWKB_M_FLAG) or dimension_offset in (2, 3)
        if dimension_offset in (1, 2, 3) and type_word & (EWKB_Z_FLAG | EWKB_M_FLAG):
            return None
        if dimension_offset in (1, 2, 3) and type_word & EWKB_SRID_FLAG and base_type != 21:
            return None
        if base_type == 21 and type_word & (EWKB_Z_FLAG | EWKB_M_FLAG):
            return None
        dimensions = 2 + int(has_z) + int(has_m)
        minimum_bytes = 5 + (4 if type_word & EWKB_SRID_FLAG else 0)
        if base_type == 1:
            minimum_bytes += dimensions * 8
        elif base_type == 21:
            minimum_bytes += 12
        else:
            minimum_bytes += 4
        if len(payload) // 2 < minimum_bytes:
            return None
        return {"hexwkb": payload.upper(), "type": geometry_type}

    def geometry_output_hex(self, value, output_type):
        if not GEOMETRY_OUTPUT_TYPE_RE.fullmatch(output_type or ""):
            return None
        return self.hexwkb_geometry(value)

    def documented_wkt_decimal_digits(self, expected_wkt):
        if expected_wkt is None:
            return VALUE_DECIMAL_DIGITS
        digits = []
        for match in re.finditer(NUMBER_PATTERN, expected_wkt):
            value = match.group(0).lower()
            if "e" in value:
                return VALUE_DECIMAL_DIGITS
            if "." in value:
                digits.append(len(value.split(".", 1)[1]))
        return min(max(digits, default=0), VALUE_DECIMAL_DIGITS)

    def documented_wkt_tolerance(self, expected_wkt):
        digits = self.documented_wkt_decimal_digits(expected_wkt)
        return max(VALUE_TOLERANCE, 0.5 * 10 ** -digits)

    def documented_wkt_uses_explicit_z_token(self, expected_wkt):
        return bool(re.search(
            rf"\b(?:{WKT_TYPES})\s+Z(?:M)?\s*(?:\(|EMPTY\b)",
            expected_wkt or "",
            re.I,
        ))

    def documented_wkt_is_2d(self, expected_wkt):
        if expected_wkt is None or self.documented_wkt_uses_explicit_z_token(expected_wkt):
            return False
        return not re.search(
            rf"\(\s*{NUMBER_PATTERN}\s+{NUMBER_PATTERN}\s+{NUMBER_PATTERN}",
            expected_wkt,
        )

    def areal_canonical_key_sql(self, geometry, digits=VALUE_DECIMAL_DIGITS):
        """Return a representation-independent key for areal geometry.

        ST_Normalize canonicalizes polygon orientation and component order, but
        its ring-start choice is based on XY.  A 3D face may therefore retain
        different (but equivalent) cyclic starts when two vertices share XY
        and differ only in Z.  Rotate each normalized ring using rounded point
        coordinate keys, then sort the normalized areal components.
        """
        tolerance = format(VALUE_TOLERANCE, ".17g")
        point_key = (
            "concat_ws(' ', "
            f"round((CASE WHEN abs(ST_X(point.geom)) < {tolerance} THEN 0 ELSE ST_X(point.geom) END)::numeric, {digits}), "
            f"round((CASE WHEN abs(ST_Y(point.geom)) < {tolerance} THEN 0 ELSE ST_Y(point.geom) END)::numeric, {digits}), "
            f"round((CASE WHEN abs(ST_Z(point.geom)) < {tolerance} THEN 0 ELSE ST_Z(point.geom) END)::numeric, {digits}), "
            f"round((CASE WHEN abs(ST_M(point.geom)) < {tolerance} THEN 0 ELSE ST_M(point.geom) END)::numeric, {digits}))"
        )
        return (
            "(SELECT jsonb_agg(face_key ORDER BY face_key) FROM (SELECT "
            "face_path, jsonb_agg(ring_key ORDER BY ring_path) AS face_key "
            "FROM (SELECT face_path, ring_path, (SELECT "
            "array_to_string("
            "points[rotation:array_length(points, 1)] || points[1:rotation - 1], "
            "','"
            ") "
            "FROM generate_subscripts(points, 1) AS rotations(rotation) "
            "ORDER BY 1 LIMIT 1) AS ring_key FROM (SELECT face_path, ring_path, "
            "array_agg(point_key ORDER BY point_ordinal) AS points FROM (SELECT "
            "face.path AS face_path, "
            "point.path[1:array_length(point.path, 1) - 1] AS ring_path, "
            "point.path[array_length(point.path, 1)] AS point_ordinal, "
            "max(point.path[array_length(point.path, 1)]) OVER (PARTITION BY "
            "face.path, point.path[1:array_length(point.path, 1) - 1]) AS "
            f"closing_ordinal, {point_key} AS point_key "
            f"FROM canonical CROSS JOIN LATERAL ST_Dump({geometry}) AS face "
            "CROSS JOIN LATERAL ST_DumpPoints(ST_Normalize(face.geom)) AS point"
            ") AS face_points WHERE point_ordinal < closing_ordinal "
            "GROUP BY face_path, ring_path) AS face_rings) AS canonical_rings "
            "GROUP BY face_path) AS canonical_faces)"
        )

    def geometry_comparison_query(
        self, actual_hex, expected_hex=None, expected_wkt=None, actual_type=None,
        expected_wkt_digits=None,
    ):
        actual = (
            "ST_GeomFromEWKB(decode("
            f"'{actual_hex}', 'hex'))"
        )
        if expected_hex is not None:
            expected = (
                "ST_GeomFromEWKB(decode("
                f"'{expected_hex}', 'hex'))"
            )
            srid_is_optional = "FALSE"
        else:
            expected_literal = expected_wkt.replace("'", "''")
            expected = f"ST_GeomFromEWKT('{expected_literal}')"
            srid_is_optional = (
                "FALSE" if re.match(r"^\s*SRID\s*=", expected_wkt, re.I) else "TRUE"
            )
        if expected_hex is None and self.documented_wkt_is_2d(expected_wkt):
            actual = f"ST_Force2D({actual})"
            expected = f"ST_Force2D({expected})"
        preamble = (
            "WITH geometries AS (SELECT "
            f"{actual} AS actual, {expected} AS expected), "
            "comparable AS (SELECT actual, CASE WHEN "
            f"{srid_is_optional} AND ST_SRID(expected) = 0 "
            "THEN ST_SetSRID(expected, ST_SRID(actual)) ELSE expected END AS expected "
            "FROM geometries), "
            "canonical AS (SELECT "
            f"CASE WHEN GeometryType(actual) IN {UNORDERED_AREAL_TYPES!r} "
            "THEN ST_Normalize(actual) ELSE actual END AS actual, "
            f"CASE WHEN GeometryType(expected) IN {UNORDERED_AREAL_TYPES!r} "
            "THEN ST_Normalize(expected) ELSE expected END AS expected "
            "FROM comparable) "
        )
        metadata = (
            "GeometryType(actual) = GeometryType(expected) "
            "AND ST_CoordDim(actual) = ST_CoordDim(expected) "
            "AND ST_Zmflag(actual) = ST_Zmflag(expected) "
            "AND ST_SRID(actual) = ST_SRID(expected) "
        )
        documented_digits = (
            expected_wkt_digits
            if expected_wkt_digits is not None else
            self.documented_wkt_decimal_digits(expected_wkt)
            if expected_hex is None else VALUE_DECIMAL_DIGITS
        )

        if actual_type in UNORDERED_AREAL_TYPES + UNORDERED_SURFACE_TYPES:
            return preamble + (
                "SELECT " + metadata +
                "AND " + self.areal_canonical_key_sql("actual", documented_digits) +
                " IS NOT DISTINCT FROM " +
                self.areal_canonical_key_sql("expected", documented_digits) + " "
                "FROM canonical"
            )

        if expected_hex is None and (
            expected_wkt_digits is not None or 0 < documented_digits < VALUE_DECIMAL_DIGITS
        ):
            shape_match = (
                f"ST_AsText(actual, {documented_digits}) = "
                f"ST_AsText(expected, {documented_digits})"
            )
            if expected_wkt_digits is not None:
                shape_match = (
                    f"({shape_match} OR "
                    f"ST_HausdorffDistance(actual, expected) <= {10 ** -expected_wkt_digits:.17g})"
                )
            return preamble + (
                "SELECT " + metadata +
                f"AND {shape_match} "
                "FROM comparable"
            )

        if actual_type == "NURBSCURVE":
            return preamble + (
                "SELECT " + metadata +
                "AND ST_AsHEXEWKB(actual, 'XDR') = "
                "ST_AsHEXEWKB(expected, 'XDR') "
                "FROM canonical"
            )

        if actual_type == "GEOMETRYCOLLECTION":
            surface_types = "('POLYHEDRALSURFACE', 'TIN')"
            return preamble + (
                "SELECT " + metadata +
                "AND CASE WHEN EXISTS (SELECT 1 FROM ST_Dump(actual) AS part "
                f"WHERE GeometryType(part.geom) IN {surface_types}) "
                "OR EXISTS (SELECT 1 FROM ST_Dump(expected) AS part "
                f"WHERE GeometryType(part.geom) IN {surface_types}) "
                "THEN ST_AsEWKT(actual) = ST_AsEWKT(expected) "
                "ELSE ST_AsEWKT(ST_Normalize(actual)) = "
                "ST_AsEWKT(ST_Normalize(expected)) END "
                "FROM canonical"
            )

        tolerance = format(
            self.documented_wkt_tolerance(expected_wkt)
            if expected_hex is None else VALUE_TOLERANCE,
            ".17g",
        )
        return preamble + (
            ", actual_components AS (SELECT path, GeometryType(geom) AS type "
            "FROM canonical CROSS JOIN LATERAL ST_Dump(actual)), "
            "expected_components AS (SELECT path, GeometryType(geom) AS type "
            "FROM canonical CROSS JOIN LATERAL ST_Dump(expected)), "
            "actual_points AS (SELECT path, ST_X(geom) AS x, ST_Y(geom) AS y, "
            "ST_Z(geom) AS z, ST_M(geom) AS m "
            "FROM canonical CROSS JOIN LATERAL ST_DumpPoints(actual)), "
            "expected_points AS (SELECT path, ST_X(geom) AS x, ST_Y(geom) AS y, "
            "ST_Z(geom) AS z, ST_M(geom) AS m "
            "FROM canonical CROSS JOIN LATERAL ST_DumpPoints(expected)) "
            "SELECT " + metadata +
            "AND NOT EXISTS (SELECT 1 FROM actual_components a FULL JOIN "
            "expected_components e USING (path) WHERE a.path IS NULL OR e.path IS NULL "
            "OR a.type IS DISTINCT FROM e.type) "
            "AND NOT EXISTS (SELECT 1 FROM actual_points a FULL JOIN expected_points e "
            "USING (path) WHERE a.path IS NULL OR e.path IS NULL "
            "OR (a.x IS NULL) <> (e.x IS NULL) "
            f"OR (a.x IS NOT NULL AND abs(a.x - e.x) > {tolerance} * "
            "greatest(1.0, abs(a.x), abs(e.x))) "
            "OR (a.y IS NULL) <> (e.y IS NULL) "
            f"OR (a.y IS NOT NULL AND abs(a.y - e.y) > {tolerance} * "
            "greatest(1.0, abs(a.y), abs(e.y))) "
            "OR (a.z IS NULL) <> (e.z IS NULL) "
            f"OR (a.z IS NOT NULL AND abs(a.z - e.z) > {tolerance} * "
            "greatest(1.0, abs(a.z), abs(e.z))) "
            "OR (a.m IS NULL) <> (e.m IS NULL) "
            f"OR (a.m IS NOT NULL AND abs(a.m - e.m) > {tolerance} * "
            "greatest(1.0, abs(a.m), abs(e.m)))) "
            "FROM canonical"
        )

    def expected_has_hexwkb(self, expected):
        return any(
            self.hexwkb_geometry(value, allow_bytea_prefix=False) is not None
            for row in expected
            for value in row
        )

    def expected_has_geometry_text(self, expected):
        return any(
            isinstance(value, str) and WKT_VALUE_RE.match(value)
            for row in expected
            for value in row
        )

    def documented_visual_types(self, expected):
        width = max((len(row) for row in expected), default=0)
        return [
            "geometry" if any(
                column < len(row)
                and self.hexwkb_geometry(row[column], allow_bytea_prefix=False) is not None
                for row in expected
            ) else ""
            for column in range(width)
        ]

    def normalized_geometry(self, value):
        return re.sub(r"\s+", "", value or "").upper()

    def point_only_geometry(self, value):
        geometry_types = [match.group(1).upper() for match in WKT_COMPONENT_RE.finditer(value or "")]
        return bool(geometry_types) and all(
            geometry_type in POINT_TYPES or geometry_type == "GEOMETRYCOLLECTION"
            for geometry_type in geometry_types
        )

    def visual_candidate(self, query, expected, explicit=False):
        code = [candidate["wkt"] for candidate in self.code_geometry_candidates(query)]
        output_wkt = self.geometry_candidates(self.rows_to_string(expected))
        output_hex = []
        if not NONVISUAL_SINGLE_INPUT_RE.search(query):
            for row in expected:
                for value in row:
                    candidate = self.hexwkb_geometry(value, allow_bytea_prefix=False)
                    if candidate is not None:
                        output_hex.append(candidate)
        if explicit:
            return {"kind": "explicit", "preferred": True}
        if code and not output_wkt and not output_hex and GEOGRAPHY_INPUT_RE.search(query):
            return None
        output_values = output_wkt + [candidate["hexwkb"] for candidate in output_hex]
        values = code + output_values
        if not values:
            return None
        if len(values) > MAX_VISUAL_GEOMETRIES or sum(len(value) for value in values) > MAX_VISUAL_WKT_BYTES:
            return None
        code_types = [self.geometry_type(value) for value in code]
        output_types = [self.geometry_type(value) for value in output_wkt]
        output_types.extend(candidate["type"] for candidate in output_hex)
        if (
            len(code) == len(output_wkt) == 1
            and not output_hex
            and self.normalized_geometry(code[0]) == self.normalized_geometry(output_wkt[0])
        ):
            return None
        if output_values:
            if all(value in POINT_TYPES for value in output_types) and not (
                len(values) > 1 and any(value not in POINT_TYPES for value in code_types)
            ):
                return None
            return {"kind": "geometry-output", "preferred": True}
        if not code or all(self.point_only_geometry(value) for value in code):
            return None
        if len(code) == 1 and NONVISUAL_SINGLE_INPUT_RE.search(query):
            return None
        return {"kind": "input-context", "preferred": False}

    def parse_adjacent_example(self, query, screen):
        return self.clean_example(query, self.expected_rows_from_psql_lines(screen.split("\n")))

    def clean_example(self, query, expected_rows):
        query = query.strip()
        query = re.sub(r";\s*$", "", query)
        if not query or not expected_rows:
            return None, None
        return query, expected_rows

    def expected_rows_from_plain_lines(self, lines):
        rows = []
        for line in lines:
            line = line.strip()
            if not line or re.match(r"^\(\d+ rows?\)$", line):
                continue
            rows.append(self.split_psql_row(line) if re.search(r"│|\s+\|\s+", line) else [line])
        return rows

    def split_psql_row(self, line):
        if "│" in line:
            return [part.strip() for part in line.split("│")]
        parts = re.split(r"\s+\|\s+", line)
        if len(parts) > 1 and parts[-1].endswith("|"):
            parts[-1] = parts[-1][:-1]
            parts.append("")
        return [part.strip() for part in parts]

    def psql_delimiter_positions(self, header, separator):
        positions = [index for index, char in enumerate(header) if char in "|│"]
        if not positions:
            positions = [index for index, char in enumerate(separator) if char in "+┼"]
        return positions

    def split_psql_table_row(self, line, header, separator):
        positions = self.psql_delimiter_positions(header, separator)
        if not positions:
            return [line.strip()]

        values = []
        start = 0
        actual_delimiters = [index for index, char in enumerate(line) if char in "|│"]
        for position in positions:
            candidates = [candidate for candidate in actual_delimiters if candidate >= start]
            delimiter = min(candidates, key=lambda candidate: abs(candidate - position)) if candidates else position
            values.append(line[start:delimiter].strip())
            start = delimiter + 1
        values.append(line[start:].strip())
        return values

    def psql_expanded_records(self, lines):
        record_header = re.compile(r"^\s*-\[\s*RECORD\s+\d+\s*\][-+]*\s*$", re.I)
        field = re.compile(r"^\s*([^|]+?)\s+\|\s?(.*)$")
        headers = None
        rows = []
        current_headers = None
        current_row = None

        for line in lines:
            if record_header.match(line):
                if current_row is not None:
                    if headers is None:
                        headers = current_headers
                    rows.append(current_row)
                current_headers = []
                current_row = []
                continue
            if current_row is None:
                continue
            match = field.match(line)
            if not match:
                continue
            current_headers.append(match.group(1).strip())
            current_row.append(match.group(2).strip())

        if current_row is not None:
            if headers is None:
                headers = current_headers
            rows.append(current_row)
        if not rows:
            return None
        return headers, rows

    def expected_rows_from_psql_lines(self, lines):
        clean = [line.replace("\r\n", "\n").replace("\r", "\n") for line in lines if re.search(r"\S", line)]
        if not clean:
            return []

        expanded = self.psql_expanded_records(clean)
        if expanded is not None:
            return expanded[1]

        for i in range(len(clean) - 1):
            if not re.match(r"^\s*[-+─┼]+\s*$", clean[i + 1]):
                continue
            rows = []
            saw_row_count = False
            for line in clean[i + 2 :]:
                if re.match(r"^\s*$", line):
                    break
                if re.match(r"^\s*\(\d+ rows?\)\s*$", line):
                    saw_row_count = True
                    break
                if re.match(r"^\s*[-+─┼]+\s*$", line):
                    continue
                rows.append(self.split_psql_table_row(line, clean[i], clean[i + 1]))
            if rows or saw_row_count:
                return rows

        return self.expected_rows_from_plain_lines(clean)

    def psql_headers_from_lines(self, lines):
        clean = [line for line in lines if re.search(r"\S", line)]
        expanded = self.psql_expanded_records(clean)
        if expanded is not None:
            return expanded[0]
        for i in range(len(clean) - 1):
            if re.match(r"^\s*[-+─┼]+\s*$", clean[i + 1]):
                return self.split_psql_table_row(clean[i], clean[i], clean[i + 1])
        return []

    def comparable_value(self, value):
        if value is None:
            return ""
        if value == "":
            return "null"
        if re.match(r"^null$", value, re.I):
            return "null"
        if re.match(rf"^(?:SRID=\d+;)?(?:{WKT_TYPES}){WKT_DIMENSION_SUFFIX}\s*(?:\(|EMPTY\b)", value, re.I):
            value = re.sub(rf"\b({WKT_TYPES})\s+(ZM|Z|M)\s*\(", r"\1\2(", value, flags=re.I)
            value = re.sub(rf"\b({WKT_TYPES})(ZM|Z|M)\s*\(", r"\1\2(", value, flags=re.I)
            value = re.sub(rf"\b({WKT_TYPES})\s+\(", r"\1(", value, flags=re.I)
            value = re.sub(r",\s+", ",", value)
        value = DECIMAL_RE.sub(lambda match: f"{float(match.group(0)):.12g}", value)
        return value

    def canonical_ring(self, ring):
        points = ring.split(",")
        if len(points) > 1 and points[0] == points[-1]:
            points = points[:-1]
        if not points:
            return ring

        rotations = []
        for candidate in (points, list(reversed(points))):
            for index in range(len(candidate)):
                rotated = candidate[index:] + candidate[:index]
                rotations.append(rotated)
        points = min(rotations)
        return ",".join(points + [points[0]])

    def canonical_polygon_wkt(self, value):
        match = re.match(r"^(?:SRID=\d+;)?POLYGON\(\((.*)\)\)$", value, re.I | re.S)
        if not match:
            return value
        rings = match.group(1).split("),(")
        return "POLYGON((" + "),(".join(self.canonical_ring(ring) for ring in rings) + "))"

    def split_wkt_parts(self, value):
        parts = []
        depth = 0
        start = 0
        for index, char in enumerate(value):
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
            elif char == "," and depth == 0:
                parts.append(value[start:index])
                start = index + 1
        parts.append(value[start:])
        return [part.strip() for part in parts if part.strip()]

    def canonical_multipoint_wkt(self, value):
        match = re.match(r"^(?:SRID=\d+;)?MULTIPOINT\((.*)\)$", value, re.I | re.S)
        if not match:
            return value
        points = []
        for point in self.split_wkt_parts(match.group(1)):
            point_match = re.match(r"^\((.*)\)$", point, re.S)
            points.append(point_match.group(1) if point_match else point)
        return "MULTIPOINT(" + ",".join(points) + ")"

    def canonical_multipolygon_wkt(self, value):
        match = re.match(r"^(?:SRID=\d+;)?MULTIPOLYGON\((.*)\)$", value, re.I | re.S)
        if not match:
            return value

        polygons = []
        for polygon in self.split_wkt_parts(match.group(1)):
            polygon_match = re.match(r"^\(\((.*)\)\)$", polygon)
            if not polygon_match:
                return value
            rings = polygon_match.group(1).split("),(")
            polygons.append("((" + "),(".join(self.canonical_ring(ring) for ring in rings) + "))")

        return "MULTIPOLYGON(" + ",".join(sorted(polygons)) + ")"

    def canonical_geometrycollection_wkt(self, value):
        match = re.match(r"^(?:SRID=\d+;)?GEOMETRYCOLLECTION\((.*)\)$", value, re.I | re.S)
        if not match:
            return value

        geometries = [self.canonical_geometry_wkt(part) for part in self.split_wkt_parts(match.group(1))]
        return "GEOMETRYCOLLECTION(" + ",".join(sorted(geometries)) + ")"

    def canonical_surface_wkt(self, value):
        match = re.match(
            rf"^(?:SRID=\d+;)?(POLYHEDRALSURFACE|TIN){WKT_DIMENSION_SUFFIX}\s*\((.*)\)$",
            value, re.I | re.S,
        )
        if not match:
            return value
        faces = []
        for face in self.split_wkt_parts(match.group(2)):
            face_match = re.match(r"^\(\((.*)\)\)$", face, re.S)
            if not face_match:
                return value
            polygon = self.canonical_polygon_wkt(f"POLYGON(({face_match.group(1)}))")
            if not polygon.startswith("POLYGON"):
                return value
            faces.append(polygon[len("POLYGON"):])
        return f"{match.group(1).upper()}(" + ",".join(sorted(faces)) + ")"

    def canonical_geometry_wkt(self, value):
        multipoint = self.canonical_multipoint_wkt(value)
        if multipoint != value:
            return multipoint
        polygon = self.canonical_polygon_wkt(value)
        if polygon != value:
            return polygon
        multipolygon = self.canonical_multipolygon_wkt(value)
        if multipolygon != value:
            return multipolygon
        surface = self.canonical_surface_wkt(value)
        if surface != value:
            return surface
        return self.canonical_geometrycollection_wkt(value)

    def values_equal(self, left, right):
        left_value = self.comparable_value(left)
        right_value = self.comparable_value(right)
        if left_value == right_value:
            return True
        if self.canonical_geometry_wkt(left_value) == self.canonical_geometry_wkt(right_value):
            return True
        if re.match(r"^-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?$", left_value) and re.match(
            r"^-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?$", right_value
        ):
            return abs(float(left_value) - float(right_value)) <= VALUE_TOLERANCE
        return False

    def normalized_header_name(self, value):
        return re.sub(r"[^a-z0-9]+", "_", (value or "").strip().lower()).strip("_")

    def cluster_column_index(self, headers):
        for index, header in enumerate(headers or []):
            if self.normalized_header_name(header) in CLUSTER_COLUMN_NAMES:
                return index
        return None

    def marker_scale_column_index(self, headers):
        for index, header in enumerate(headers or []):
            if self.normalized_header_name(header) == MARKER_SCALE_COLUMN_NAME:
                return index
        return None

    def label_companion_column_index(self, headers, excluded_indexes=None):
        excluded_indexes = set(excluded_indexes or ())
        for index, header in enumerate(headers or []):
            if index in excluded_indexes:
                continue
            if self.normalized_header_name(header) in LABEL_COLUMN_NAMES:
                return index
        return None

    def scalar_companion_value(self, row, index):
        if index is None or index >= len(row):
            return None
        value = row[index]
        if value is None:
            return None
        text = str(value).strip()
        return text or None

    def marker_scale_value(self, value):
        if value is None or not re.match(rf"^{NUMBER_PATTERN}$", value):
            return None
        return max(0.25, min(float(value), 6.0))

    def marker_density_scale(self, total_points, source_point_count):
        density = max(int(total_points or 0), int(source_point_count or 0))
        if density <= 16:
            return 1.0
        if density <= 64:
            return 0.55
        if density <= 256:
            return 0.62
        return 0.48

    def display_label(self, label):
        return re.sub(r"\s+", " ", (label or "").strip().replace("_", " "))

    def grouped_geometry_label(self, header, value, detail=None):
        header_label = self.display_label(header)
        label = f"{header_label} {value}".strip()
        if detail:
            return f"{label}: {detail}" if label else detail
        return label

    def rows_equal(self, left, right):
        if len(left) == 1 and len(left[0]) == 1 and len(right) > 1:
            if all(len(row) == 1 for row in right):
                actual = left[0][0]
                if self.values_equal(actual, "".join(row[0] for row in right)):
                    return True
                if self.values_equal(actual, " ".join(row[0] for row in right)):
                    return True

        if len(left) != len(right):
            return False
        for left_row, right_row in zip(left, right):
            if len(left_row) != len(right_row):
                return False
            for left_value, right_value in zip(left_row, right_row):
                if not self.values_equal(left_value, right_value):
                    return False
        return True

    def version_rows_equal(self, actual, expected):
        if len(actual) != len(expected):
            return False
        for actual_row, expected_row in zip(actual, expected):
            if len(actual_row) != len(expected_row):
                return False
        return True

    def catalog_rows_equal(self, actual, expected):
        if not expected:
            return False
        if not actual:
            return True
        expected_width = len(expected[0])
        return all(len(row) == expected_width for row in actual)

    def rows_to_string(self, rows):
        return "\n".join(" | ".join(row) for row in rows)

    def parse_example_node(self, node):
        text = self.node_text(node)
        screen = self.following_screen(node)
        if screen is not None:
            return self.parse_adjacent_example(text, self.node_text(screen))
        return None, None

    def quoted_ranges(self, text):
        ranges = []
        index = 0
        while index < len(text):
            if text.startswith("--", index):
                newline = text.find("\n", index + 2)
                index = len(text) if newline < 0 else newline + 1
                continue
            if text.startswith("/*", index):
                closing = text.find("*/", index + 2)
                index = len(text) if closing < 0 else closing + 2
                continue
            if text[index] == "$":
                delimiter = re.match(r"\$(?:[A-Za-z_][A-Za-z0-9_]*)?\$", text[index:])
                if delimiter:
                    token = delimiter.group(0)
                    start = index + len(token)
                    closing = text.find(token, start)
                    if closing < 0:
                        break
                    ranges.append((start, closing))
                    index = closing + len(token)
                    continue
            if text[index] != "'":
                if text[index] == '"':
                    start = index + 1
                    index += 1
                    while index < len(text):
                        if text[index] != '"':
                            index += 1
                            continue
                        if index + 1 < len(text) and text[index + 1] == '"':
                            index += 2
                            continue
                        ranges.append((start, index))
                        index += 1
                        break
                    continue
                index += 1
                continue
            start = index + 1
            index += 1
            while index < len(text):
                if text[index] != "'":
                    index += 1
                    continue
                if index + 1 < len(text) and text[index + 1] == "'":
                    index += 2
                    continue
                ranges.append((start, index))
                index += 1
                break
        return ranges

    def closing_parenthesis(self, text, opening):
        depth = 0
        for index in range(opening, len(text)):
            if text[index] == "(":
                depth += 1
            elif text[index] == ")":
                depth -= 1
                if depth == 0:
                    return index
        return -1

    def geometry_candidate_matches(self, text, quoted_only=False):
        quoted = self.quoted_ranges(text) if quoted_only else []
        candidates = []
        cursor = 0
        while True:
            match = WKT_START_RE.search(text, cursor)
            if not match:
                break
            if match.start() and re.match(r"[A-Za-z0-9_]", text[match.start() - 1]):
                cursor = match.end()
                continue
            opening = text.find("(", match.start())
            closing = self.closing_parenthesis(text, opening)
            if closing < 0:
                raise RuntimeError(f"Unclosed geometry candidate near: {text[match.start():match.start() + 80]}")
            cursor = closing + 1
            if closing + 1 < len(text) and re.match(r"[A-Za-z0-9_]", text[closing + 1]):
                continue
            if quoted_only and not any(match.start() >= start and closing + 1 <= end for start, end in quoted):
                continue
            candidate = text[match.start():closing + 1].replace("''", "'")
            candidates.append({"start": match.start(), "end": closing + 1, "wkt": candidate})
        return candidates

    def geometry_candidates(self, text, quoted_only=False):
        return [match["wkt"] for match in self.geometry_candidate_matches(text, quoted_only)]

    def code_geometry_candidates(self, text):
        candidates = self.geometry_candidate_matches(text, quoted_only=True)
        for candidate in candidates:
            alias = re.match(
                r'\s*\'\s*::\s*(?:geometry|geography)\s+(?:AS\s+)?'
                r'("(?:""|[^"])+"|[A-Za-z_][A-Za-z0-9_]*)',
                text[candidate["end"]:],
                re.I,
            )
            if alias:
                label = alias.group(1)
                candidate["label"] = (
                    label[1:-1].replace('""', '"') if label.startswith('"') else label
                )
        for match in MAKE_ENVELOPE_RE.finditer(text):
            min_x, min_y, max_x, max_y = match.group(1, 2, 3, 4)
            wkt = (
                f"POLYGON(({min_x} {min_y},{max_x} {min_y},{max_x} {max_y},"
                f"{min_x} {max_y},{min_x} {min_y}))"
            )
            if match.group(5) is not None:
                wkt = f"SRID={match.group(5)};{wkt}"
            candidates.append({
                "start": match.start(), "end": match.end(), "wkt": wkt, "label": "Envelope",
            })
        for match in POINT_CONSTRUCTOR_RE.finditer(text):
            candidates.append({
                "start": match.start(),
                "end": match.end(),
                "wkt": f"POINT({match.group(1)} {match.group(2)})",
                "label": "Point",
            })
        return sorted(candidates, key=lambda candidate: candidate["start"])

    def query_output_headers(self, query):
        """Return explicit aliases from the outer SELECT list.

        Documented-only visual examples are not executed because their result
        ordering is intentionally unstable.  Their SQL aliases still carry the
        same input/output layer semantics as executed examples, so retain those
        aliases without introducing a SQL parser dependency.
        """
        masked = list(query)
        for start, end in self.quoted_ranges(query):
            masked[start:end] = " " * (end - start)
        masked_text = "".join(masked)
        masked_text = re.sub(r"--[^\n]*|/\*.*?\*/", lambda match: " " * len(match.group(0)), masked_text, flags=re.S)

        depth = 0
        select_start = None
        from_start = None
        index = 0
        while index < len(masked_text):
            char = masked_text[index]
            if char == "(":
                depth += 1
            elif char == ")":
                depth = max(0, depth - 1)
            elif depth == 0:
                token = re.match(r"(?i)(SELECT|FROM)\b", masked_text[index:])
                if token and (index == 0 or not re.match(r"[A-Za-z0-9_]", masked_text[index - 1])):
                    if token.group(1).upper() == "SELECT":
                        select_start = index + len(token.group(0))
                        from_start = None
                    elif select_start is not None:
                        from_start = index
                        break
                    index += len(token.group(0))
                    continue
            index += 1
        if select_start is None:
            return []
        select_end = from_start if from_start is not None else len(query)

        items = []
        item_start = select_start
        depth = 0
        for index in range(select_start, select_end):
            char = masked_text[index]
            if char == "(":
                depth += 1
            elif char == ")":
                depth = max(0, depth - 1)
            elif char == "," and depth == 0:
                items.append(query[item_start:index])
                item_start = index + 1
        items.append(query[item_start:select_end])

        headers = []
        for item in items:
            alias = re.search(r'(?i)\bAS\s+("(?:""|[^"])+"|[A-Za-z_][A-Za-z0-9_]*)\s*;?\s*$', item)
            header = alias.group(1) if alias else ""
            if header.startswith('"'):
                header = header[1:-1].replace('""', '"')
            headers.append(header)
        return headers

    def query_is_auto_safe(self, query):
        if query is None:
            return False
        if not re.search(r"^\s*(?:--[^\n]*\n\s*)*(?:SELECT|WITH)\b", query, re.I):
            return False
        return query.count(";") <= 1

    def query_is_version_example(self, query):
        return query is not None and bool(VERSION_FUNCTION_RE.search(query))

    def query_is_catalog_example(self, query):
        return query is not None and bool(CATALOG_QUERY_RE.search(query))

    def expected_rows_are_auto_safe(self, expected):
        if not expected:
            return False
        for row in expected:
            for value in row:
                if re.search(r"\b(?:SELECT|WITH)\b", value, re.I):
                    return False
                if (
                    "..." in value
                    or re.search(r"\.\.(?:[),]|\s*$)", value)
                    or re.search(r"(?:AD INFINITUM|NOTICE:|ERROR:)", value, re.I)
                ):
                    return False
        return True

    def example_for_node(self, node):
        text = self.node_text(node)
        screen = self.following_screen(node)
        forced = self.has_role(node, FORCE_ROLE)
        # Layout roles are also explicit author requests for a generated figure.
        # This matters for documented-only examples whose output is intentionally
        # not executed because it can vary between dependency versions.
        explicit_visual = screen is not None and any(
            self.has_role(screen, role)
            for role in (
                VISUAL_ROLE,
                VISUAL_DIRECTION_ROLE,
                VISUAL_SEPARATE_OUTPUT_ROLE,
                VISUAL_ROW_PANELS_ROLE,
                VISUAL_MATRIX_3X3_ROLE,
                VISUAL_OVERLAY_ROLE,
            )
        )
        skip_reason = self.obvious_skip_reason(text)
        documented_output = self.has_role(node, DOCUMENTED_OUTPUT_ROLE)
        documented_only = documented_output or bool(explicit_visual and skip_reason)
        if self.has_role(node, EXTERNAL_STATE_ROLE) and not forced and not documented_only:
            return None
        if not forced:
            if not self.looks_query(text):
                return None
            if skip_reason and not documented_only:
                return None

        query, expected = self.parse_example_node(node)
        valid = query is not None and expected is not None
        if not forced and (
            not valid or not self.query_is_auto_safe(query) or not self.expected_rows_are_auto_safe(expected)
        ):
            return None

        refentry = ""
        screen_ordinal = 0
        expected_headers = self.psql_headers_from_lines(self.node_text(screen).split("\n")) if screen is not None else []
        skip_visual = self.has_role(node, VISUAL_SKIP_ROLE) or (
            screen is not None and self.has_role(screen, VISUAL_SKIP_ROLE)
        )
        visual_separate_output = screen is not None and self.has_role(
            screen, VISUAL_SEPARATE_OUTPUT_ROLE
        )
        visual_row_panels = screen is not None and self.has_role(
            screen, VISUAL_ROW_PANELS_ROLE
        )
        visual_matrix_3x3 = screen is not None and self.has_role(screen, VISUAL_MATRIX_3X3_ROLE)
        visual_overlay = screen is not None and self.has_role(screen, VISUAL_OVERLAY_ROLE)
        visual_direction = screen is not None and self.has_role(screen, VISUAL_DIRECTION_ROLE)
        visual_output_only = screen is not None and self.has_role(screen, VISUAL_OUTPUT_ONLY_ROLE)
        visual_hide_output_area_vertices = screen is not None and self.has_role(
            screen, VISUAL_HIDE_OUTPUT_AREA_VERTICES_ROLE
        )
        if visual_separate_output and visual_overlay:
            raise RuntimeError(
                f"Visual example {self.source_label(node)} cannot request both "
                f"{VISUAL_SEPARATE_OUTPUT_ROLE} and {VISUAL_OVERLAY_ROLE}"
            )
        visual = None
        if screen is not None and not skip_visual:
            refentry, screen_ordinal = self.visual_location(screen)
            visual = self.visual_candidate(query, expected, explicit_visual)

        return {
            "label": self.source_label(node),
            "query": query,
            "expected": expected,
            "expected_headers": expected_headers,
            "valid": valid,
            "forced": forced,
            "version": self.query_is_version_example(query),
            "catalog": self.query_is_catalog_example(query),
            "requirements": self.merge_capability_requirements(
                self.capability_requirements(node)
                + self.inferred_capability_requirements(query)
            ),
            "documented_only": documented_only,
            "volatile": self.query_is_version_example(query) or self.query_is_catalog_example(query),
            "visual_id": self.visual_id(screen) if visual is not None else None,
            "visual_kind": visual["kind"] if visual is not None else None,
            "visual_preferred": visual["preferred"] if visual is not None else False,
            "visual_direction": visual_direction,
            "visual_hide_output_area_vertices": visual_hide_output_area_vertices,
            "visual_output_only": visual_output_only,
            "visual_separate_output": visual_separate_output,
            "visual_row_panels": visual_row_panels,
            "visual_matrix_3x3": visual_matrix_3x3,
            "visual_overlay": visual_overlay,
            "geometry_output_precision": self.geometry_output_precision(screen),
            "visual_refentry": refentry,
            "visual_screen": screen_ordinal,
        }

    def examples(self):
        examples = []
        for node in self.doc.getroot().iter(f"{{{DOCBOOK_NS}}}programlisting"):
            example = self.example_for_node(node)
            if example is not None:
                examples.append(example)
        return examples

    def print_report(self):
        stats = {
            "programlisting_total": 0,
            "sql_like": 0,
            "select_or_with": 0,
            "legacy_inband_expected": 0,
            "external_context": 0,
            "placeholder_output": 0,
            "document_output": 0,
            "unstable_output": 0,
            "notice_or_error": 0,
            "cleanish_runnable_query": 0,
            "obvious_not_runnable_query": 0,
            "cleanish_legacy_inband_expected": 0,
            "obvious_bad_legacy_inband_expected": 0,
            "adjacent_programlisting_screen": 0,
            "cleanish_adjacent_expected": 0,
            "obvious_bad_adjacent_expected": 0,
            "requires_external_state": 0,
            "auto_example_tests": 0,
            "forced_example_tests": 0,
            "volatile_version_tests": 0,
        }

        for node in self.doc.getroot().iter(f"{{{DOCBOOK_NS}}}programlisting"):
            text = self.node_text(node)
            stats["programlisting_total"] += 1
            if self.has_role(node, EXTERNAL_STATE_ROLE):
                stats["requires_external_state"] += 1
            if self.looks_sql(text):
                stats["sql_like"] += 1
            if not self.looks_query(text):
                continue
            stats["select_or_with"] += 1

            reason = self.obvious_skip_reason(text)
            if reason == "external-context":
                stats["external_context"] += 1
            if reason == "placeholder-output":
                stats["placeholder_output"] += 1
            if reason == "document-output":
                stats["document_output"] += 1
            if reason == "unstable-output":
                stats["unstable_output"] += 1
            if reason == "notice-or-error":
                stats["notice_or_error"] += 1

            if reason:
                stats["obvious_not_runnable_query"] += 1
            else:
                stats["cleanish_runnable_query"] += 1

            if self.has_legacy_inband_expected(text):
                stats["legacy_inband_expected"] += 1
                if reason:
                    stats["obvious_bad_legacy_inband_expected"] += 1
                else:
                    stats["cleanish_legacy_inband_expected"] += 1

            screen = self.following_screen(node)
            if screen is not None:
                stats["adjacent_programlisting_screen"] += 1
                screen_text = self.node_text(screen)
                bad_screen = re.search(r"\.\.\.|NOTICE:|ERROR:", screen_text, re.I)
                if reason or bad_screen:
                    stats["obvious_bad_adjacent_expected"] += 1
                else:
                    stats["cleanish_adjacent_expected"] += 1

        examples = self.examples()
        stats["auto_example_tests"] = len([example for example in examples if not example["forced"]])
        stats["forced_example_tests"] = len([example for example in examples if example["forced"]])
        stats["volatile_version_tests"] = len([example for example in examples if example["volatile"]])
        visual_kinds = {
            kind: len([example for example in examples if example["visual_kind"] == kind])
            for kind in ("explicit", "geometry-output", "input-context")
        }
        stats["visual_examples"] = sum(visual_kinds.values())
        stats["visual_explicit"] = visual_kinds["explicit"]
        stats["visual_geometry_output"] = visual_kinds["geometry-output"]
        stats["visual_input_context"] = visual_kinds["input-context"]
        stats["example_tests"] = len(examples)
        stats["parseable_example_tests"] = len([example for example in examples if example["valid"]])
        invalid_tests = stats["example_tests"] - stats["parseable_example_tests"]
        skipped_query_candidates = stats["select_or_with"] - stats["example_tests"]
        if invalid_tests:
            status = f"ERROR: {invalid_tests} selected example test(s) could not be parsed."
        elif stats["example_tests"] == 0:
            status = "WARNING: no manual example tests were selected."
        else:
            status = "OK: selected manual example tests are parseable."

        print("Manual example test report")
        print(f"Status: {status}")
        print("")
        print("Selected tests:")
        print(f"  {stats['example_tests']} total examples selected for --run")
        print(f"  {stats['parseable_example_tests']} parseable examples")
        print(f"  {stats['volatile_version_tests']} version/catalog examples checked with relaxed comparison")
        print(f"  {stats['forced_example_tests']} forced by role=\"{FORCE_ROLE}\"")
        print(f"  {stats['visual_examples']} useful geometry figures selected at build time")
        print(f"    {stats['visual_geometry_output']} geometry outputs")
        print(f"    {stats['visual_input_context']} input-context figures with textual output")
        print(f"    {stats['visual_explicit']} explicit role=\"{VISUAL_ROLE}\" overrides")
        print("")
        print("Skipped by design:")
        print(f"  {stats['requires_external_state']} marked role=\"{EXTERNAL_STATE_ROLE}\"")
        print(f"  {skipped_query_candidates} SELECT/WITH program listings not selected")
        print(f"  {stats['external_context']} need tables, files, topology state, or other external context")
        print(f"  {stats['placeholder_output']} have placeholder output")
        print(f"  {stats['unstable_output']} have intentionally unstable output")
        print(f"  {stats['notice_or_error']} show NOTICE/ERROR output")
        print("")
        print("Document scan:")
        print(f"  {stats['programlisting_total']} programlisting blocks")
        print(f"  {stats['sql_like']} SQL-like blocks")
        print(f"  {stats['select_or_with']} SELECT/WITH query blocks")
        print(f"  {stats['adjacent_programlisting_screen']} query blocks with adjacent <screen> output")
        print("")
        print("Raw counters:")

        for key in sorted(stats):
            print(f"{key}={stats[key]}")

    def run_psql_query(self, database, query):
        cmd = ["psql", "-X", "-v", "ON_ERROR_STOP=1", "-P", "null=null", "-d", database, "-c", query]
        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True, encoding="utf-8",
                env=self.psql_environment(), timeout=self.psql_subprocess_timeout(),
            )
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError(
                f"psql timed out after {exc.timeout:g}s for query:\n{query}"
            ) from exc
        if result.returncode != 0:
            raise RuntimeError(f"psql failed for query:\n{query}\n{result.stdout}{result.stderr}")
        lines = result.stdout.split("\n")
        return QueryRows(
            self.expected_rows_from_psql_lines(lines),
            self.psql_headers_from_lines(lines),
        )

    def psql_environment(self):
        env = os.environ.copy()
        options = env.get("PGOPTIONS", "").strip()
        settings = []
        if "postgis.gdal_enabled_drivers" not in options:
            settings.extend(["-c", "postgis.gdal_enabled_drivers=PNG"])
        if "statement_timeout" not in options:
            timeout = env.get("POSTGIS_EXAMPLETEST_STATEMENT_TIMEOUT", "60s")
            settings.extend(["-c", f"statement_timeout={timeout}"])
        if settings:
            options = f"{options} {' '.join(settings)}".strip()
        env["PGOPTIONS"] = options
        return env

    def duration_seconds(self, value):
        match = re.fullmatch(r"\s*(\d+(?:\.\d+)?)(ms|s|min)?\s*", value or "")
        if not match:
            return None
        duration = float(match.group(1))
        unit = match.group(2) or "s"
        if unit == "ms":
            return duration / 1000.0
        if unit == "min":
            return duration * 60.0
        return duration

    def psql_subprocess_timeout(self):
        override = os.environ.get("POSTGIS_EXAMPLETEST_SUBPROCESS_TIMEOUT")
        if override is not None:
            if override.strip().lower() in {"0", "off", "none"}:
                return None
            return self.duration_seconds(override)
        statement_timeout = os.environ.get("POSTGIS_EXAMPLETEST_STATEMENT_TIMEOUT", "60s")
        seconds = self.duration_seconds(statement_timeout)
        if seconds is None:
            return None
        return seconds + 15.0

    def query_output_types(self, database, query):
        query = re.sub(r";\s*$", "", query.strip())
        cmd = [
            "psql", "-X", "-A", "-t", "-F", "\t", "-P", "footer=off",
            "-v", "ON_ERROR_STOP=1", "-d", database,
        ]
        try:
            result = subprocess.run(
                cmd, input=f"{query}\n\\gdesc\n", capture_output=True, text=True,
                encoding="utf-8", env=self.psql_environment(),
                timeout=self.psql_subprocess_timeout(),
            )
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError(
                f"psql timed out after {exc.timeout:g}s while describing a visual example:\n{query}"
            ) from exc
        if result.returncode != 0:
            raise RuntimeError(
                f"psql failed while describing a visual example:\n{query}\n"
                f"{result.stdout}{result.stderr}"
            )
        return [
            line.rsplit("\t", 1)[1].strip().lower()
            for line in result.stdout.splitlines()
            if "\t" in line
        ]

    def bytea_image(self, value):
        if not isinstance(value, str) or not value.startswith("\\x"):
            return None
        try:
            raw = bytes.fromhex(value[2:])
        except ValueError:
            return None
        if raw.startswith(b"\x89PNG\r\n\x1a\n") and len(raw) >= 24:
            width = int.from_bytes(raw[16:20], "big")
            height = int.from_bytes(raw[20:24], "big")
            return {
                "format": "PNG", "media_type": "image/png", "width": width,
                "height": height, "data": base64.b64encode(raw).decode("ascii"),
            }
        return None

    def prepare_visual_rows(self, actual, types):
        if not actual:
            return QueryRows(actual, getattr(actual, "headers", []), types)
        actual = self.coalesce_single_column_wkt_rows(actual)
        width = max((len(row) for row in actual), default=0)
        if len(types) != width:
            raise RuntimeError(
                f"Visual query returned {width} columns but psql described {len(types)}"
            )
        headers = getattr(actual, "headers", [])
        rows = []
        visuals = []
        for row_index, row in enumerate(actual, 1):
            row_label = next((
                value.strip() for column_index, value in enumerate(row)
                if types[column_index] != "bytea"
                and isinstance(value, str) and 0 < len(value.strip()) <= 80
                and not self.geometry_type(value)
            ), "")
            display_row = []
            for column_index, value in enumerate(row):
                image = self.bytea_image(value) if types[column_index] == "bytea" else None
                if image is None:
                    display_row.append(value)
                    continue
                header = headers[column_index] if column_index < len(headers) else ""
                source = "Code" if header.lower().startswith("input_") else "Output"
                label = header[len("input_"):] if source == "Code" else header
                label = self.display_label(label)
                row_labeled = False
                if not label or label.lower() in {"?column?", "image", "st_aspng"}:
                    label = self.display_label(row_label) or source
                    row_labeled = bool(row_label)
                if len(actual) > 1 and not row_labeled:
                    label = f"{label} {row_index}"
                image.update({
                    "source": source, "label": label, "row": row_index,
                    "column": column_index + 1,
                })
                visuals.append(image)
                display_row.append(
                    f"{image['format']} image, {image['width']} x {image['height']} pixels"
                )
            rows.append(display_row)
        return QueryRows(rows, headers, types, visuals)

    def coalesce_single_column_wkt_rows(self, rows):
        if not (
            len(rows) > 1
            and all(len(row) == 1 and isinstance(row[0], str) for row in rows)
        ):
            return rows
        for candidate in (
            "".join(row[0] for row in rows),
            " ".join(row[0] for row in rows),
            "\n".join(row[0] for row in rows),
        ):
            value = candidate.strip()
            if not WKT_VALUE_RE.match(value):
                continue
            try:
                matches = self.geometry_candidates(value)
            except RuntimeError:
                continue
            if len(matches) == 1 and matches[0].strip() == value:
                return QueryRows(
                    [[value]],
                    getattr(rows, "headers", []),
                    getattr(rows, "types", []),
                    getattr(rows, "visuals", []),
                )
        return rows

    def typed_rows_equal(self, database, actual, expected, expected_wkt_digits=None):
        if len(actual) == 1 and len(actual[0]) == 1:
            expected = self.coalesce_single_column_wkt_rows(expected)
        if len(actual) != len(expected) or any(
            len(actual_row) != len(expected_row)
            for actual_row, expected_row in zip(actual, expected)
        ):
            return False
        for actual_row, expected_row in zip(actual, expected):
            for column, (actual_value, expected_value) in enumerate(zip(actual_row, expected_row)):
                output_type = actual.types[column] if column < len(actual.types) else ""
                actual_hex = self.geometry_output_hex(actual_value, output_type)
                expected_hex = self.geometry_output_hex(expected_value, output_type)
                actual_wkt = (
                    actual_value.strip()
                    if isinstance(actual_value, str) and WKT_VALUE_RE.match(actual_value)
                    else None
                )
                expected_wkt = (
                    expected_value.strip()
                    if isinstance(expected_value, str) and WKT_VALUE_RE.match(expected_value)
                    else None
                )
                if actual_hex is not None or expected_hex is not None:
                    if actual_hex is None or (expected_hex is None and expected_wkt is None):
                        return False
                    documented_digits = (
                        expected_wkt_digits
                        if expected_wkt_digits is not None else
                        self.documented_wkt_decimal_digits(expected_wkt)
                        if expected_wkt is not None else VALUE_DECIMAL_DIGITS
                    )
                    if (
                        expected_hex is None
                        and expected_wkt is not None
                        and expected_wkt_digits is None
                        and 0 < documented_digits < VALUE_DECIMAL_DIGITS
                    ):
                        geom = (
                            "ST_GeomFromEWKB(decode("
                            f"'{actual_hex['hexwkb']}', 'hex'))"
                        )
                        if self.documented_wkt_uses_explicit_z_token(expected_wkt):
                            formatter = (
                                f"CASE WHEN ST_SRID({geom}) = 0 "
                                f"THEN ST_AsText({geom}, {documented_digits}) "
                                f"ELSE 'SRID=' || ST_SRID({geom}) || ';' || "
                                f"ST_AsText({geom}, {documented_digits}) END"
                            )
                        else:
                            formatter = f"ST_AsEWKT({geom}, {documented_digits})"
                        actual_text = self.run_psql_scalar(
                            database,
                            f"SELECT {formatter}",
                        )
                        if not re.match(r"^\s*SRID\s*=", expected_wkt, re.I):
                            actual_text = re.sub(r"^\s*SRID\s*=\s*\d+\s*;\s*", "", actual_text, flags=re.I)
                        if not self.values_equal(actual_text, expected_wkt):
                            return False
                        continue
                    query = self.geometry_comparison_query(
                        actual_hex["hexwkb"],
                        expected_hex["hexwkb"] if expected_hex is not None else None,
                        expected_wkt,
                        actual_hex["type"],
                        expected_wkt_digits,
                    )
                    if self.run_psql_scalar(database, query) != "t":
                        return False
                elif actual_wkt is not None or expected_wkt is not None:
                    if actual_wkt is None or expected_wkt is None:
                        return False
                    if not self.values_equal(actual_wkt, expected_wkt):
                        return False
                elif not self.values_equal(actual_value, expected_value):
                    return False
        return True

    def run_examples(self, database, keep_going=False, render_dir=None, visual_only=False, jobs=1):
        all_examples = self.examples()
        required_capabilities = {
            requirement["name"]
            for example in all_examples
            for requirement in example.get("requirements", [])
        }
        capabilities = self.runtime_capabilities(database, required_capabilities)
        unavailable_examples = [
            example for example in all_examples
            if not self.requirements_satisfied(example.get("requirements", []), capabilities)
        ]
        documented_visuals = [
            example for example in all_examples
            if example.get("documented_only") and example["visual_id"]
        ]
        examples = [
            example for example in all_examples
            if not example.get("documented_only") and example not in unavailable_examples
        ]
        if visual_only:
            examples = [example for example in examples if example["visual_id"]]
        else:
            self.check_environment(database)
        failures = []
        ran = 0
        visuals = []

        def render_documented_visuals():
            rendered = []
            for example in documented_visuals:
                rows = QueryRows(
                    example["expected"],
                    example.get("expected_headers", []),
                    self.documented_visual_types(example["expected"]),
                )
                visual = self.render_visual_example(database, example, rows)
                if visual is not None:
                    rendered.append(visual)
            return rendered

        if visual_only and jobs > 1:
            def run_visual(example):
                try:
                    actual = self.run_one_example(database, example)
                    return {
                        "label": example["label"],
                        "visual": self.render_visual_example(database, example, actual),
                    }
                except RuntimeError as exc:
                    if not keep_going:
                        raise
                    return {
                        "label": example["label"],
                        "failure": str(exc),
                    }

            completed = 0
            with ThreadPoolExecutor(max_workers=jobs) as pool:
                futures = [pool.submit(run_visual, example) for example in examples]
                for future in as_completed(futures):
                    result = future.result()
                    completed += 1
                    if result.get("failure"):
                        print(
                            f"Example test failed to run: {result['label']}\n"
                            f"{result['failure']}",
                            file=sys.stderr,
                            flush=True,
                        )
                        failures.append(result["label"])
                        continue
                    visual = result.get("visual")
                    if visual is not None:
                        visuals.append(visual)
                    ran += 1
                    if completed == len(futures) or completed % 50 == 0:
                        print(
                            f"manual visual examples processed: {completed}/{len(futures)}",
                            file=sys.stderr,
                            flush=True,
                        )
            if failures:
                raise RuntimeError(f"FAILED {len(failures)} example(s): {', '.join(failures)}")
            if render_dir:
                visuals.extend(render_documented_visuals())
            if render_dir:
                self.publish_visual_examples(render_dir, visuals)
            print(f"manual example tests passed: {ran}")
            if unavailable_examples:
                print(f"manual examples skipped for unavailable capabilities: {len(unavailable_examples)}")
            if render_dir:
                print(f"visual examples rendered: {len(visuals)}")
            return

        for example in examples:
            try:
                actual = self.run_one_example(database, example)
                visual = self.render_visual_example(database, example, actual) if example["visual_id"] else None
            except RuntimeError as exc:
                if not keep_going:
                    raise
                print(exc, file=sys.stderr)
                failures.append(example["label"])
                continue
            ran += 1
            if visual:
                visuals.append(visual)

        if failures:
            raise RuntimeError(f"FAILED {len(failures)} example(s): {', '.join(failures)}")

        if render_dir:
            visuals.extend(render_documented_visuals())
            self.publish_visual_examples(render_dir, visuals)

        print(f"manual example tests passed: {ran}")
        if unavailable_examples:
            print(f"manual examples skipped for unavailable capabilities: {len(unavailable_examples)}")
        if render_dir:
            print(f"visual examples rendered: {len(visuals)}")

    def run_one_example(self, database, example):
        if not example["valid"]:
            raise RuntimeError(f"Could not parse manual example test at {example['label']}")

        try:
            actual = self.run_psql_query(database, example["query"])
            if example.get("visual_kind") == "explicit" or (
                (
                    self.expected_has_hexwkb(example["expected"])
                    or self.expected_has_geometry_text(example["expected"])
                )
            ):
                actual = self.prepare_visual_rows(
                    actual, self.query_output_types(database, example["query"])
                )
        except RuntimeError as exc:
            raise RuntimeError(f"Example test failed to run: {example['label']}\n{exc}") from exc

        if example["catalog"]:
            equal = self.catalog_rows_equal(actual, example["expected"])
        elif example["version"]:
            equal = self.version_rows_equal(actual, example["expected"])
        elif (
            any(GEOMETRY_OUTPUT_TYPE_RE.fullmatch(output_type or "") for output_type in getattr(actual, "types", []))
            and (
                self.expected_has_hexwkb(example["expected"])
                or self.expected_has_geometry_text(example["expected"])
            )
        ):
            equal = self.typed_rows_equal(
                database,
                actual,
                example["expected"],
                example.get("geometry_output_precision"),
            )
        else:
            equal = self.rows_equal(actual, example["expected"])
        if not equal:
            raise RuntimeError(
                f"Example test failed: {example['label']}\n"
                f"Query:\n{example['query']}\n"
                f"Expected:\n{self.rows_to_string(example['expected'])}\n"
                f"Actual:\n{self.rows_to_string(actual)}"
            )
        return actual

    def run_psql_scalar(self, database, query):
        cmd = ["psql", "-X", "-A", "-t", "-v", "ON_ERROR_STOP=1", "-d", database]
        try:
            result = subprocess.run(
                cmd, input=f"{query}\n", capture_output=True, text=True, encoding="utf-8",
                env=self.psql_environment(), timeout=self.psql_subprocess_timeout(),
            )
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError(
                f"psql timed out after {exc.timeout:g}s while rendering a visual example"
            ) from exc
        if result.returncode != 0:
            raise RuntimeError(f"psql failed while rendering a visual example:\n{result.stdout}{result.stderr}")
        return result.stdout.strip()

    def visual_payload(self, database, layers):
        encoded = base64.b64encode(
            json.dumps(layers, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        ).decode("ascii")
        query = f"""
WITH raw AS (
  SELECT ord, source, label, row_num, column_num, requested_frame, wkt, hexwkb,
    source_point_count, total_points, marker_scale
  FROM jsonb_to_recordset(
    convert_from(decode('{encoded}', 'base64'), 'UTF8')::jsonb
  ) AS item(ord integer, source text, label text, row_num integer, column_num integer,
            requested_frame text, wkt text, hexwkb text, source_point_count integer,
            total_points integer, marker_scale double precision)
), parsed_all AS (
  SELECT ord, source, label, row_num, column_num, requested_frame,
    source_point_count, total_points, marker_scale, hexwkb,
    CASE WHEN hexwkb IS NOT NULL
      THEN ST_GeomFromEWKB(decode(hexwkb, 'hex'))
      ELSE ST_GeomFromEWKT(wkt)
    END AS geom
  FROM raw
), parsed_raw AS (
  SELECT ord, source, label, row_num, column_num, requested_frame,
    source_point_count, total_points, marker_scale, geom
  FROM parsed_all
), source_dimensions AS (
  SELECT source, max(ST_Dimension(geom)) AS dimension
  FROM parsed_raw
  GROUP BY source
), frame_policy AS (
  SELECT
    EXISTS (SELECT 1 FROM parsed_raw WHERE source = 'Code')
    AND EXISTS (SELECT 1 FROM parsed_raw WHERE source = 'Output')
    AND (
      (SELECT dimension FROM source_dimensions WHERE source = 'Code') =
        (SELECT dimension FROM source_dimensions WHERE source = 'Output')
      OR (SELECT count(DISTINCT ST_SRID(geom)) FROM parsed_raw) > 1
    ) AS separate_sources,
    (SELECT count(DISTINCT ST_SRID(geom)) FROM parsed_raw) = 1 AS shared_coordinate_space
  FROM parsed_raw
  LIMIT 1
), framed AS (
  SELECT ord, source, label, row_num, column_num, source_point_count, total_points,
    marker_scale, geom, shared_coordinate_space,
    COALESCE(requested_frame,
      CASE WHEN separate_sources THEN source ELSE 'Overlay' END) AS frame
  FROM parsed_raw CROSS JOIN frame_policy
), frame_bounds_raw AS (
  SELECT frame, min(ord) AS first_ord,
    CASE WHEN bool_and(shared_coordinate_space)
      THEN (SELECT ST_Extent(ST_Force2D(all_geometries.geom)) FROM framed AS all_geometries)
      ELSE ST_Extent(ST_Force2D(geom))
    END AS box,
    CASE WHEN count(DISTINCT ST_SRID(geom)) = 1 THEN max(ST_SRID(geom)) ELSE 0 END AS srid
  FROM framed
  GROUP BY frame
), frame_bounds AS (
  SELECT frame, first_ord, srid,
    ST_XMin(box) AS min_x, ST_YMin(box) AS min_y,
    ST_XMax(box) - ST_XMin(box) AS width,
    ST_YMax(box) - ST_YMin(box) AS height
  FROM frame_bounds_raw
  WHERE box IS NOT NULL
), parts AS (
  SELECT ord, source, label, row_num, column_num, frame,
    source_point_count,
    COALESCE(total_points, ST_NPoints(framed.geom)) AS total_points,
    marker_scale,
    GeometryType(framed.geom) AS root_type,
    part.geom
  FROM framed
  CROSS JOIN LATERAL generate_series(1, ST_NumGeometries(framed.geom)) AS part_num(n)
  CROSS JOIN LATERAL (
    SELECT ST_GeometryN(framed.geom, part_num.n) AS geom
  ) AS part
  WHERE NOT ST_IsEmpty(part.geom)
), translated_parts AS (
  SELECT ord, source, label, row_num, column_num, parts.frame, source_point_count,
    total_points, marker_scale, root_type,
    ST_Translate(geom, -min_x, -min_y) AS geom
  FROM parts JOIN frame_bounds USING (frame)
), render_parts AS (
  SELECT *,
    geom AS render_geom
  FROM translated_parts
)
SELECT json_build_object(
  'frames', (
    SELECT json_agg(json_build_object(
      'id', frame,
      'label', CASE WHEN srid > 0 THEN frame || ' (SRID ' || srid || ')' ELSE frame END,
      'bounds', json_build_array(0, 0, width, height)
    ) ORDER BY first_ord)
    FROM frame_bounds
  ),
  'parts', (
    SELECT json_agg(json_build_object(
      'ord', ord, 'source', source, 'label', label,
      'row', row_num, 'column', column_num, 'frame', frame,
      'root_type', root_type, 'source_point_count', source_point_count,
      'total_points', total_points, 'marker_scale', marker_scale,
      'type', GeometryType(geom),
      'svg', CASE
        WHEN ST_Dimension(render_geom) = 0 THEN NULL
        ELSE ST_AsSVG(ST_Force2D(render_geom), 0, 12)
      END,
      'dimension', ST_Dimension(geom), 'srid', ST_SRID(geom),
      'x', CASE WHEN GeometryType(geom) = 'POINT' THEN ST_X(geom) END,
      'y', CASE WHEN GeometryType(geom) = 'POINT' THEN -ST_Y(geom) END,
      'points', CASE
        WHEN ST_Dimension(render_geom) = 0
        THEN (
          SELECT json_agg(json_build_array(ST_X(point.geom), -ST_Y(point.geom)))
          FROM ST_DumpPoints(render_geom) AS point
        )
      END,
      'closed', CASE
        WHEN GeometryType(geom) IN ('LINESTRING', 'CIRCULARSTRING', 'COMPOUNDCURVE')
          THEN ST_IsClosed(geom)
      END,
      'vertices', CASE
        WHEN root_type IN ('LINESTRING', 'POLYGON', 'TRIANGLE')
          AND total_points <= 64
        THEN (
          SELECT json_agg(json_build_array(ST_X(vertex.geom), -ST_Y(vertex.geom)))
          FROM ST_DumpPoints(geom) AS vertex
        )
      END,
      'direction_paths', CASE
        WHEN GeometryType(render_geom) IN ('POLYGON', 'TRIANGLE')
          AND total_points <= 128
        THEN (
          SELECT json_agg(points ORDER BY ring_num)
          FROM (
            SELECT COALESCE(vertex.path[1], 1) AS ring_num,
              json_agg(
                json_build_array(ST_X(vertex.geom), -ST_Y(vertex.geom))
                ORDER BY vertex.path
              ) AS points
            FROM ST_DumpPoints(render_geom) AS vertex
            GROUP BY COALESCE(vertex.path[1], 1)
          ) AS rings
        )
      END,
      'has_z', ST_Zmflag(geom) IN (2, 3),
      'points_xyz', CASE
        WHEN GeometryType(render_geom) IN (
          'POINT', 'LINESTRING', 'POLYGON', 'TRIANGLE',
          'TIN', 'POLYHEDRALSURFACE'
        )
        THEN (
          SELECT json_agg(
            json_build_object(
              'path', array_to_json(vertex3d.path),
              'point', json_build_array(
                ST_X(vertex3d.geom), ST_Y(vertex3d.geom),
                COALESCE(ST_Z(vertex3d.geom), 0)
              )
            ) ORDER BY vertex3d.path
          )
          FROM ST_DumpPoints(render_geom) AS vertex3d
        )
      END
    ) ORDER BY ord)
    FROM render_parts
  )
)::text
"""
        payload = self.run_psql_scalar(database, query)
        if not payload:
            raise RuntimeError("PostGIS returned no visual-example payload")
        return json.loads(payload)

    def visual_image_svg(self, visual_id, images):
        count = len(images)
        columns = 1 if count == 1 else min(2, count)
        rows = math.ceil(count / columns)
        gap = 14
        outer = 20
        label_height = 24 if count > 1 else 0
        cell_width = (560 - gap * (columns - 1)) / columns
        cell_height = 520 if count == 1 else 320
        available_scale = min(
            min(cell_width / image["width"], cell_height / image["height"])
            for image in images
        )
        shared_scale = (
            max(1, math.floor(available_scale))
            if available_scale >= 1 else available_scale
        )
        svg_height = outer + rows * (label_height + cell_height) + (rows - 1) * gap + 44
        panels = []
        legends = []
        source_indexes = {"Code": 0, "Output": 0}
        for index, image in enumerate(images):
            column = index % columns
            row = index // columns
            left = outer + column * (cell_width + gap)
            top = outer + row * (label_height + cell_height + gap)
            image_top = top + label_height
            width = image["width"] * shared_scale
            height = image["height"] * shared_scale
            x = left + (cell_width - width) / 2
            y = image_top + (cell_height - height) / 2
            source_indexes[image["source"]] += 1
            layer_id = (
                f"{visual_id}-{image['source'].lower()}-"
                f"{source_indexes[image['source']]}"
            )
            label = html.escape(image["label"])
            if count > 1:
                panels.append(
                    f'<text class="frame-label" x="{left + cell_width / 2:.12g}" '
                    f'y="{top + 16:.12g}" text-anchor="middle">{label}</text>'
                )
            panels.append(
                f'<rect class="image-background" x="{x:.12g}" y="{y:.12g}" '
                f'width="{width:.12g}" height="{height:.12g}" fill="white"/>'
                f'<g class="geometry-layer image-layer" data-postgis-geometry-id="{layer_id}">'
                f'<image x="{x:.12g}" y="{y:.12g}" width="{width:.12g}" '
                f'height="{height:.12g}" preserveAspectRatio="none" '
                'style="image-rendering:crisp-edges;image-rendering:pixelated" '
                f'href="data:{image["media_type"]};base64,{image["data"]}"/>'
                '</g>'
            )
            if count == 1:
                legends.append(
                    f'<g class="legend-row" data-postgis-geometry-id="{layer_id}" '
                    f'transform="translate(20 {svg_height - 26})">'
                    '<rect width="11" height="11" rx="2" fill="#a62c2b"/>'
                    f'<text x="16" y="10">{label}</text></g>'
                )
        safe_id = html.escape(visual_id, quote=True)
        return (
            '<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" width="600" height="{svg_height}" '
            f'viewBox="0 0 600 {svg_height}" role="img" aria-labelledby="{safe_id}-title">\n'
            f'<title id="{safe_id}-title">Rendered SQL image output for {safe_id}</title>\n'
            '<style>.frame-label{font-family:sans-serif;font-size:12px;font-weight:600;fill:#344}'
            '.geometry-layer{opacity:1;transition:opacity 90ms ease}'
            '.image-layer image{image-rendering:crisp-edges;image-rendering:pixelated}'
            '.legend-row text{font-family:sans-serif;font-size:12px;fill:#344}'
            'svg:has(.geometry-layer.active) .geometry-layer:not(.active){opacity:.18}'
            '.geometry-layer.active{filter:brightness(.72)}</style>\n'
            + ''.join(panels) + '\n' + ''.join(legends) + '\n</svg>\n'
        )

    def render_image_visual_example(self, example, actual):
        layers = []
        source_indexes = {"Code": 0, "Output": 0}
        for image in actual.visuals:
            source = image["source"]
            source_indexes[source] += 1
            layers.append({
                "id": f"{example['visual_id']}-{source.lower()}-{source_indexes[source]}",
                "source": source, "label": image["label"], "row": image["row"],
                "column": image["column"], "media_type": image["media_type"],
                "width": image["width"], "height": image["height"], "frame": image["label"],
            })
        return {
            "id": example["visual_id"], "source": example["label"],
            "refentry": example["visual_refentry"], "screen": example["visual_screen"],
            "preferred": example["visual_preferred"], "kind": "image-output",
            "output_omitted": False, "layers": layers,
            "svg": self.visual_image_svg(example["visual_id"], actual.visuals),
        }

    def render_visual_example(self, database, example, actual):
        actual = self.coalesce_single_column_wkt_rows(actual)
        if getattr(actual, "visuals", None):
            return self.render_image_visual_example(example, actual)
        code = (
            []
            if example.get("visual_output_only")
            else self.code_geometry_candidates(example["query"])
        )
        output = []
        headers = getattr(actual, "headers", []) or self.query_output_headers(example["query"])
        explicit_headers = self.query_output_headers(example["query"])
        output_types = getattr(actual, "types", [])
        cluster_column = self.cluster_column_index(headers) if len(actual) > 1 else None
        marker_scale_column = self.marker_scale_column_index(headers)
        row_panel_label_column = (
            self.label_companion_column_index(headers)
            if example.get("visual_row_panels")
            else None
        )
        output_groups = {}
        next_output_group = 1
        for row_index, row in enumerate(actual, 1):
            cluster_value = self.scalar_companion_value(row, cluster_column)
            marker_scale = self.marker_scale_value(
                self.scalar_companion_value(row, marker_scale_column)
            )
            for column_index, value in enumerate(row):
                matches = [{"wkt": wkt} for wkt in self.geometry_candidates(value)]
                output_type = output_types[column_index] if column_index < len(output_types) else ""
                hexwkb = self.geometry_output_hex(value, output_type)
                if hexwkb is not None:
                    matches.append({"hexwkb": hexwkb["hexwkb"]})
                for match_index, geometry in enumerate(matches, 1):
                    header_label = headers[column_index] if column_index < len(headers) else ""
                    label = header_label
                    has_explicit_header = (
                        column_index < len(explicit_headers) and bool(explicit_headers[column_index])
                    )
                    if (
                        not label
                        or label.lower() in {"?column?", "st_astext", "st_asewkt"}
                        or ("hexwkb" in geometry and not has_explicit_header)
                    ):
                        label = "Output"
                    source = "Code" if label.lower().startswith("input_") else "Output"
                    if source == "Code":
                        label = label[len("input_"):]
                    row_panel_label = self.scalar_companion_value(row, row_panel_label_column)
                    if row_panel_label and column_index != row_panel_label_column:
                        label = row_panel_label
                    label = self.display_label(label)
                    if len(actual) > 1 and not row_panel_label:
                        label = f"{label} {row_index}"
                    if len(matches) > 1:
                        label = f"{label}.{match_index}"
                    group_key = None
                    group_label = None
                    if (
                        source == "Output"
                        and cluster_value is not None
                        and column_index != cluster_column
                    ):
                        label_column = self.label_companion_column_index(
                            headers,
                            excluded_indexes={cluster_column, column_index},
                        )
                        label_value = self.scalar_companion_value(row, label_column)
                        group_key = (column_index + 1, cluster_value)
                        if group_key not in output_groups:
                            output_groups[group_key] = next_output_group
                            next_output_group += 1
                        group_label = self.grouped_geometry_label(
                            headers[cluster_column] if cluster_column is not None else "",
                            cluster_value,
                            label_value,
                        )
                    output.append({
                        **geometry, "label": label, "source": source,
                        "row_num": row_index, "column_num": column_index + 1,
                        "group_ord": output_groups.get(group_key),
                        "group_label": group_label,
                        "marker_scale": marker_scale,
                    })
        if (
            example.get("visual_kind") == "geometry-output"
            and self.expected_has_hexwkb(example.get("expected", []))
            and not output
        ):
            return None
        if any(candidate["source"] == "Code" for candidate in output):
            # Explicit input_* result columns define the authored input layers.
            # Do not repeat geometry literals and constructor arguments inferred
            # from the query, but retain them for ordinary named output columns.
            code = []
        layers = []
        forced_frame = "Overlay" if example.get("visual_overlay") else None
        code_geometry_count = sum("label" not in candidate for candidate in code)
        code_point_count = sum(
            self.geometry_type(candidate.get("wkt") or "") in POINT_TYPES
            for candidate in code
        )
        output_point_counts = {"Code": 0, "Output": 0}
        for candidate in output:
            geometry_value = candidate.get("wkt") or candidate.get("hexwkb") or ""
            geometry_type = self.geometry_type(geometry_value)
            if geometry_type is None and candidate.get("hexwkb"):
                geometry_type = self.hexwkb_geometry(
                    candidate["hexwkb"], allow_bytea_prefix=False
                )["type"]
            if geometry_type in POINT_TYPES:
                output_point_counts[candidate["source"]] += 1
        for index, candidate in enumerate(code, 1):
            if candidate.get("label"):
                label = self.display_label(candidate["label"])
            elif code_geometry_count == 1:
                label = "Code"
            else:
                code_index = sum("label" not in previous for previous in code[:index])
                label = f"Code {code_index}"
            layers.append({
                "ord": len(layers) + 1,
                "source": "Code",
                "label": label,
                "row_num": None,
                "column_num": None,
                "requested_frame": forced_frame or (
                    "Code" if example.get("visual_separate_output") else None
                ),
                "source_point_count": code_point_count,
                # Let PostGIS compute ST_NPoints for the actual geometry.  A
                # MULTIPOINT is one layer, but may contain hundreds of points.
                "total_points": None,
                "marker_scale": None,
                "wkt": candidate["wkt"],
                "hexwkb": None,
            })
        for source in ("Code", "Output"):
            values = [candidate for candidate in output if candidate["source"] == source]
            for index, candidate in enumerate(values, 1):
                meaningful_label = candidate["label"] != source and (len(output) > 1 or bool(code))
                layers.append({
                    "ord": (
                        len(code) + candidate["group_ord"]
                        if source == "Output" and candidate.get("group_ord")
                        else len(layers) + 1
                    ),
                    "source": source,
                    "label": candidate.get("group_label") or (
                        candidate["label"] if meaningful_label else source if len(values) == 1 else (
                            candidate["label"] if candidate["label"] != source else f"{source} {index}"
                        )
                    ),
                    "row_num": candidate["row_num"],
                    "column_num": candidate["column_num"],
                    "requested_frame": forced_frame or (
                        candidate["label"]
                        if example.get("visual_separate_output") or example.get("visual_row_panels")
                        else None
                    ),
                    "source_point_count": output_point_counts.get(source, 0),
                    "total_points": None,
                    "marker_scale": candidate.get("marker_scale"),
                    "wkt": candidate.get("wkt"),
                    "hexwkb": candidate.get("hexwkb"),
                })
        if not layers:
            raise RuntimeError(f"Visual example {example['visual_id']} has no geometry input or output")
        output_omitted = False
        try:
            payload = self.visual_payload(database, layers)
        except RuntimeError as exc:
            parse_failure = re.search(
                r"geometry (?:contains non-closed rings|requires more points)|parse error",
                str(exc),
                re.I,
            )
            if not output or not code or not parse_failure:
                raise
            layers = [layer for layer in layers if layer["source"] == "Code"]
            payload = self.visual_payload(database, layers)
            output_omitted = True
        if not payload.get("parts"):
            return None
        if (
            any(layer["source"] == "Output" for layer in layers)
            and not any(part["source"] == "Output" for part in payload["parts"])
        ):
            output_omitted = True
        rendered_layers = []
        layer_source_indexes = {"Code": 0, "Output": 0}
        parts_by_ord = {}
        for part in payload.get("parts") or []:
            parts_by_ord.setdefault(part["ord"], []).append(part)
        for ordinal, parts in sorted(parts_by_ord.items()):
            first = parts[0]
            source_index = layer_source_indexes[first["source"]]
            layer_source_indexes[first["source"]] += 1
            closed_values = [part.get("closed") for part in parts if part.get("closed") is not None]
            rendered_layers.append({
                "id": f"{example['visual_id']}-{first['source'].lower()}-{source_index + 1}",
                "source": first["source"],
                "label": first["label"],
                "row": first.get("row"),
                "column": first.get("column"),
                "dimension": first.get("dimension"),
                "closed": all(closed_values) if closed_values else None,
                "srid": first.get("srid"),
                "frame": first.get("frame", "Overlay"),
            })
        return {
            "id": example["visual_id"],
            "source": example["label"],
            "refentry": example["visual_refentry"],
            "screen": example["visual_screen"],
            "preferred": example["visual_preferred"] and not output_omitted,
            "kind": "input-context-fallback" if output_omitted else example["visual_kind"],
            "output_omitted": output_omitted,
            "native_output": (
                self.rows_to_string(actual)
                if self.expected_has_geometry_text(example.get("expected", []))
                and any(
                    GEOMETRY_OUTPUT_TYPE_RE.fullmatch(output_type or "")
                    for output_type in getattr(actual, "types", [])
                )
                else None
            ),
            "layers": rendered_layers,
            "svg": self.visual_svg(
                example["visual_id"], payload,
                show_direction=example.get("visual_direction", False),
                hide_output_area_vertices=example.get(
                    "visual_hide_output_area_vertices", False
                ),
                matrix_3x3=example.get("visual_matrix_3x3", False),
            ),
        }

    def grid_step(self, span, target_lines=8):
        if span <= 0:
            return 1
        rough = span / target_lines
        power = 10 ** math.floor(math.log10(rough))
        normalized = rough / power
        nice = 1 if normalized <= 1 else 2 if normalized <= 2 else 5 if normalized <= 5 else 10
        return nice * power

    def grid_label(self, value):
        if abs(value) < 1e-12:
            value = 0
        label = f"{value:.12g}"
        return "0" if label == "-0" else label

    DEFAULT_3D_VIEW = (-1.35, -1.0, 0.8)
    CANDIDATE_3D_VIEWS = (
        DEFAULT_3D_VIEW,
        (-0.45, -1.6, 0.85),
        (1.2, -1.1, 0.85),
        (1.45, 0.65, 0.85),
        (-1.0, 1.35, 0.85),
        (0.2, -1.5, 1.15),
        (-1.5, 0.15, 1.15),
        (1.5, -0.2, 1.15),
    )

    def project_3d_point(self, point, view=None):
        """Return deterministic orthographic x/y/depth coordinates for one XYZ point."""
        x, y, z = [float(value) for value in point]
        # Slightly elevated, asymmetric cameras avoid collapsing opposite
        # corners of a cube while retaining a stable orthographic projection.
        view = self.DEFAULT_3D_VIEW if view is None else view
        view_length = math.sqrt(sum(value ** 2 for value in view))
        view = tuple(value / view_length for value in view)
        horizontal_length = math.hypot(view[0], view[1])
        right = (-view[1] / horizontal_length, view[0] / horizontal_length, 0.0)
        up = (
            view[1] * right[2] - view[2] * right[1],
            view[2] * right[0] - view[0] * right[2],
            view[0] * right[1] - view[1] * right[0],
        )
        projected_x = x * right[0] + y * right[1] + z * right[2]
        projected_y = x * up[0] + y * up[1] + z * up[2]
        depth = x * view[0] + y * view[1] + z * view[2]
        return projected_x, projected_y, depth

    def choose_3d_view(self, points):
        """Choose a deterministic 3D view that avoids edge-on projections."""
        if len(points) < 2:
            return self.DEFAULT_3D_VIEW
        xs = [float(point[0]) for point in points]
        ys = [float(point[1]) for point in points]
        zs = [float(point[2]) for point in points]
        source_width = max(xs) - min(xs)
        source_height = max(ys) - min(ys)
        source_depth = max(zs) - min(zs)
        wide_scene = source_width > max(source_height, source_depth, 1e-9) * 2.2
        best = None
        for view_index, view in enumerate(self.CANDIDATE_3D_VIEWS):
            projected = [self.project_3d_point(point, view) for point in points]
            xs = [point[0] for point in projected]
            ys = [point[1] for point in projected]
            width = max(xs) - min(xs)
            height = max(ys) - min(ys)
            smaller_span = min(width, height)
            larger_span = max(width, height)
            balance = smaller_span / larger_span if larger_span else 0
            if wide_scene:
                origin = self.project_3d_point((0, 0, 0), view)
                x_axis = self.project_3d_point((1, 0, 0), view)
                dx = x_axis[0] - origin[0]
                dy = x_axis[1] - origin[1]
                horizontalness = abs(dx) / math.hypot(dx, dy) if dx or dy else 0
                # Long text-like 3D geometries are easier to read when their
                # source X axis remains left-to-right and nearly horizontal.
                # A purely bbox-driven camera can make these examples look
                # mirrored or rotated even though the solid itself is valid.
                score = (dx > 0, horizontalness, width, smaller_span, -view_index)
            else:
                score = (smaller_span, balance, -view_index)
            if best is None or score > best[0]:
                best = (score, view)
        return best[1]

    def face_shade(self, rings):
        """Return an orientation-independent light factor for a polygon face."""
        points = rings[0] if rings else []
        if len(points) > 1 and points[0] == points[-1]:
            points = points[:-1]
        if len(points) < 3:
            return 0.72
        normal_x = normal_y = normal_z = 0.0
        for index, current in enumerate(points):
            following = points[(index + 1) % len(points)]
            x1, y1, z1 = [float(value) for value in current]
            x2, y2, z2 = [float(value) for value in following]
            normal_x += (y1 - y2) * (z1 + z2)
            normal_y += (z1 - z2) * (x1 + x2)
            normal_z += (x1 - x2) * (y1 + y2)
        length = math.sqrt(normal_x ** 2 + normal_y ** 2 + normal_z ** 2)
        if length == 0:
            return 0.72
        light = (0.25, -0.45, 0.86)
        light_length = math.sqrt(sum(value ** 2 for value in light))
        incidence = abs(
            (normal_x * light[0] + normal_y * light[1] + normal_z * light[2])
            / (length * light_length)
        )
        return 0.48 + 0.52 * incidence

    def shade_color(self, color, factor):
        channels = [int(color[index:index + 2], 16) for index in (1, 3, 5)]
        return "#" + "".join(f"{max(0, min(255, round(channel * factor))):02x}" for channel in channels)

    def xyz_paths(self, part):
        """Rebuild deterministic point/line/ring paths from ST_DumpPoints paths."""
        paths = {}
        for vertex in part.get("points_xyz") or []:
            path = tuple(vertex.get("path") or [])
            parent_path = path[:-1] if len(path) > 1 else ()
            paths.setdefault(parent_path, []).append(vertex["point"])
        return list(paths.values())

    def project_3d_payload(self, payload):
        """Project every frame consistently when the visual contains a Z surface."""
        frames = payload.get("frames") or [{"id": "Overlay", "bounds": payload["bounds"]}]
        area_types = {
            "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
            "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
        }
        parts = payload.get("parts") or []
        three_dimensional_frames = {
            str(part.get("frame", frames[0]["id"])) for part in parts
            if part.get("has_z") and part.get("points_xyz")
        }
        projected_bounds = {}
        projected_parts = []
        frame_views = {}
        shared_3d_view = None
        if len(three_dimensional_frames) > 1:
            shared_points = [
                point
                for part in parts
                if str(part.get("frame", frames[0]["id"])) in three_dimensional_frames
                for vertex in (part.get("points_xyz") or [])
                for point in [vertex["point"]]
            ]
            shared_3d_view = self.choose_3d_view(shared_points)
        for frame in frames:
            frame_id = str(frame["id"])
            if frame_id not in three_dimensional_frames:
                continue
            if shared_3d_view is not None:
                frame_views[frame_id] = shared_3d_view
            else:
                frame_points = [
                    point
                    for part in parts
                    if str(part.get("frame", frames[0]["id"])) == frame_id
                    for vertex in (part.get("points_xyz") or [])
                    for point in [vertex["point"]]
                ]
                frame_views[frame_id] = self.choose_3d_view(frame_points)
        for original in parts:
            frame_id = str(original.get("frame", frames[0]["id"]))
            paths = self.xyz_paths(original)
            if frame_id not in three_dimensional_frames or not paths:
                projected_parts.append(original)
                continue
            projected_paths = []
            depths = []
            for path in paths:
                projected_path = []
                for point in path:
                    projected_x, projected_y, depth = self.project_3d_point(
                        point, frame_views.get(frame_id)
                    )
                    projected_path.append((projected_x, projected_y))
                    depths.append(depth)
                if projected_path:
                    projected_paths.append(projected_path)
            if not projected_paths:
                projected_parts.append(original)
                continue
            bounds = projected_bounds.setdefault(frame_id, [math.inf, math.inf, -math.inf, -math.inf])
            for path in projected_paths:
                for x, y in path:
                    bounds[0] = min(bounds[0], x)
                    bounds[1] = min(bounds[1], y)
                    bounds[2] = max(bounds[2], x)
                    bounds[3] = max(bounds[3], y)
            projected = dict(original)
            geometry_type = original.get("type", "").upper()
            if geometry_type in POINT_TYPES:
                projected_x, projected_y = projected_paths[0][0]
                projected.update({"x": projected_x, "y": -projected_y, "vertices": []})
            else:
                close_paths = geometry_type in area_types
                projected["svg"] = " ".join(
                    "M " + " L ".join(f"{x:.12g} {-y:.12g}" for x, y in path)
                    + (" Z" if close_paths else "")
                    for path in projected_paths
                )
                if close_paths:
                    projected["direction_paths"] = [
                        [[x, -y] for x, y in path] for path in projected_paths
                    ]
                projected["vertices"] = (
                    [] if close_paths or geometry_type in {"CIRCULARSTRING", "COMPOUNDCURVE"}
                    else [[x, -y] for x, y in projected_paths[0]]
                )
            if geometry_type in area_types:
                projected.update({
                    "is_3d_face": True,
                    "depth": sum(depths) / len(depths),
                    "shade": self.face_shade(paths),
                })
            projected_parts.append(projected)

        projected_frames = []
        if len(projected_bounds) > 1:
            shared_bounds = [
                min(bounds[0] for bounds in projected_bounds.values()),
                min(bounds[1] for bounds in projected_bounds.values()),
                max(bounds[2] for bounds in projected_bounds.values()),
                max(bounds[3] for bounds in projected_bounds.values()),
            ]
            projected_bounds = {
                frame_id: list(shared_bounds)
                for frame_id in projected_bounds
            }
        for frame in frames:
            projected_frame = dict(frame)
            frame_id = str(frame["id"])
            if frame_id in projected_bounds:
                projected_frame["bounds"] = projected_bounds[frame_id]
                projected_frame["three_dimensional"] = True
            projected_frames.append(projected_frame)
        projected_payload = dict(payload)
        projected_payload["frames"] = projected_frames
        projected_payload["parts"] = projected_parts
        return projected_payload

    def visual_svg(
        self, visual_id, payload, show_direction=False,
        hide_output_area_vertices=False,
        matrix_3x3=False,
    ):
        if any(
            part.get("has_z") and part.get("points_xyz")
            and part.get("type", "").upper() in {
                "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
                "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
            }
            for part in payload.get("parts") or []
        ):
            payload = self.project_3d_payload(payload)
        frames = payload.get("frames") or [{
            "id": "Overlay",
            "bounds": payload["bounds"],
        }]
        frame_count = len(frames)
        show_frame_labels = frame_count > 1 or any(
            frame.get("three_dimensional") for frame in frames
        )
        frame_rows = None
        if matrix_3x3 and frame_count > 9:
            sources_by_frame = {}
            for part in payload.get("parts") or []:
                frame_id = str(part.get("frame", frames[0]["id"]))
                sources_by_frame.setdefault(frame_id, set()).add(part.get("source"))
            code_frame_ids = [
                str(frame["id"])
                for frame in frames
                if "Code" in sources_by_frame.get(str(frame["id"]), set())
            ]
            output_frame_ids = [
                str(frame["id"])
                for frame in frames
                if "Output" in sources_by_frame.get(str(frame["id"]), set())
            ]
            if len(output_frame_ids) == 9:
                frame_rows = []
                for index in range(0, len(code_frame_ids), 3):
                    frame_rows.append(code_frame_ids[index:index + 3])
                frame_rows.extend([
                    output_frame_ids[index:index + 3]
                    for index in range(0, len(output_frame_ids), 3)
                ])
        if frame_rows:
            panel_columns = max(len(row) for row in frame_rows)
            panel_rows = len(frame_rows)
            frame_positions = {
                frame_id: (row_index, column_index, len(row))
                for row_index, row in enumerate(frame_rows)
                for column_index, frame_id in enumerate(row)
            }
        elif frame_count == 1:
            panel_columns = 1
            panel_rows = 1
            frame_positions = None
        elif frame_count == 3:
            panel_columns = 3
            panel_rows = 1
            frame_positions = None
        elif frame_count == 9:
            panel_columns = 3
            panel_rows = 3
            frame_positions = None
        else:
            panel_columns = min(2, frame_count)
            panel_rows = math.ceil(frame_count / panel_columns)
            frame_positions = None
        panel_gap = 20 if frame_count > 1 else 0
        svg_width = 900 if frame_count == 3 or frame_rows else 600
        plot_width = svg_width - 40
        panel_width = (plot_width - panel_gap * (panel_columns - 1)) / panel_columns
        plot_top = 30 if show_frame_labels else 12
        panel_height = 322 - plot_top if panel_rows == 1 else 220
        plot_bottom = plot_top + panel_rows * panel_height + panel_gap * (panel_rows - 1)
        layouts = {}
        backgrounds = []
        frame_labels = []
        grid = []
        for frame_index, frame in enumerate(frames):
            frame_id = str(frame["id"])
            min_x, min_y, max_x, max_y = [float(value) for value in frame["bounds"]]
            width = max_x - min_x
            height = max_y - min_y
            fallback = max(width, height, 1.0)
            if width == 0:
                min_x -= fallback / 2
                max_x += fallback / 2
            if height == 0:
                min_y -= fallback / 2
                max_y += fallback / 2
            width = max_x - min_x
            height = max_y - min_y
            min_x -= width * 0.08
            max_x += width * 0.08
            min_y -= height * 0.08
            max_y += height * 0.08
            if frame_positions and frame_id in frame_positions:
                panel_row, panel_column, panels_in_row = frame_positions[frame_id]
            else:
                panel_row = frame_index // panel_columns
                panel_column = frame_index % panel_columns
                panels_in_row = min(panel_columns, frame_count - panel_row * panel_columns)
            row_width = panels_in_row * panel_width + (panels_in_row - 1) * panel_gap
            row_left = 20 + (plot_width - row_width) / 2
            panel_left = row_left + panel_column * (panel_width + panel_gap)
            panel_right = panel_left + panel_width
            panel_top = plot_top + panel_row * (panel_height + panel_gap)
            panel_bottom = panel_top + panel_height
            scale = min(panel_width / (max_x - min_x), panel_height / (max_y - min_y))
            used_width = (max_x - min_x) * scale
            used_height = (max_y - min_y) * scale
            left = panel_left + (panel_width - used_width) / 2
            top = panel_top + (panel_height - used_height) / 2
            translate_x = left - min_x * scale
            translate_y = top + max_y * scale
            layouts[frame_id] = {
                "scale": scale,
                "translate_x": translate_x,
                "translate_y": translate_y,
            }
            backgrounds.append(
                f'<rect class="plot-background" x="{panel_left:.12g}" y="{panel_top}" '
                f'width="{panel_width:.12g}" height="{panel_height}" fill="#fbfcfd"/>'
            )
            if show_frame_labels:
                frame_label = str(frame.get("label", frame_id))
                if frame.get("three_dimensional"):
                    frame_label = (
                        "3D view" if frame_count == 1 or frame_label == "Overlay"
                        else f"{frame_label} (3D view)"
                    )
                frame_labels.append(
                    f'<text class="frame-label" x="{(panel_left + panel_right) / 2:.12g}" y="{panel_top - 10}" '
                    f'text-anchor="middle">{html.escape(frame_label)}</text>'
                )

            if frame.get("three_dimensional"):
                continue
            step = self.grid_step(max(max_x - min_x, max_y - min_y))
            first_x = math.ceil(min_x / step) * step
            first_y = math.ceil(min_y / step) * step
            value = first_x
            for _ in range(32):
                if value > max_x + step * 1e-9:
                    break
                x = translate_x + value * scale
                grid.append(
                    f'<line x1="{x:.12g}" y1="{panel_top}" x2="{x:.12g}" y2="{panel_bottom}" '
                    'stroke="#dce2e7" stroke-width="1"/>'
                )
                if panel_left + 18 <= x <= panel_right - 18:
                    grid.append(
                        f'<text class="grid-label grid-label-x" x="{x:.12g}" '
                        f'y="{panel_bottom - 5:.12g}" text-anchor="middle">'
                        f'{html.escape(self.grid_label(value))}</text>'
                    )
                next_value = value + step
                if next_value <= value:
                    break
                value = next_value
            value = first_y
            for _ in range(32):
                if value > max_y + step * 1e-9:
                    break
                y = translate_y - value * scale
                grid.append(
                    f'<line x1="{panel_left:.12g}" y1="{y:.12g}" '
                    f'x2="{panel_right:.12g}" y2="{y:.12g}" '
                    'stroke="#dce2e7" stroke-width="1"/>'
                )
                if panel_top + 12 <= y <= panel_bottom - 16:
                    grid.append(
                        f'<text class="grid-label grid-label-y" x="{panel_left + 5:.12g}" '
                        f'y="{y - 3:.12g}" text-anchor="start">'
                        f'{html.escape(self.grid_label(value))}</text>'
                    )
                next_value = value + step
                if next_value <= value:
                    break
                value = next_value

        hidden_edge_groups = []
        area_groups = []
        line_groups = []
        point_groups = []
        legend = []
        source_indexes = {"Code": 0, "Output": 0}
        parts_by_ord = {}
        for part in payload.get("parts") or []:
            parts_by_ord.setdefault(part["ord"], []).append(part)
        for ordinal, parts in sorted(parts_by_ord.items()):
            if any(part.get("is_3d_face") for part in parts):
                # Paint larger projected-depth faces first so nearer faces hide
                # the far side of the solid.
                parts.sort(
                    key=lambda part: (-float(part.get("depth", 0)), part.get("svg") or ""),
                )
            source = parts[0]["source"]
            frame_id = str(parts[0].get("frame", frames[0]["id"]))
            layout = layouts[frame_id]
            scale = layout["scale"]
            source_index = source_indexes[source]
            source_indexes[source] += 1
            color = SVG_PALETTES[source][source_index % len(SVG_PALETTES[source])]
            geometry_id = f"{visual_id}-{source.lower()}-{source_index + 1}"
            hidden_edge_shapes = []
            shapes = []
            root_type = parts[0].get("root_type", "").upper()
            multipart_areas = len(parts) > 1 and all(part["type"].upper() in {
                "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
                "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
            } for part in parts) and root_type != "POLYHEDRALSURFACE"
            for part_index, part in enumerate(parts):
                part_color = (
                    SVG_PALETTES[source][(source_index + part_index) % len(SVG_PALETTES[source])]
                    if multipart_areas else color
                )
                if str(part.get("dimension")) == "0" or part["type"].upper() in {"POINT", "MULTIPOINT"}:
                    points = part.get("points") or [[part.get("x"), part.get("y")]]
                    density_scale = self.marker_density_scale(
                        part.get("total_points"),
                        part.get("source_point_count"),
                    )
                    marker_scale = max(0.75, min(float(part.get("marker_scale") or 1.0), 2.2))
                    radius = ((6.5 if source == "Code" else 4.5) * density_scale * marker_scale) / scale
                    for point in points:
                        if point[0] is None or point[1] is None:
                            continue
                        point_x = float(point[0])
                        point_y = float(point[1])
                        shapes.append(
                            f'<path class="point" d="M {point_x - radius:.12g} {point_y:.12g} '
                            f'A {radius:.12g} {radius:.12g} 0 1 0 {point_x + radius:.12g} {point_y:.12g} '
                            f'A {radius:.12g} {radius:.12g} 0 1 0 {point_x - radius:.12g} {point_y:.12g} Z" '
                            f'fill="{part_color}" stroke="white" stroke-width="{1.2 / scale:.12g}"/>'
                        )
                else:
                    if not part.get("svg"):
                        raise RuntimeError(
                            f"PostGIS returned an empty SVG path for {visual_id} {part['type']}"
                        )
                    if re.search(r"(?:^|\s)[Aa]\s+-(?:\d|\.)|\b(?:nan|inf)\b", part["svg"], re.I):
                        raise RuntimeError(
                            f"PostGIS returned an invalid SVG path for {visual_id} {part['type']}: {part['svg']}"
                        )
                    svg_data = html.escape(part["svg"], quote=True)
                    dimension_class = "area" if part["type"].upper() in {
                        "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
                        "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
                    } else "line"
                    fill = color if dimension_class == "area" else "none"
                    stroke_color = part_color
                    if part.get("is_3d_face"):
                        part_color = self.shade_color(part_color, float(part["shade"]))
                        # 3D solids need visible face boundaries to read as
                        # faceted surfaces.  Using the fill color as the stroke
                        # makes rounded polyhedral buffers collapse visually
                        # into a flat disk.
                        stroke_color = self.shade_color(part_color, 0.68)
                    stroke_pixels = (
                        5 if source == "Code" and dimension_class == "line"
                        else 1.2 if part.get("is_3d_face")
                        else 1.8 if root_type == "POLYHEDRALSURFACE"
                        else 3
                    )
                    stroke_width = stroke_pixels / scale
                    if part.get("is_3d_face"):
                        hidden_edge_shapes.append(
                            f'<path class="hidden-edge" d="{svg_data}" '
                            f'stroke="{part_color}" fill="none" '
                            f'stroke-width="{0.9 / scale:.12g}" '
                            f'stroke-dasharray="{2.4 / scale:.12g} {2.4 / scale:.12g}" '
                            'stroke-opacity="0.48" fill-rule="evenodd" '
                            'stroke-linecap="round" stroke-linejoin="round" '
                            'pointer-events="none"/>'
                        )
                    face_attributes = (
                        f' data-postgis-face="{part_index + 1}"'
                        f' data-postgis-depth="{float(part["depth"]):.12g}"'
                        if part.get("is_3d_face") else ""
                    )
                    shapes.append(
                        f'<path class="{dimension_class}"{face_attributes} d="{svg_data}" '
                        f'stroke="{stroke_color}" fill="{part_color if dimension_class == "area" else fill}" '
                        f'fill-opacity="{1 if part.get("is_3d_face") else 0.24 if dimension_class == "area" else 0}" stroke-opacity="1" '
                        'fill-rule="evenodd" '
                        f'stroke-width="{stroke_width:.12g}" '
                        'stroke-linecap="round" stroke-linejoin="round"/>'
                    )
                    vertices = part.get("vertices") or []
                    if len(vertices) > 1 and vertices[0] == vertices[-1]:
                        vertices = vertices[:-1]
                    if dimension_class == "line" and not part.get("closed") and len(vertices) >= 2:
                        end_x, end_y = [float(value) for value in vertices[-1]]
                        previous_x, previous_y = [float(value) for value in vertices[-2]]
                        delta_x = end_x - previous_x
                        delta_y = end_y - previous_y
                        length = math.hypot(delta_x, delta_y)
                        if length > 0:
                            arrow_length = (10 if source == "Code" else 8) / scale
                            arrow_half_width = arrow_length * 0.45
                            unit_x = delta_x / length
                            unit_y = delta_y / length
                            base_x = end_x - unit_x * arrow_length
                            base_y = end_y - unit_y * arrow_length
                            perpendicular_x = -unit_y * arrow_half_width
                            perpendicular_y = unit_x * arrow_half_width
                            shapes.append(
                                f'<path class="direction-arrow" d="M {end_x:.12g} {end_y:.12g} '
                                f'L {base_x + perpendicular_x:.12g} {base_y + perpendicular_y:.12g} '
                                f'L {base_x - perpendicular_x:.12g} {base_y - perpendicular_y:.12g} Z" '
                                f'fill="{part_color}" stroke="white" stroke-width="{0.7 / scale:.12g}"/>'
                            )
                    if show_direction and dimension_class == "area":
                        direction_paths = part.get("direction_paths") or [vertices]
                        for direction_path in direction_paths:
                            if len(direction_path) > 1 and direction_path[0] == direction_path[-1]:
                                direction_path = direction_path[:-1]
                            if len(direction_path) < 2:
                                continue
                            for segment_index, (start, end) in enumerate(
                                zip(direction_path, direction_path[1:] + direction_path[:1])
                            ):
                                # Two arrows per ring are enough to make winding visible
                                # without turning small polygons into hedgehogs.  Offset
                                # the first arrow from vertices so it does not disappear
                                # under vertex markers on simple rectangles.
                                arrow_step = max(1, len(direction_path) // 2)
                                arrow_offset = 1 if arrow_step > 1 else 0
                                if segment_index % arrow_step != arrow_offset:
                                    continue
                                start_x, start_y = [float(value) for value in start]
                                end_x, end_y = [float(value) for value in end]
                                delta_x = end_x - start_x
                                delta_y = end_y - start_y
                                length = math.hypot(delta_x, delta_y)
                                if length <= 0:
                                    continue
                                arrow_length = (15 if source == "Code" else 13) / scale
                                arrow_half_width = arrow_length * 0.5
                                unit_x = delta_x / length
                                unit_y = delta_y / length
                                tip_x = start_x + delta_x * 0.5
                                tip_y = start_y + delta_y * 0.5
                                base_x = tip_x - unit_x * arrow_length
                                base_y = tip_y - unit_y * arrow_length
                                perpendicular_x = -unit_y * arrow_half_width
                                perpendicular_y = unit_x * arrow_half_width
                                arrow_path = (
                                    f'M {tip_x:.12g} {tip_y:.12g} '
                                    f'L {base_x + perpendicular_x:.12g} {base_y + perpendicular_y:.12g} '
                                    f'L {base_x - perpendicular_x:.12g} {base_y - perpendicular_y:.12g} Z'
                                )
                                shapes.append(
                                    f'<path class="direction-arrow-halo" '
                                    f'd="{arrow_path}" fill="white" stroke="white" '
                                    f'stroke-width="{2.2 / scale:.12g}"/>'
                                )
                                shapes.append(
                                    f'<path class="direction-arrow ring-direction-arrow" '
                                    f'd="{arrow_path}" fill="{part_color}" stroke="{part_color}" '
                                    f'stroke-width="{0.35 / scale:.12g}"/>'
                                )
                    show_vertices = (
                        source == "Code"
                        or dimension_class == "line"
                        or (
                            len(vertices) <= 16
                            and len(parts) <= 8
                            and not (
                                hide_output_area_vertices
                                and source == "Output"
                                and dimension_class == "area"
                            )
                        )
                    )
                    if show_vertices:
                        vertex_radius = (
                            3 * self.marker_density_scale(
                                part.get("total_points"),
                                part.get("source_point_count"),
                            )
                        ) / scale
                        for x, y in vertices:
                            shapes.append(
                                f'<circle class="vertex" cx="{float(x):.12g}" cy="{float(y):.12g}" '
                                f'r="{vertex_radius:.12g}" fill="{part_color}" stroke="white" '
                                f'stroke-width="{0.8 / scale:.12g}"/>'
                            )
                    if dimension_class == "line" and part.get("closed") and vertices:
                        start_x, start_y = [float(value) for value in vertices[0]]
                        marker_label = html.escape(f"Start vertex for {parts[0]['label']}")
                        shapes.append(
                            f'<g class="start-vertex" data-postgis-start-vertex="true" '
                            f'pointer-events="all">'
                            f'<title>{marker_label}</title>'
                            f'<circle cx="{start_x:.12g}" cy="{start_y:.12g}" '
                            f'r="{7 / scale:.12g}" fill="white" stroke="{part_color}" '
                            f'stroke-width="{2 / scale:.12g}"/>'
                            f'<text x="{start_x:.12g}" y="{start_y + 3.1 / scale:.12g}" '
                            f'text-anchor="middle" font-size="{9 / scale:.12g}" '
                            f'fill="{part_color}" font-family="sans-serif" font-weight="700" '
                            f'pointer-events="none">1</text></g>'
                        )
            hidden_edge_svg = (
                f'<g class="hidden-edge-layer" aria-hidden="true" '
                f'data-postgis-geometry-id="{geometry_id}" '
                f'data-postgis-frame="{html.escape(frame_id, quote=True)}" '
                f'transform="matrix({scale:.12g} 0 0 {scale:.12g} '
                f'{layout["translate_x"]:.12g} {layout["translate_y"]:.12g})">' +
                "".join(hidden_edge_shapes) + "</g>"
                if hidden_edge_shapes else ""
            )
            group_svg = (
                f'<g class="geometry-layer" opacity="1" data-postgis-geometry-id="{geometry_id}" '
                f'data-postgis-frame="{html.escape(frame_id, quote=True)}" '
                f'transform="matrix({scale:.12g} 0 0 {scale:.12g} '
                f'{layout["translate_x"]:.12g} {layout["translate_y"]:.12g})">' +
                "".join(shapes) + "</g>"
            )
            if all(part["type"].upper() in POINT_TYPES for part in parts):
                point_groups.append(group_svg)
            elif all(part["type"].upper() in {
                "POLYGON", "MULTIPOLYGON", "CURVEPOLYGON", "MULTISURFACE",
                "TRIANGLE", "TIN", "POLYHEDRALSURFACE",
            } for part in parts):
                if hidden_edge_svg:
                    hidden_edge_groups.append(hidden_edge_svg)
                area_groups.append(group_svg)
            else:
                line_groups.append(group_svg)
            legend.append((geometry_id, color, parts[0]["label"]))

        legend_svg = []
        cursor_x = 20
        cursor_y = plot_bottom + 32
        for geometry_id, color, label in legend:
            safe_label = html.escape(label)
            item_width = max(72, 34 + len(label) * 7)
            if cursor_x + item_width > svg_width - 20 and cursor_x > 20:
                cursor_x = 20
                cursor_y += 22
            legend_svg.append(
                f'<g class="legend-row" data-postgis-geometry-id="{geometry_id}" transform="translate({cursor_x} {cursor_y})">'
                f'<rect width="11" height="11" rx="2" fill="{color}"/>'
                f'<text x="16" y="10" font-family="sans-serif" font-size="12" fill="#344">{safe_label}</text></g>'
            )
            cursor_x += item_width

        svg_height = cursor_y + 26

        safe_id = html.escape(visual_id, quote=True)
        return (
            '<?xml version="1.0" encoding="UTF-8"?>\n'
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{svg_width}" height="{svg_height}" viewBox="0 0 {svg_width} {svg_height}" role="img" aria-labelledby="{safe_id}-title">\n'
            f'<title id="{safe_id}-title">Input and output geometries for {safe_id}</title>\n'
            '<style>.plot-grid line{stroke:#dce2e7;stroke-width:1}.plot-grid text{font-family:sans-serif;font-size:9px;fill:#667887;fill-opacity:.56;paint-order:stroke;stroke:#fbfcfd;stroke-width:2.5px;stroke-opacity:.76}.frame-label{font-family:sans-serif;font-size:12px;font-weight:600;fill:#344}.geometry-layer{opacity:1;transition:opacity 90ms ease}.line,.area{stroke-linecap:round;stroke-linejoin:round;pointer-events:stroke}.point,.vertex{pointer-events:all}.legend-row{cursor:default}.legend-row text{font-family:sans-serif;font-size:12px;fill:#344}svg:has(.geometry-layer.active) .geometry-layer:not(.active){opacity:.18}.geometry-layer.active{filter:brightness(.72)}</style>\n'
            + "".join(backgrounds) + "\n" + "".join(frame_labels) + "\n"
            '<g class="plot-grid" aria-hidden="true">' + "".join(grid) + '</g>\n'
            + "".join(hidden_edge_groups + area_groups + line_groups + point_groups)
            + '\n' + "".join(legend_svg) + '\n</svg>\n'
        )

    def publish_visual_examples(self, render_dir, visuals):
        destination = Path(render_dir)
        destination.parent.mkdir(parents=True, exist_ok=True)
        temporary = Path(tempfile.mkdtemp(prefix=f".{destination.name}-", dir=destination.parent))
        try:
            manifest = []
            seen = set()
            for visual in visuals:
                if not visual["id"] or visual["id"] in seen:
                    raise RuntimeError(f"Duplicate or missing visual example id: {visual['id']!r}")
                seen.add(visual["id"])
                (temporary / f"{visual['id']}.svg").write_text(visual["svg"], encoding="utf-8")
                manifest_item = {
                    "id": visual["id"],
                    "kind": visual["kind"],
                    "output_omitted": visual.get("output_omitted", False),
                    "preferred": visual["preferred"],
                    "refentry": visual["refentry"],
                    "screen": visual["screen"],
                    "source": visual["source"],
                    "layers": visual.get("layers", []),
                }
                if visual.get("native_output"):
                    manifest_item["native_output"] = visual["native_output"]
                manifest.append(manifest_item)
            (temporary / "manifest.json").write_text(
                json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
            )
            manifest_root = ET.Element("visual-examples")
            for item in manifest:
                visual_node = ET.SubElement(manifest_root, "visual", {
                    key: ("true" if value is True else "false" if value is False else str(value))
                    for key, value in item.items()
                    if key not in {"layers", "native_output"} and value is not None
                })
                if item.get("native_output"):
                    native_output = ET.SubElement(visual_node, "native-output", {
                        "format": "hexewkb",
                    })
                    native_output.text = item["native_output"]
                for layer in item["layers"]:
                    ET.SubElement(visual_node, "layer", {
                        key: ("true" if value is True else "false" if value is False else str(value))
                        for key, value in layer.items()
                        if value is not None
                    })
            ET.ElementTree(manifest_root).write(
                temporary / "manifest.xml", encoding="utf-8", xml_declaration=True
            )
            backup = destination.with_name(f".{destination.name}.old")
            if backup.exists():
                shutil.rmtree(backup)
            if destination.exists():
                destination.rename(backup)
            temporary.rename(destination)
            if backup.exists():
                shutil.rmtree(backup)
        except Exception:
            shutil.rmtree(temporary, ignore_errors=True)
            raise

    def check_environment(self, database):
        for check in ENVIRONMENT_CHECKS:
            try:
                actual = self.run_psql_query(database, check["query"])
            except RuntimeError as exc:
                raise RuntimeError(
                    f"Manual example test environment check failed: {check['label']}\n"
                    f"{check['hint']}\n{exc}"
                ) from exc

            if not self.rows_equal(actual, check["expected"]):
                raise RuntimeError(
                    f"Manual example test environment check failed: {check['label']}\n"
                    f"{check['hint']}\n"
                    f"Expected:\n{self.rows_to_string(check['expected'])}\n"
                    f"Actual:\n{self.rows_to_string(actual)}"
                )

        print(f"manual example test environment checks passed: {len(ENVIRONMENT_CHECKS)}")


def parse_source_example(body, screen_body=None):
    tester = ExampleTester.__new__(ExampleTester)
    if screen_body is not None:
        return tester.parse_adjacent_example(body, screen_body)
    return None, None


def parse_args():
    parser = argparse.ArgumentParser(
        description="Report or run manual example tests.",
        usage="%(prog)s --report|--run|--check-environment [--database DB] [--keep-going] [--render-dir DIR] <postgis-out.xml>",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--report", action="store_true")
    mode.add_argument("--run", action="store_true")
    mode.add_argument("--check-environment", action="store_true")
    parser.add_argument("--database")
    parser.add_argument("--keep-going", action="store_true", help="run all examples before failing")
    parser.add_argument("--render-dir", help="write selected build-time SVG assets after a successful --run")
    parser.add_argument("--visual-only", action="store_true", help="run only selected visual examples")
    parser.add_argument("--jobs", type=int, default=1, help="parallel workers for --visual-only (default: 1)")
    parser.add_argument("xml_file")
    return parser.parse_args()


def main():
    args = parse_args()

    try:
        tester = ExampleTester(args.xml_file)

        if args.report:
            tester.print_report()
        elif args.run:
            if not args.database:
                raise RuntimeError("--database is required with --run")
            tester.run_examples(
                args.database,
                keep_going=args.keep_going,
                render_dir=args.render_dir,
                visual_only=args.visual_only,
                jobs=max(1, args.jobs),
            )
        elif args.check_environment:
            if not args.database:
                raise RuntimeError("--database is required with --check-environment")
            tester.check_environment(args.database)
        return 0
    except Exception as exc:
        print(exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
