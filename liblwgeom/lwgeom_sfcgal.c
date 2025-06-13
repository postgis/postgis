/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2012-2013 Oslandia <infos@oslandia.com>
 *
 **********************************************************************/

#include "lwgeom_sfcgal.h"
#include <stdio.h>

#if POSTGIS_SFCGAL_VERSION >= 20100
#define sfcgal_triangulated_surface_num_triangles(g) sfcgal_triangulated_surface_num_patches((g))
#define sfcgal_triangulated_surface_triangle_n(g, i) sfcgal_triangulated_surface_patch_n((g), (i))
#define sfcgal_triangulated_surface_add_triangle(g, p) sfcgal_triangulated_surface_add_patch((g), (p))
#define sfcgal_polyhedral_surface_num_polygons(g) sfcgal_polyhedral_surface_num_patches((g))
#define sfcgal_polyhedral_surface_polygon_n(g, i) sfcgal_polyhedral_surface_patch_n((g), (i))
#define sfcgal_geometry_collection_num_geometries(g) sfcgal_geometry_num_geometries((g))
#define sfcgal_polyhedral_surface_add_polygon(g, p) sfcgal_polyhedral_surface_add_patch((g), (p))
#endif

static int SFCGAL_type_to_lwgeom_type(sfcgal_geometry_type_t type);
static POINTARRAY *ptarray_from_SFCGAL(const sfcgal_geometry_t *geom, int force3D);
static sfcgal_geometry_t *ptarray_to_SFCGAL(const POINTARRAY *pa, int type);

/* Return SFCGAL version string */
const char *
lwgeom_sfcgal_version()
{
	const char *version = sfcgal_version();
	return version;
}

#define MAX_LENGTH_SFCGAL_FULL_VERSION 50
/* Return SFCGAL full version string */
const char *
lwgeom_sfcgal_full_version()
{
#if POSTGIS_SFCGAL_VERSION >= 10400
	const char *version = sfcgal_full_version();
#else
	char *version = (char *)lwalloc(MAX_LENGTH_SFCGAL_FULL_VERSION);
	snprintf(version,
		 MAX_LENGTH_SFCGAL_FULL_VERSION,
		 "SFCGAL=\"%s\" CGAL=\"Unknown\" Boost=\"Unknown\"",
		 sfcgal_version());
#endif
	return version;
}

/**
 * Map an SFCGAL geometry type to the corresponding PostGIS lwgeom type constant.
 *
 * Returns the lwgeom type constant (e.g., POINTTYPE, LINETYPE, POLYGONTYPE, etc.)
 * for a supported SFCGAL type. For SFCGAL types that have no direct PostGIS
 * counterpart the function returns COLLECTIONTYPE where appropriate (e.g.,
 * MULTISOLID) or maps specialized surface/triangle types to their PostGIS
 * equivalents (POLYHEDRALSURFACETYPE, TINTYPE, TRIANGLETYPE). If the input
 * type is not recognized the function reports an error via lwerror and
 * returns 0.
 */
static int
SFCGAL_type_to_lwgeom_type(sfcgal_geometry_type_t type)
{
	switch (type)
	{
	case SFCGAL_TYPE_POINT:
		return POINTTYPE;

	case SFCGAL_TYPE_LINESTRING:
		return LINETYPE;

	case SFCGAL_TYPE_POLYGON:
		return POLYGONTYPE;

	case SFCGAL_TYPE_MULTIPOINT:
		return MULTIPOINTTYPE;

	case SFCGAL_TYPE_MULTILINESTRING:
		return MULTILINETYPE;

	case SFCGAL_TYPE_MULTIPOLYGON:
		return MULTIPOLYGONTYPE;

	case SFCGAL_TYPE_MULTISOLID:
		return COLLECTIONTYPE; /* Nota: PolyhedralSurface closed inside
					  aim is to use true solid type as soon
					  as available in OGC SFS */

	case SFCGAL_TYPE_GEOMETRYCOLLECTION:
		return COLLECTIONTYPE;

#if 0
	case SFCGAL_TYPE_CIRCULARSTRING:
		return CIRCSTRINGTYPE;

	case SFCGAL_TYPE_COMPOUNDCURVE:
		return COMPOUNDTYPE;

	case SFCGAL_TYPE_CURVEPOLYGON:
		return CURVEPOLYTYPE;

	case SFCGAL_TYPE_MULTICURVE:
		return MULTICURVETYPE;

	case SFCGAL_TYPE_MULTISURFACE:
		return MULTISURFACETYPE;
#endif

	case SFCGAL_TYPE_POLYHEDRALSURFACE:
		return POLYHEDRALSURFACETYPE;

	case SFCGAL_TYPE_TRIANGULATEDSURFACE:
		return TINTYPE;

	case SFCGAL_TYPE_TRIANGLE:
		return TRIANGLETYPE;

#if POSTGIS_SFCGAL_VERSION >= 20300
	case SFCGAL_TYPE_NURBSCURVE:
		return NURBSCURVETYPE;
#endif
	default:
		lwerror("SFCGAL_type_to_lwgeom_type: Unknown Type");
		return 0;
	}
}

/*
 * Return a PostGIS pointarray from a simple SFCGAL geometry:
 * POINT, LINESTRING or TRIANGLE
 *
 * Throw an error on others types
 */
static POINTARRAY *
ptarray_from_SFCGAL(const sfcgal_geometry_t *geom, int want3d)
{
	POINT4D point;
	uint32_t i, npoints;
	POINTARRAY *pa = NULL;
	int is_3d;
	int is_measured = 0;

	assert(geom);

	is_3d = sfcgal_geometry_is_3d(geom);

#if POSTGIS_SFCGAL_VERSION >= 10308
	is_measured = sfcgal_geometry_is_measured(geom);
#endif

	switch (sfcgal_geometry_type_id(geom))
	{
	case SFCGAL_TYPE_POINT: {
		pa = ptarray_construct(want3d, is_measured, 1);
		point.x = sfcgal_point_x(geom);
		point.y = sfcgal_point_y(geom);

		if (is_3d)
			point.z = sfcgal_point_z(geom);
		else if (want3d)
			point.z = 0.0;

#if POSTGIS_SFCGAL_VERSION >= 10308
		if (is_measured)
			point.m = sfcgal_point_m(geom);
#endif

		ptarray_set_point4d(pa, 0, &point);
	}
	break;

	case SFCGAL_TYPE_LINESTRING: {
		npoints = sfcgal_linestring_num_points(geom);
		pa = ptarray_construct(want3d, is_measured, npoints);

		for (i = 0; i < npoints; i++)
		{
			const sfcgal_geometry_t *pt = sfcgal_linestring_point_n(geom, i);
			point.x = sfcgal_point_x(pt);
			point.y = sfcgal_point_y(pt);

			if (is_3d)
				point.z = sfcgal_point_z(pt);
			else if (want3d)
				point.z = 0.0;

#if POSTGIS_SFCGAL_VERSION >= 10308
			if (is_measured)
				point.m = sfcgal_point_m(pt);
#endif

			ptarray_set_point4d(pa, i, &point);
		}
	}
	break;

	case SFCGAL_TYPE_TRIANGLE: {
		pa = ptarray_construct(want3d, is_measured, 4);

		for (i = 0; i < 4; i++)
		{
			const sfcgal_geometry_t *pt = sfcgal_triangle_vertex(geom, (i % 3));
			point.x = sfcgal_point_x(pt);
			point.y = sfcgal_point_y(pt);

			if (is_3d)
				point.z = sfcgal_point_z(pt);
			else if (want3d)
				point.z = 0.0;

#if POSTGIS_SFCGAL_VERSION >= 10308
			if (is_measured)
				point.m = sfcgal_point_m(pt);
#endif

			ptarray_set_point4d(pa, i, &point);
		}
	}
	break;

	/* Other types should not be called directly ... */
	default:
		lwerror("ptarray_from_SFCGAL: Unknown Type");
		break;
	}
	return pa;
}

/**
 * Create a SFCGAL point based on dimensional flags (XY, XYZ, XYM, XYZM).
 */
static sfcgal_geometry_t *
create_sfcgal_point_by_dimensions(double x, double y, double z, double m, int is_3d, int is_measured)
{
#if POSTGIS_SFCGAL_VERSION >= 10500
	if (is_3d && is_measured)
		return sfcgal_point_create_from_xyzm(x, y, z, m);
	else if (is_3d)
		return sfcgal_point_create_from_xyz(x, y, z);
	else if (is_measured)
		return sfcgal_point_create_from_xym(x, y, m);
	else
		return sfcgal_point_create_from_xy(x, y);
#else
	(void)is_measured;
	(void)m;
	if (is_3d)
		return sfcgal_point_create_from_xyz(x, y, z);
	else
		return sfcgal_point_create_from_xy(x, y);
#endif
}

/**
 * Convert a PostGIS POINTARRAY into a new SFCGAL geometry of the requested type.
 *
 * The function inspects POINTARRAY flags to determine dimensionality (Z and M)
 * and creates one of:
 *  - POINTTYPE: a SFCGAL point created from the first point in `pa`.
 *  - LINETYPE: a SFCGAL linestring with one vertex per point in `pa`.
 *  - TRIANGLETYPE: a SFCGAL triangle using the first three points in `pa`.
 *
 * Requirements and behavior:
 *  - `pa` must be non-NULL.
 *  - For TRIANGLETYPE, `pa->npoints` should be >= 3 (first three points are used).
 *  - For POINTTYPE the first point (index 0) is used.
 *  - Dimensionality (2D/3D and measured) is taken from pa->flags and applied to created points.
 *
 * Parameters:
 *  - pa: source POINTARRAY to convert.
 *  - type: desired target geometry type (POINTTYPE, LINETYPE, or TRIANGLETYPE).
 *
 * Returns:
 *  - A newly allocated sfcgal_geometry_t* representing the converted geometry on success.
 *  - NULL if `type` is unsupported or on error.
 *
 * Ownership:
 *  - Caller is responsible for freeing the returned geometry (e.g., with sfcgal_geometry_delete).
 */
static sfcgal_geometry_t *
ptarray_to_SFCGAL(const POINTARRAY *pa, int type)
{
	POINT4D point;
	int is_3d, is_measured;
	uint32_t i;

	assert(pa);

	is_3d = FLAGS_GET_Z(pa->flags) != 0;
	is_measured = FLAGS_GET_M(pa->flags) != 0;

	switch (type)
	{
	case POINTTYPE: {
		getPoint4d_p(pa, 0, &point);
		return create_sfcgal_point_by_dimensions(point.x, point.y, point.z, point.m, is_3d, is_measured);
	}
	break;
	case LINETYPE: {
		sfcgal_geometry_t *line = sfcgal_linestring_create();
		for (i = 0; i < pa->npoints; i++)
		{
			getPoint4d_p(pa, i, &point);
			sfcgal_linestring_add_point(
			    line,
			    create_sfcgal_point_by_dimensions(point.x, point.y, point.z, point.m, is_3d, is_measured));
		}
		return line;
	}
	break;
	case TRIANGLETYPE: {
		sfcgal_geometry_t *triangle = sfcgal_triangle_create();
		for (i = 0; i < 3; i++)
		{
			getPoint4d_p(pa, i, &point);
			sfcgal_geometry_t *vertex =
			    create_sfcgal_point_by_dimensions(point.x, point.y, point.z, point.m, is_3d, is_measured);
			if (vertex)
			{
				sfcgal_triangle_set_vertex(triangle, i, vertex);
				sfcgal_geometry_delete(vertex);
			}
		}
		return triangle;
	}
	break;
	default:
		lwerror("ptarray_to_SFCGAL: Unknown Type");
		return NULL;
	}
}

#if POSTGIS_SFCGAL_VERSION >= 20300
/**
 * Convert a PostGIS LWNURBSCURVE to an SFCGAL NURBS geometry.
 *
 * Builds an SFCGAL NURBS curve from the control points stored in `nurbs`.
 * Dimensionality (XY, XYZ, XYM, XYZM) is inferred from `nurbs->flags`. If
 * `nurbs->knots` is present it is used as the knot vector; otherwise the
 * function selects a uniform knot method. If `nurbs->weights` is present the
 * curve is created as rational. Temporary SFCGAL point objects are allocated
 * during construction and freed before returning.
 *
 * @param nurbs PostGIS NURBS curve to convert. Must be non-NULL and contain at
 *              least one control point.
 * @returns A newly created `sfcgal_geometry_t *` representing the NURBS curve,
 *          or NULL if `nurbs` is NULL, has no control points, or conversion
 *          fails. The caller is responsible for deleting the returned geometry
 *          with `sfcgal_geometry_delete`.
 */
static sfcgal_geometry_t*
lwgeom_nurbs_to_sfcgal_nurbs(const LWNURBSCURVE *nurbs)
{
	sfcgal_geometry_t **points;
	double *weights = NULL;
	double *knots = NULL;
	sfcgal_geometry_t *result;
	uint32_t i;
	POINT4D pt;

	if (!nurbs || !nurbs->points || nurbs->points->npoints == 0)
		return NULL;

	/* Create array of SFCGAL points from control points */
	points = (sfcgal_geometry_t**)lwalloc(sizeof(sfcgal_geometry_t*) * nurbs->points->npoints);

	for (i = 0; i < nurbs->points->npoints; i++)
	{
    if (!getPoint4d_p(nurbs->points, i, &pt)) {
        for (uint32_t j = 0; j < i; j++) sfcgal_geometry_delete(points[j]);
        lwfree(points);
        return NULL;
    }
		if (FLAGS_GET_Z(nurbs->flags) && FLAGS_GET_M(nurbs->flags))
			points[i] = sfcgal_point_create_from_xyzm(pt.x, pt.y, pt.z, pt.m);
		else if (FLAGS_GET_Z(nurbs->flags))
			points[i] = sfcgal_point_create_from_xyz(pt.x, pt.y, pt.z);
		else if (FLAGS_GET_M(nurbs->flags))
			points[i] = sfcgal_point_create_from_xym(pt.x, pt.y, pt.m);
		else
			points[i] = sfcgal_point_create_from_xy(pt.x, pt.y);
	}

	/* Handle weights if present */
	if (nurbs->weights && nurbs->nweights > 0)
	{
		weights = nurbs->weights;
	}

	/* Handle knot vector if present */
	if (nurbs->knots && nurbs->nknots > 0)
	{
		knots = nurbs->knots;
		result = sfcgal_nurbs_curve_create_from_full_data(
			(const sfcgal_geometry_t**)points, weights, nurbs->points->npoints,
			nurbs->degree, knots, nurbs->nknots);
	}
	else if (weights)
	{
		result = sfcgal_nurbs_curve_create_from_points_and_weights(
			(const sfcgal_geometry_t**)points, weights, nurbs->points->npoints,
			nurbs->degree, SFCGAL_KNOT_METHOD_UNIFORM);
	}
	else
	{
		result = sfcgal_nurbs_curve_create_from_points(
			(const sfcgal_geometry_t**)points, nurbs->points->npoints,
			nurbs->degree, SFCGAL_KNOT_METHOD_UNIFORM);
	}

	/* Clean up temporary points */
	for (i = 0; i < nurbs->points->npoints; i++)
	{
		sfcgal_geometry_delete(points[i]);
	}
	lwfree(points);

	return result;
}

/**
 * Convert an SFCGAL NURBS geometry into a PostGIS LWNURBSCURVE.
 *
 * Builds a new LWNURBSCURVE by extracting the NURBS degree, control points,
 * optional rational weights, and optional knot vector from the provided
 * SFCGAL NURBS geometry. Control points are copied into a new POINTARRAY;
 * Z coordinates are preserved when present. SFCGAL does not carry M values
 * so M is not populated. If the SFCGAL NURBS is rational, weights are
 * extracted and passed through to the constructed LWNURBSCURVE. If the
 * SFCGAL NURBS contains knots, the knot vector is copied as well.
 *
 * @param sfcgal_nurbs The source SFCGAL NURBS geometry (may be NULL).
 * @param srid The SRID to assign to the resulting LWNURBSCURVE.
 * @return Newly allocated LWNURBSCURVE on success, or NULL if sfcgal_nurbs
 *         is NULL or conversion fails. Caller takes ownership of the result.
 */
static LWNURBSCURVE*
sfcgal_nurbs_to_lwgeom_nurbs(const sfcgal_geometry_t* sfcgal_nurbs, int srid)
{
	size_t num_points, num_knots, i;
	unsigned int degree;
	POINTARRAY *pa;
	double *weights = NULL;
	double *knots = NULL;
	LWNURBSCURVE *result;
	const sfcgal_geometry_t *pt;
	POINT4D point;
	int has_z = 0, has_m = 0;

	if (!sfcgal_nurbs)
		return NULL;

	num_points = sfcgal_nurbs_curve_num_control_points(sfcgal_nurbs);
	degree = sfcgal_nurbs_curve_degree(sfcgal_nurbs);
	num_knots = sfcgal_nurbs_curve_num_knots(sfcgal_nurbs);

	/* Check if first point has Z or M coordinates */
	if (num_points > 0)
	{
		pt = sfcgal_nurbs_curve_control_point_n(sfcgal_nurbs, 0);
		/* Check dimensionality */
		has_z = (sfcgal_geometry_dimension(pt) >= 3 || sfcgal_geometry_is_3d(pt));
#if POSTGIS_SFCGAL_VERSION >= 10308
		has_m = sfcgal_geometry_is_measured(pt);
#else
		has_m = 0; /* SFCGAL doesn't support M coordinates in older versions */
#endif
	}

	/* Create point array */
	pa = ptarray_construct(has_z, has_m, num_points);

	/* Extract control points */
	for (i = 0; i < num_points; i++)
	{
		pt = sfcgal_nurbs_curve_control_point_n(sfcgal_nurbs, i);

		point.x = sfcgal_point_x(pt);
		point.y = sfcgal_point_y(pt);
		if (has_z) point.z = sfcgal_point_z(pt);
		if (has_m) point.m = sfcgal_point_m(pt);

		ptarray_set_point4d(pa, i, &point);
	}

	/* Extract weights if rational */
	if (sfcgal_nurbs_curve_is_rational(sfcgal_nurbs))
	{
		weights = (double*)lwalloc(sizeof(double) * num_points);
		for (i = 0; i < num_points; i++)
		{
			weights[i] = sfcgal_nurbs_curve_weight_n(sfcgal_nurbs, i);
		}
	}

	/* Extract knot vector */
	if (num_knots > 0)
	{
		knots = (double*)lwalloc(sizeof(double) * num_knots);
		for (i = 0; i < num_knots; i++)
		{
			knots[i] = sfcgal_nurbs_curve_knot_n(sfcgal_nurbs, i);
		}
	}

	result = lwnurbscurve_construct(srid, NULL, degree, pa, weights, knots,
		weights ? num_points : 0, num_knots);

	if (!result) {
	 if (pa) ptarray_free(pa);
	 /* weights/knots freed below */
	}

	/* Free temporary buffers after constructor deep-copies them */
	if (weights) lwfree(weights);
	if (knots) lwfree(knots);

	return result;
}
#endif /* POSTGIS_SFCGAL_VERSION >= 20300 */

/**
 * Convert an SFCGAL geometry to an equivalent PostGIS LWGEOM.
 *
 * Converts the provided sfcgal_geometry_t into a newly constructed LWGEOM
 * representation with the given SRID. The conversion preserves dimensionality
 * unless overridden by force3D (which requests a 3D output). Supports points,
 * linestrings, triangles, polygons, multipoints/multilines/multipolygons/
 * multisolid/geometrycollections, polyhedral surfaces, solids (as closed
 * polyhedral surfaces), triangulated surfaces, and — when available —
 * NURBS curves.
 *
 * @param geom Pointer to the input SFCGAL geometry (must be non-NULL).
 * @param force3D If non-zero, request the resulting LWGEOM be 3D even if the
 *                source geometry is 2D.
 * @param srid  Spatial reference identifier to assign to the returned LWGEOM.
 *
 * @return Pointer to a newly allocated LWGEOM on success, or NULL on error.
 *         For empty input geometries, returns the corresponding empty LWGEOM.
 *         On unsupported or unknown SFCGAL types the function returns NULL
 *         (and reports an error).
 */
LWGEOM *
SFCGAL2LWGEOM(const sfcgal_geometry_t *geom, int force3D, int32_t srid)
{
	uint32_t ngeoms = 0, nshells = 0, nsolids = 0;
	uint32_t i, j, k;
	int want3d;

	assert(geom);

	want3d = force3D || sfcgal_geometry_is_3d(geom);

	switch (sfcgal_geometry_type_id(geom))
	{
	case SFCGAL_TYPE_POINT: {
		if (sfcgal_geometry_is_empty(geom))
			return (LWGEOM *)lwpoint_construct_empty(srid, want3d, 0);

		POINTARRAY *pa = ptarray_from_SFCGAL(geom, want3d);
		return (LWGEOM *)lwpoint_construct(srid, NULL, pa);
	}

	case SFCGAL_TYPE_LINESTRING: {
		if (sfcgal_geometry_is_empty(geom))
			return (LWGEOM *)lwline_construct_empty(srid, want3d, 0);

		POINTARRAY *pa = ptarray_from_SFCGAL(geom, want3d);
		return (LWGEOM *)lwline_construct(srid, NULL, pa);
	}

	case SFCGAL_TYPE_TRIANGLE: {
		if (sfcgal_geometry_is_empty(geom))
			return (LWGEOM *)lwtriangle_construct_empty(srid, want3d, 0);

		POINTARRAY *pa = ptarray_from_SFCGAL(geom, want3d);
		return (LWGEOM *)lwtriangle_construct(srid, NULL, pa);
	}

	case SFCGAL_TYPE_POLYGON: {
		if (sfcgal_geometry_is_empty(geom))
			return (LWGEOM *)lwpoly_construct_empty(srid, want3d, 0);

		uint32_t nrings = sfcgal_polygon_num_interior_rings(geom) + 1;
		POINTARRAY **pa = (POINTARRAY **)lwalloc(sizeof(POINTARRAY *) * nrings);

		pa[0] = ptarray_from_SFCGAL(sfcgal_polygon_exterior_ring(geom), want3d);
		for (i = 1; i < nrings; i++)
			pa[i] = ptarray_from_SFCGAL(sfcgal_polygon_interior_ring_n(geom, i - 1), want3d);

		return (LWGEOM *)lwpoly_construct(srid, NULL, nrings, pa);
	}

	case SFCGAL_TYPE_MULTIPOINT:
	case SFCGAL_TYPE_MULTILINESTRING:
	case SFCGAL_TYPE_MULTIPOLYGON:
	case SFCGAL_TYPE_MULTISOLID:
	case SFCGAL_TYPE_GEOMETRYCOLLECTION: {
		ngeoms = sfcgal_geometry_collection_num_geometries(geom);
		LWGEOM **geoms = NULL;
		if (ngeoms)
		{
			nsolids = 0;
			geoms = (LWGEOM **)lwalloc(sizeof(LWGEOM *) * ngeoms);
			for (i = 0; i < ngeoms; i++)
			{
				const sfcgal_geometry_t *g = sfcgal_geometry_collection_geometry_n(geom, i);
				geoms[i] = SFCGAL2LWGEOM(g, 0, srid);
				if (FLAGS_GET_SOLID(geoms[i]->flags))
					++nsolids;
			}
			geoms = (LWGEOM **)lwrealloc(geoms, sizeof(LWGEOM *) * ngeoms);
		}
		LWGEOM *rgeom = (LWGEOM *)lwcollection_construct(
		    SFCGAL_type_to_lwgeom_type(sfcgal_geometry_type_id(geom)), srid, NULL, ngeoms, geoms);
		if (ngeoms)
		{
			if (ngeoms == nsolids)
				FLAGS_SET_SOLID(rgeom->flags, 1);
			else if (nsolids)
				lwnotice(
				    "SFCGAL2LWGEOM: SOLID in heterogeneous collection will be treated as a POLYHEDRALSURFACETYPE");
		}
		return rgeom;
	}

#if 0
	case SFCGAL_TYPE_CIRCULARSTRING:
	case SFCGAL_TYPE_COMPOUNDCURVE:
	case SFCGAL_TYPE_CURVEPOLYGON:
	case SFCGAL_TYPE_MULTICURVE:
	case SFCGAL_TYPE_MULTISURFACE:
	case SFCGAL_TYPE_CURVE:
	case SFCGAL_TYPE_SURFACE:

	/* TODO curve types handling */
#endif

	case SFCGAL_TYPE_POLYHEDRALSURFACE: {
		ngeoms = sfcgal_polyhedral_surface_num_polygons(geom);

		LWGEOM **geoms = NULL;
		if (ngeoms)
		{
			geoms = (LWGEOM **)lwalloc(sizeof(LWGEOM *) * ngeoms);
			for (i = 0; i < ngeoms; i++)
			{
				const sfcgal_geometry_t *g = sfcgal_polyhedral_surface_polygon_n(geom, i);
				geoms[i] = SFCGAL2LWGEOM(g, 0, srid);
			}
		}
		return (LWGEOM *)lwcollection_construct(POLYHEDRALSURFACETYPE, srid, NULL, ngeoms, geoms);
	}

	/* Solid is map as a closed PolyhedralSurface (for now) */
	case SFCGAL_TYPE_SOLID: {
		nshells = sfcgal_solid_num_shells(geom);

		for (ngeoms = 0, i = 0; i < nshells; i++)
			ngeoms += sfcgal_polyhedral_surface_num_polygons(sfcgal_solid_shell_n(geom, i));

		LWGEOM **geoms = 0;
		if (ngeoms)
		{
			geoms = (LWGEOM **)lwalloc(sizeof(LWGEOM *) * ngeoms);
			for (i = 0, k = 0; i < nshells; i++)
			{
				const sfcgal_geometry_t *shell = sfcgal_solid_shell_n(geom, i);
				ngeoms = sfcgal_polyhedral_surface_num_polygons(shell);

				for (j = 0; j < ngeoms; j++)
				{
					const sfcgal_geometry_t *g = sfcgal_polyhedral_surface_polygon_n(shell, j);
					geoms[k] = SFCGAL2LWGEOM(g, 1, srid);
					k++;
				}
			}
		}
		LWGEOM *rgeom = (LWGEOM *)lwcollection_construct(POLYHEDRALSURFACETYPE, srid, NULL, ngeoms, geoms);
		if (ngeoms)
			FLAGS_SET_SOLID(rgeom->flags, 1);
		return rgeom;
	}

	case SFCGAL_TYPE_TRIANGULATEDSURFACE: {
		ngeoms = sfcgal_triangulated_surface_num_triangles(geom);
		LWGEOM **geoms = NULL;
		if (ngeoms)
		{
			geoms = (LWGEOM **)lwalloc(sizeof(LWGEOM *) * ngeoms);
			for (i = 0; i < ngeoms; i++)
			{
				const sfcgal_geometry_t *g = sfcgal_triangulated_surface_triangle_n(geom, i);
				geoms[i] = SFCGAL2LWGEOM(g, 0, srid);
			}
		}
		return (LWGEOM *)lwcollection_construct(TINTYPE, srid, NULL, ngeoms, geoms);
	}

#if POSTGIS_SFCGAL_VERSION >= 20300
	case SFCGAL_TYPE_NURBSCURVE: {
		LWNURBSCURVE *nurbs = sfcgal_nurbs_to_lwgeom_nurbs(geom, srid);
		if (!nurbs)
			return NULL;
		return (LWGEOM *)nurbs;
	}
#endif

	default:
		lwerror("SFCGAL2LWGEOM: Unknown Type");
		return NULL;
	}
}

/**
 * Convert a PostGIS LWGEOM to an equivalent SFCGAL geometry.
 *
 * Converts the input LWGEOM into a newly allocated sfcgal_geometry_t representing
 * the same geometric object. Empty LWGEOMs are mapped to empty SFCGAL geometries
 * of the corresponding type. Supported LWGEOM types include POINT, LINESTRING,
 * TRIANGLE, POLYGON, MULTI* collections, POLYHEDRALSURFACE, TRIANGULATEDSURFACE
 * and (when compiled with POSTGIS_SFCGAL_VERSION >= 20300) NURBSCURVE.
 *
 * @param geom the input LWGEOM to convert (must be non-NULL)
 * @return a newly allocated sfcgal_geometry_t equivalent to `geom`, or NULL if
 *         the LWGEOM type is unsupported. The caller is responsible for freeing
 *         the returned SFCGAL geometry.
 */
sfcgal_geometry_t *
LWGEOM2SFCGAL(const LWGEOM *geom)
{
	uint32_t i;
	sfcgal_geometry_t *ret_geom = NULL;

	assert(geom);

	switch (geom->type)
	{
	case POINTTYPE: {
		const LWPOINT *lwp = (const LWPOINT *)geom;
		if (lwgeom_is_empty(geom))
			return sfcgal_point_create();

		return ptarray_to_SFCGAL(lwp->point, POINTTYPE);
	}
	break;

	case LINETYPE: {
		const LWLINE *line = (const LWLINE *)geom;
		if (lwgeom_is_empty(geom))
			return sfcgal_linestring_create();

		return ptarray_to_SFCGAL(line->points, LINETYPE);
	}
	break;

	case TRIANGLETYPE: {
		const LWTRIANGLE *triangle = (const LWTRIANGLE *)geom;
		if (lwgeom_is_empty(geom))
			return sfcgal_triangle_create();
		return ptarray_to_SFCGAL(triangle->points, TRIANGLETYPE);
	}
	break;

	case POLYGONTYPE: {
		const LWPOLY *poly = (const LWPOLY *)geom;
		uint32_t nrings = poly->nrings - 1;

		if (lwgeom_is_empty(geom))
			return sfcgal_polygon_create();

		sfcgal_geometry_t *exterior_ring = ptarray_to_SFCGAL(poly->rings[0], LINETYPE);
		ret_geom = sfcgal_polygon_create_from_exterior_ring(exterior_ring);

		for (i = 0; i < nrings; i++)
		{
			sfcgal_geometry_t *ring = ptarray_to_SFCGAL(poly->rings[i + 1], LINETYPE);
			sfcgal_polygon_add_interior_ring(ret_geom, ring);
		}
		return ret_geom;
	}
	break;

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE: {
		if (geom->type == MULTIPOINTTYPE)
			ret_geom = sfcgal_multi_point_create();
		else if (geom->type == MULTILINETYPE)
			ret_geom = sfcgal_multi_linestring_create();
		else if (geom->type == MULTIPOLYGONTYPE)
			ret_geom = sfcgal_multi_polygon_create();
		else
			ret_geom = sfcgal_geometry_collection_create();

		const LWCOLLECTION *lwc = (const LWCOLLECTION *)geom;
		for (i = 0; i < lwc->ngeoms; i++)
		{
			sfcgal_geometry_t *g = LWGEOM2SFCGAL(lwc->geoms[i]);
			sfcgal_geometry_collection_add_geometry(ret_geom, g);
		}

		return ret_geom;
	}
	break;

	case POLYHEDRALSURFACETYPE: {
		const LWPSURFACE *lwp = (const LWPSURFACE *)geom;
		ret_geom = sfcgal_polyhedral_surface_create();

		for (i = 0; i < lwp->ngeoms; i++)
		{
			sfcgal_geometry_t *g = LWGEOM2SFCGAL((const LWGEOM *)lwp->geoms[i]);
			sfcgal_polyhedral_surface_add_polygon(ret_geom, g);
		}
		/* We treat polyhedral surface as the only exterior shell,
		   since we can't distinguish exterior from interior shells ... */
		if (FLAGS_GET_SOLID(lwp->flags))
		{
			return sfcgal_solid_create_from_exterior_shell(ret_geom);
		}

		return ret_geom;
	}
	break;

	case TINTYPE: {
		const LWTIN *lwp = (const LWTIN *)geom;
		ret_geom = sfcgal_triangulated_surface_create();

		for (i = 0; i < lwp->ngeoms; i++)
		{
			sfcgal_geometry_t *g = LWGEOM2SFCGAL((const LWGEOM *)lwp->geoms[i]);
			sfcgal_triangulated_surface_add_triangle(ret_geom, g);
		}

		return ret_geom;
	}
	break;

#if POSTGIS_SFCGAL_VERSION >= 20300
	case NURBSCURVETYPE: {
		const LWNURBSCURVE *nurbs = (const LWNURBSCURVE *)geom;
		return lwgeom_nurbs_to_sfcgal_nurbs(nurbs);
	}
	break;
#endif

	default:
		lwerror("LWGEOM2SFCGAL: Unsupported geometry type %s !", lwtype_name(geom->type));
		return NULL;
	}
}

/*
 * No Operation SFCGAL function, used (only) for cunit tests
 * Take a PostGIS geometry, send it to SFCGAL and return it unchanged (in theory)
 */
LWGEOM *
lwgeom_sfcgal_noop(const LWGEOM *geom_in)
{
	sfcgal_geometry_t *converted;

	assert(geom_in);

	converted = LWGEOM2SFCGAL(geom_in);
	assert(converted);

	LWGEOM *geom_out = SFCGAL2LWGEOM(converted, 0, SRID_UNKNOWN);
	sfcgal_geometry_delete(converted);

	/* copy SRID (SFCGAL does not store the SRID) */
	geom_out->srid = geom_in->srid;
	return geom_out;
}
