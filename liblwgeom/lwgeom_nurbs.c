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
 * Copyright (C) 2025 PostGIS contributors
 * Basic NURBS curve support for PostGIS
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"

/**
 * Construct a new NURBS curve.
 *
 * Creates a NURBS (Non-Uniform Rational B-Spline) curve object from the
 * provided control points, optional weights, and optional knot vector.
 *
 * Validation and ownership:
 * - Returns NULL if degree is outside [1,10].
 * - If `weights` is non-NULL, `nweights` must equal the number of control points.
 * - If `knots` is non-NULL, `nknots` must equal npoints + degree + 1.
 * - Ownership of `points` is transferred to the returned curve (caller must not free it).
 * - `weights` and `knots`, if provided, are deep-copied into the new curve.
 *
 * Behavioral notes:
 * - If `weights` is NULL, the curve is treated as non-rational (implicit weights = 1.0).
 * - If `knots` is NULL, a uniform clamped knot vector will be generated on demand.
 *
 * @param srid Spatial reference identifier for the curve.
 * @param bbox Bounding box for the curve (can be NULL).
 * @param degree Polynomial degree of the curve (1..10).
 * @param points Control points array; ownership is transferred to the returned curve.
 * @param weights Optional weights array; copied when non-NULL.
 * @param knots Optional knot vector; copied when non-NULL.
 * @param nweights Number of entries in `weights` (must match point count if `weights` provided).
 * @param nknots Number of entries in `knots` (must equal npoints + degree + 1 if `knots` provided).
 * @return Pointer to the newly allocated LWNURBSCURVE, or NULL on invalid parameters.
 */
LWNURBSCURVE *
lwnurbscurve_construct(int32_t srid, GBOX *bbox, uint32_t degree, POINTARRAY *points,
                      double *weights, double *knots, uint32_t nweights, uint32_t nknots)
{
	LWNURBSCURVE *result;

	/* Validate degree: must be between 1 and 10 for practical use */
	if (degree < 1 || degree > 10)
		return NULL;

	/* Basic invariants */
	if (points && weights && nweights != points->npoints)
		lwerror("NURBS: nweights (%u) must equal number of control points (%u)", nweights, points->npoints);
	if (points && knots && nknots != points->npoints + degree + 1)
		lwerror("NURBS: nknots (%u) must equal npoints + degree + 1 (%u)", nknots, points->npoints + degree + 1);
	result = lwalloc(sizeof(LWNURBSCURVE));
	result->type = NURBSCURVETYPE;
	result->srid = srid;
	result->bbox = bbox; /* Use provided bbox */
	result->degree = degree;
	result->nweights = nweights;
	result->nknots = nknots;

	/* Inherit dimensional flags from control points */
	if (points) {
		result->flags = points->flags;
		result->points = points; /* Take ownership - caller must not free */
	} else {
		/* Empty curve: no dimensional information available */
		result->flags = 0;
		result->points = NULL;
	}

	/* Deep copy weights array for rational NURBS */
	if (weights && nweights > 0) {
		result->weights = lwalloc(sizeof(double) * nweights);
		memcpy(result->weights, weights, sizeof(double) * nweights);
	} else {
		/* Non-rational NURBS: all weights implicitly 1.0 */
		result->weights = NULL;
	}

	/* Deep copy knot vector for explicit parameterization */
	if (knots && nknots > 0) {
		result->knots = lwalloc(sizeof(double) * nknots);
		memcpy(result->knots, knots, sizeof(double) * nknots);
	} else {
		/* No knots: uniform parameterization will be generated when needed */
		result->knots = NULL;
	}

	return result;
}

/**
 * Construct an empty NURBS curve
 *
 * Creates a valid but empty NURBS curve with specified dimensional flags.
 * Empty curves contain no control points but maintain proper structure
 * for dimensional consistency in PostGIS operations.
 *
 * @param srid Spatial reference system identifier
 * @param hasz True if curve should support Z coordinates
 * @param hasm True if curve should support M coordinates
 * @return Empty NURBS curve with specified dimensions
 */
LWNURBSCURVE *
lwnurbscurve_construct_empty(int32_t srid, char hasz, char hasm)
{
	LWNURBSCURVE *result = lwalloc(sizeof(LWNURBSCURVE));
	result->type = NURBSCURVETYPE;
	result->flags = lwflags(hasz, hasm, 0); /* Set dimensional flags */
	result->srid = srid;
	result->bbox = NULL; /* No bounding box for empty geometry */
	result->degree = 1; /* Minimum valid degree */
	result->points = ptarray_construct_empty(hasz, hasm, 1); /* Empty point array */
	result->weights = NULL; /* No weights for empty curve */
	result->nweights = 0;
	result->knots = NULL; /* No knots for empty curve */
	result->nknots = 0;
	return result;
}

/**
 * Free NURBS curve memory
 *
 * Recursively frees all memory associated with a NURBS curve,
 * including bounding box, control points, weights, and knot vector.
 * Safe to call with NULL pointer.
 *
 * @param curve NURBS curve to free (can be NULL)
 */
void
lwnurbscurve_free(LWNURBSCURVE *curve)
{
	if (!curve) return;

	/* Free bounding box if computed */
	if (curve->bbox)
		lwfree(curve->bbox);

	/* Free control points array */
	if (curve->points)
		ptarray_free(curve->points);

	/* Free weights array for rational curves */
	if (curve->weights)
		lwfree(curve->weights);

	/* Free knot vector */
	if (curve->knots)
		lwfree(curve->knots);

	/* Free the curve structure itself */
	lwfree(curve);
}

/**
 * Deep clone NURBS curve
 *
 * Creates a complete independent copy of a NURBS curve.
 * All arrays (points, weights, knots) are deep-copied,
 * ensuring the clone can be modified without affecting the original.
 *
 * @param curve Source NURBS curve to clone
 * @return Deep copy of the curve, or NULL if input is NULL
 */
LWNURBSCURVE *
lwnurbscurve_clone_deep(const LWNURBSCURVE *curve)
{
	if (!curve) return NULL;

	/* Deep clone control points array */
	POINTARRAY *points = curve->points ? ptarray_clone_deep(curve->points) : NULL;

	/* Clone bbox if it exists */
	GBOX *bbox = curve->bbox ? gbox_copy(curve->bbox) : NULL;

	/* lwnurbscurve_construct will deep-copy weights and knots arrays */
	return lwnurbscurve_construct(curve->srid, bbox, curve->degree, points,
	                             curve->weights, curve->knots,
	                             curve->nweights, curve->nknots);
}

/**
 * Generate uniform knot vector for NURBS curve
 *
 * Creates a clamped uniform knot vector suitable for standard NURBS curves.
 * The knot vector has:
 * - (degree+1) repeated knots at the start (value 0.0) for clamping
 * - (degree+1) repeated knots at the end (value 1.0) for clamping
 * - Internal knots uniformly distributed between 0 and 1
 *
 * This ensures the curve starts at the first control point and ends
 * at the last control point (clamped behavior).
 *
 * Mathematical constraint: nknots = npoints + degree + 1
 *
 * @param degree Polynomial degree of the curve
 * @param npoints Number of control points
 * @param nknots_out Output parameter for knot vector size
 * @return Newly allocated uniform knot vector
 */
static double *
lwnurbscurve_generate_uniform_knots(uint32_t degree, uint32_t npoints, uint32_t *nknots_out)
{
	double *knots;
	uint32_t nknots, i;

	/* Input validation to prevent underflow */
	if (degree == 0 || npoints < degree + 1) {
		*nknots_out = 0;
		return NULL;
	}

	/* Standard NURBS constraint: knot vector size */
	if (npoints < degree + 1) {
		if (nknots_out) *nknots_out = 0;
		return NULL;
	}
	nknots = npoints + degree + 1;

	knots = lwalloc(sizeof(double) * nknots);
	if (!knots) {
		*nknots_out = 0;
		return NULL;
	}

	/* Clamp start: first (degree+1) knots set to 0.0 */
	for (i = 0; i <= degree; i++) {
		knots[i] = 0.0;
	}

	/* Clamp end: last (degree+1) knots set to 1.0 */
	for (i = nknots - degree - 1; i < nknots; i++) {
		knots[i] = 1.0;
	}

	/* Distribute internal knots uniformly in (0,1) interval */
	if (nknots > 2 * (degree + 1)) {
		uint32_t internal_knots = nknots - 2 * (degree + 1);
		for (i = 0; i < internal_knots; i++) {
			/* Uniform distribution: 1/(n+1), 2/(n+1), ..., n/(n+1) */
			knots[degree + 1 + i] = (double)(i + 1) / (internal_knots + 1);
		}
	}

	*nknots_out = nknots;
	return knots;
}

/**
 * Retrieve a knot vector suitable for WKB serialization.
 *
 * Returns a newly-allocated knot vector for the given NURBS curve. If the curve
 * has an explicit knot vector it is deep-copied; otherwise a clamped uniform
 * knot vector is generated from the curve degree and number of control points.
 *
 * The caller is responsible for freeing the returned array. If `curve` is NULL
 * or has no control points, `*nknots_out` is set to 0 and NULL is returned.
 *
 * @param curve Source NURBS curve.
 * @param nknots_out Output location which will be set to the number of knots
 *                   in the returned array (set to 0 on NULL/empty input).
 * @return Newly allocated array of knots, or NULL for invalid/empty curves.
 */
double *
lwnurbscurve_get_knots_for_wkb(const LWNURBSCURVE *curve, uint32_t *nknots_out)
{
	/* Validate input parameters */
	if (!curve || !curve->points) {
		*nknots_out = 0;
		return NULL;
	}

	/* Use explicit knot vector if available */
	if (curve->knots && curve->nknots > 0) {
		double *knots = lwalloc(sizeof(double) * curve->nknots);
		memcpy(knots, curve->knots, sizeof(double) * curve->nknots);
		*nknots_out = curve->nknots;
		return knots;
	}

	/* Generate uniform knot vector on demand */
	return lwnurbscurve_generate_uniform_knots(curve->degree, curve->points->npoints, nknots_out);
}

/**
 * Cast a NURBS curve to the generic LWGEOM type.
 *
 * This performs a safe, no-op pointer cast from LWNURBSCURVE to LWGEOM
 * so the curve can be used with APIs expecting LWGEOM without changing
 * ownership or copying data.
 *
 * @param obj NURBS curve to cast (may be NULL)
 * @return The same pointer value cast to LWGEOM*, or NULL if `obj` is NULL
 */
LWGEOM *
lwnurbscurve_as_lwgeom(const LWNURBSCURVE *obj)
{
	return (LWGEOM *)obj;
}

/**
 * Cast a generic LWGEOM to a NURBS curve.
 *
 * This performs a safe type check and cast from LWGEOM to LWNURBSCURVE.
 * Returns NULL if the input is not a NURBS curve type.
 *
 * @param lwgeom Geometry to cast (may be NULL)
 * @return The same pointer cast to LWNURBSCURVE*, or NULL if not a NURBSCURVE
 */
LWNURBSCURVE *
lwgeom_as_lwnurbscurve(const LWGEOM *lwgeom)
{
	if ( lwgeom == NULL ) return NULL;
	if ( lwgeom->type == NURBSCURVETYPE )
		return (LWNURBSCURVE *)lwgeom;
	else return NULL;
}

/**
 * Get the control points from a NURBS curve.
 *
 * Returns the POINTARRAY containing the control points that define
 * the NURBS curve. The returned pointer points directly to the curve's
 * internal data structure and should not be modified or freed.
 *
 * @param curve NURBS curve to extract control points from (must not be NULL)
 * @return Pointer to the control points array, or NULL if curve is NULL
 */
POINTARRAY *
lwnurbscurve_get_control_points(const LWNURBSCURVE *curve)
{
	if ( curve == NULL ) return NULL;
	return curve->points;
}
