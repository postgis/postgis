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
 * Construct a new NURBS curve
 *
 * Creates a NURBS (Non-Uniform Rational B-Spline) curve from the provided
 * control points, weights, and knot vector. NURBS curves are parametric
 * curves defined by:
 * - Control points: Define the shape of the curve
 * - Weights: Make the curve "rational" (if NULL, curve is polynomial)
 * - Knot vector: Controls parameterization and continuity
 * - Degree: Polynomial degree of the curve segments
 *
 * @param srid Spatial reference system identifier
 * @param degree Polynomial degree (1-10, where 1=linear, 3=cubic)
 * @param points Control points array (takes ownership)
 * @param weights Weight array for rational curves (copied, can be NULL)
 * @param knots Knot vector (copied, can be NULL for uniform knots)
 * @param nweights Number of weights (must equal npoints if weights != NULL)
 * @param nknots Number of knots (must equal npoints + degree + 1 if provided)
 * @return New NURBS curve or NULL on invalid parameters
 */
LWNURBSCURVE *
lwnurbscurve_construct(int32_t srid, uint32_t degree, POINTARRAY *points,
                      double *weights, double *knots, uint32_t nweights, uint32_t nknots)
{
	LWNURBSCURVE *result;

	/* Validate degree: must be between 1 and 10 for practical use */
	if (degree < 1 || degree > 10)
		return NULL;

	/* Validate consistency between weights and control points */
	if (weights != NULL && points != NULL && nweights != points->npoints)
		return NULL;

	/* Validate consistency between knots and NURBS constraints */
	if (knots != NULL && points != NULL && nknots != points->npoints + degree + 1)
		return NULL;

	result = lwalloc(sizeof(LWNURBSCURVE));
	result->type = NURBSCURVETYPE;
	result->srid = srid;
	result->bbox = NULL; /* Computed on demand */
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

	/* lwnurbscurve_construct will deep-copy weights and knots arrays */
	return lwnurbscurve_construct(curve->srid, curve->degree, points,
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

	/* Standard NURBS constraint: knot vector size */
	nknots = npoints + degree + 1;
	knots = lwalloc(sizeof(double) * nknots);

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
 * Get knot vector for WKB output (generate uniform if none exists)
 *
 * Returns a knot vector suitable for serialization to WKB format.
 * If the curve has an explicit knot vector, it is copied.
 * Otherwise, a uniform clamped knot vector is generated automatically.
 *
 * The caller is responsible for freeing the returned array.
 *
 * @param curve Source NURBS curve
 * @param nknots_out Output parameter for knot vector size
 * @return Newly allocated knot vector, or NULL for invalid/empty curves
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
 * Convert to LWGEOM
 *
 * Performs a safe cast from LWNURBSCURVE to the base LWGEOM type.
 * This enables NURBS curves to be used in generic PostGIS geometry
 * operations through polymorphism.
 *
 * @param obj NURBS curve to cast
 * @return Same object cast as LWGEOM base type
 */
LWGEOM *
lwnurbscurve_as_lwgeom(const LWNURBSCURVE *obj)
{
	return (LWGEOM *)obj;
}
