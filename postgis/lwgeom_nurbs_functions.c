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
 * Basic NURBS Curve Functions for PostGIS
 *
 * This file implements basic NURBS (Non-Uniform Rational B-Spline)
 * functionality including constructors, getters, and setters.
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "catalog/pg_type.h"

#include "../postgis_config.h"
#include "liblwgeom.h"
#include "lwgeom_pg.h"

Datum ST_MakeNurbsCurve(PG_FUNCTION_ARGS);
Datum ST_MakeNurbsCurveWithWeights(PG_FUNCTION_ARGS);
Datum ST_MakeNurbsCurveComplete(PG_FUNCTION_ARGS);
Datum ST_NurbsCurveControlPoints(PG_FUNCTION_ARGS);
Datum ST_NurbsCurveDegree(PG_FUNCTION_ARGS);
Datum ST_NurbsCurveWeights(PG_FUNCTION_ARGS);
Datum ST_NurbsCurveKnots(PG_FUNCTION_ARGS);
Datum ST_NurbsCurveNumControlPoints(PG_FUNCTION_ARGS);
Datum ST_NurbsCurveIsRational(PG_FUNCTION_ARGS);
Datum ST_NurbsCurveIsValid(PG_FUNCTION_ARGS);
Datum ST_NurbsEvaluate(PG_FUNCTION_ARGS);
Datum ST_NurbsToLineString(PG_FUNCTION_ARGS);

static ArrayType* float8_array_from_double_array(double *values, int count);
static double* double_array_from_float8_array(ArrayType *array, int *count);

/**
 * Convert a C array of doubles into a PostgreSQL float8 ArrayType.
 *
 * Returns a newly constructed PostgreSQL float8 array containing `count` elements
 * copied from `values`. If `values` is NULL or `count` is not positive, NULL is
 * returned.
 *
 * @param values C array of doubles to convert (must not be NULL for a valid result).
 * @param count  Number of elements in `values` (must be > 0 for a valid result).
 * @return Pointer to an ArrayType representing a PostgreSQL float8 array, or NULL
 *         if input is invalid. The returned ArrayType is allocated in the current
 *         PostgreSQL memory context. */
static ArrayType*
float8_array_from_double_array(double *values, int count)
{
	ArrayType *result;
	Datum *elems;
	int i;

	if (!values || count <= 0) return NULL;

	elems = palloc(sizeof(Datum) * count);
	for (i = 0; i < count; i++) {
		elems[i] = Float8GetDatum(values[i]);
	}

	result = construct_array(elems, count, FLOAT8OID, sizeof(float8), FLOAT8PASSBYVAL, 'd');
	pfree(elems);
	return result;
}

/**
 * Convert a PostgreSQL float8 ArrayType to a newly allocated C double array.
 *
 * If `array` is NULL, sets `*count` to 0 and returns NULL. Otherwise allocates
 * a double array (using palloc) with length equal to the number of elements in
 * `array`, copies every non-NULL element into it, sets `*count` to that length,
 * and returns the allocated pointer. The caller is responsible for freeing the
 * returned array with pfree when no longer needed.
 *
 * The function will raise an error (ereport ERROR, ERRCODE_NULL_VALUE_NOT_ALLOWED)
 * if any element of `array` is NULL.
 *
 * @param array PostgreSQL float8 ArrayType to convert.
 * @param count Out parameter set to the number of doubles in the returned array
 *              (0 if `array` is NULL).
 * @return Pointer to a palloc'd double array, or NULL if `array` is NULL.
 */
static double*
double_array_from_float8_array(ArrayType *array, int *count)
{
	bool *nulls;
	Datum *elems;
	double *result;
	int i, nelems;

	if (!array) {
		*count = 0;
		return NULL;
	}

	deconstruct_array(array, FLOAT8OID, sizeof(float8), FLOAT8PASSBYVAL, 'd',
		&elems, &nulls, &nelems);

	result = palloc(sizeof(double) * nelems);
	for (i = 0; i < nelems; i++) {
		if (nulls[i]) {
			pfree(result);
			ereport(ERROR, (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("Array cannot contain NULL values")));
		}
		result[i] = DatumGetFloat8(elems[i]);
	}

	*count = nelems;
	pfree(elems);
	pfree(nulls);
	return result;
}

/* ST_MakeNurbsCurve(degree, control_points) - Basic constructor */
PG_FUNCTION_INFO_V1(ST_MakeNurbsCurve);
/**
 * Construct a non-rational NURBS curve from a control LINESTRING and a degree.
 *
 * Creates a LWNURBSCURVE with uniform (implicit) weights from the provided
 * control points LINESTRING and the specified degree, then serializes and
 * returns it as a GSERIALIZED geometry.
 *
 * Expected inputs (passed via PG_FUNCTION_ARGS):
 * - degree: integer in range [1, 10]
 * - control_points: a LINESTRING geometry (GSERIALIZED) containing control points
 *
 * Behavior and constraints:
 * - Degree must be between 1 and 10 inclusive; otherwise an ERROR is raised.
 * - The control_points geometry must be a LINESTRING; otherwise an ERROR is raised.
 * - The LINESTRING must contain at least (degree + 1) control points; otherwise an ERROR is raised.
 * - The constructed NURBS uses uniform/default weights (i.e., no explicit weights or knots).
 *
 * Returns:
 * - A GSERIALIZED representing the constructed NURBS curve on success.
 * - NULL is returned if either input argument is SQL NULL.
 *
 * Errors:
 * - Raises an ERROR for invalid parameters (degree out of range, wrong geometry type,
 *   insufficient control points) or internal construction failure.
 */
Datum ST_MakeNurbsCurve(PG_FUNCTION_ARGS)
{
	uint32_t degree;
	GSERIALIZED *pcontrol_pts;
	LWGEOM *control_geom;
	LWLINE *line;
	POINTARRAY *ctrl_pts;
	LWNURBSCURVE *nurbs;
	GSERIALIZED *result;

	/* Validate input parameters */
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	degree = (uint32_t)PG_GETARG_INT32(0);
	pcontrol_pts = PG_GETARG_GSERIALIZED_P(1);

	/* Validate degree bounds */
	if (degree < 1 || degree > 10) {
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("NURBS degree must be between 1 and 10, got %d", degree)));
	}

	/* Extract control points from geometry */
	control_geom = lwgeom_from_gserialized(pcontrol_pts);
	if (!control_geom) {
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Invalid control points geometry")));
	}

	if (control_geom->type != LINETYPE) {
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Control points must be a LINESTRING geometry")));
	}

	line = (LWLINE*)control_geom;
	if (!line->points || line->points->npoints < degree + 1) {
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Need at least %d control points for degree %d NURBS",
				degree + 1, degree)));
	}

	/* Create NURBS curve with uniform weights */
	ctrl_pts = ptarray_clone_deep(line->points);
	nurbs = lwnurbscurve_construct(control_geom->srid, NULL, degree, ctrl_pts,
		NULL, NULL, 0, 0);

	if (!nurbs) {
		ptarray_free(ctrl_pts);
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Failed to construct NURBS curve")));
	}

	result = geometry_serialize((LWGEOM*)nurbs);

	lwgeom_free(control_geom);
	lwnurbscurve_free(nurbs);

	PG_RETURN_POINTER(result);
}

/* ST_MakeNurbsCurve(degree, control_points, weights) - Constructor with weights */
PG_FUNCTION_INFO_V1(ST_MakeNurbsCurveWithWeights);
/**
 * Construct a NURBS curve using a degree, control points LINESTRING, and per-control-point weights.
 *
 * Given SQL arguments (degree INT, control_points GEOMETRY, weights FLOAT8[]), validates inputs and returns
 * a serialized LWNURBSCURVE (GSERIALIZED) Datum representing the constructed NURBS curve.
 *
 * Validation performed:
 * - NULL arguments: function returns SQL NULL.
 * - degree must be between 1 and 10.
 * - control_points must be a LINESTRING with at least degree + 1 points.
 * - weights must be a non-NULL float8 array with length equal to the number of control points and all values > 0.
 *
 * On validation failure or construction failure the function raises a PostgreSQL error (ereport).  On success
 * returns a pointer to a newly allocated GSERIALIZED containing the NURBS curve; caller (PostgreSQL) takes ownership.
 *
 * @return Datum pointer to the serialized NURBS curve (GSERIALIZED) or SQL NULL if any input argument is NULL.
 */
Datum ST_MakeNurbsCurveWithWeights(PG_FUNCTION_ARGS)
{
	uint32_t degree;
	GSERIALIZED *pcontrol_pts;
	ArrayType *weights_array;
	LWGEOM *control_geom;
	LWLINE *line;
	POINTARRAY *ctrl_pts;
	LWNURBSCURVE *nurbs;
	GSERIALIZED *result;
	double *weights;
	int weight_count;
	int i;

	/* Validate input parameters */
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2)) {
		PG_RETURN_NULL();
	}

	degree = (uint32_t)PG_GETARG_INT32(0);
	pcontrol_pts = PG_GETARG_GSERIALIZED_P(1);
	weights_array = PG_GETARG_ARRAYTYPE_P(2);

	/* Validate degree bounds */
	if (degree < 1 || degree > 10) {
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("NURBS degree must be between 1 and 10, got %d", degree)));
	}

	/* Extract control points from geometry */
	control_geom = lwgeom_from_gserialized(pcontrol_pts);
	if (!control_geom) {
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Invalid control points geometry")));
	}

	if (control_geom->type != LINETYPE) {
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Control points must be a LINESTRING geometry")));
	}

	line = (LWLINE*)control_geom;
	if (!line->points || line->points->npoints < degree + 1) {
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Need at least %d control points for degree %d NURBS",
				degree + 1, degree)));
	}

	/* Extract and validate weights */
	weights = double_array_from_float8_array(weights_array, &weight_count);
	if (!weights) {
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Invalid weights array")));
	}

	if (weight_count != (int)line->points->npoints) {
		pfree(weights);
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Number of weights (%d) must equal number of control points (%d)",
				weight_count, line->points->npoints)));
	}

	/* Validate all weights are positive */
	for (i = 0; i < weight_count; i++) {
		if (weights[i] <= 0.0) {
			pfree(weights);
			lwgeom_free(control_geom);
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("All weights must be positive, got %g at position %d",
					weights[i], i)));
		}
	}

	/* Create NURBS curve with specified weights */
	ctrl_pts = ptarray_clone_deep(line->points);
	nurbs = lwnurbscurve_construct(control_geom->srid, NULL, degree, ctrl_pts,
		weights, NULL, weight_count, 0);

	if (!nurbs) {
		ptarray_free(ctrl_pts);
		pfree(weights);
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Failed to construct NURBS curve")));
	}

	/* Ensure dimensional consistency */
	if (nurbs->points) {
		nurbs->flags = nurbs->points->flags;
	}

	result = geometry_serialize((LWGEOM*)nurbs);

	pfree(weights);
	lwgeom_free(control_geom);
	lwnurbscurve_free(nurbs);

	PG_RETURN_POINTER(result);
}
/* ST_MakeNurbsCurve(degree, control_points, weights, knots) - Complete constructor */
PG_FUNCTION_INFO_V1(ST_MakeNurbsCurveComplete);
/**
 * Construct a NURBS curve from degree, control points, and optional weights and knots.
 *
 * Creates and returns a NURBSCURVETYPE geometry built from the provided degree and
 * control points LINESTRING. Optional `weights` and `knots` arrays may be supplied
 * to produce a rational curve and/or specify the knot vector.
 *
 * Requirements and validation:
 * - If either required argument (degree or control_points) is NULL the function
 *   returns NULL.
 * - `degree` must be between 1 and 10 (inclusive).
 * - `control_points` must be a LINESTRING and contain at least `degree + 1` points.
 * - If `weights` is provided, it must have the same length as the number of control
 *   points and all weights must be > 0.
 * - If `knots` is provided, its length must equal `npoints + degree + 1` and the
 *   knot vector must be non-decreasing.
 *
 * On invalid input the function raises a PostgreSQL error with ERRCODE_INVALID_PARAMETER_VALUE.
 *
 * Return:
 * - A pointer to a GSERIALIZED representing the constructed NURBS curve on success.
 * - NULL if required arguments are NULL.
 */
Datum ST_MakeNurbsCurveComplete(PG_FUNCTION_ARGS)
{
	uint32_t degree;
	GSERIALIZED *pcontrol_pts;
	ArrayType *weights_array = NULL;
	ArrayType *knots_array = NULL;
	LWGEOM *control_geom;
	LWLINE *line;
	POINTARRAY *ctrl_pts;
	LWNURBSCURVE *nurbs;
	GSERIALIZED *result;
	double *weights = NULL;
	double *knots = NULL;
	int weight_count = 0, knot_count = 0;
	int i, expected_knots;

	/* Validate required parameters */
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	degree = (uint32_t)PG_GETARG_INT32(0);
	pcontrol_pts = PG_GETARG_GSERIALIZED_P(1);

	/* Optional parameters */
	if (!PG_ARGISNULL(2)) {
		weights_array = PG_GETARG_ARRAYTYPE_P(2);
	}
	if (!PG_ARGISNULL(3)) {
		knots_array = PG_GETARG_ARRAYTYPE_P(3);
	}

	/* Validate degree bounds */
	if (degree < 1 || degree > 10) {
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("NURBS degree must be between 1 and 10, got %d", degree)));
	}

	/* Extract control points from geometry */
	control_geom = lwgeom_from_gserialized(pcontrol_pts);
	if (!control_geom) {
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Invalid control points geometry")));
	}

	if (control_geom->type != LINETYPE) {
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Control points must be a LINESTRING geometry")));
	}

	line = (LWLINE*)control_geom;
	if (!line->points || line->points->npoints < degree + 1) {
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Need at least %d control points for degree %d NURBS",
				degree + 1, degree)));
	}

	/* Extract and validate weights if provided */
	if (weights_array) {
		weights = double_array_from_float8_array(weights_array, &weight_count);
		if (!weights) {
			lwgeom_free(control_geom);
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Invalid weights array")));
		}

		if (weight_count != (int)line->points->npoints) {
			pfree(weights);
			lwgeom_free(control_geom);
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Number of weights (%d) must equal number of control points (%d)",
					weight_count, line->points->npoints)));
		}

		/* Validate all weights are positive */
		for (i = 0; i < weight_count; i++) {
			if (weights[i] <= 0.0) {
				pfree(weights);
				lwgeom_free(control_geom);
				ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("All weights must be positive, got %g at position %d",
						weights[i], i)));
			}
		}
	}

	/* Extract and validate knots if provided */
	if (knots_array) {
		knots = double_array_from_float8_array(knots_array, &knot_count);
		if (!knots) {
			if (weights) pfree(weights);
			lwgeom_free(control_geom);
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Invalid knots array")));
		}

		/* Validate knot vector length: should be npoints + degree + 1 */
		expected_knots = line->points->npoints + degree + 1;
		if (knot_count != expected_knots) {
			pfree(knots);
			if (weights) pfree(weights);
			lwgeom_free(control_geom);
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Knot vector must have %d elements for %d control points and degree %d, got %d",
					expected_knots, line->points->npoints, degree, knot_count)));
		}

		/* Validate knot vector is non-decreasing */
		for (i = 1; i < knot_count; i++) {
			if (knots[i] < knots[i-1]) {
				pfree(knots);
				if (weights) pfree(weights);
				lwgeom_free(control_geom);
				ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Knot vector must be non-decreasing, but knot[%d]=%g > knot[%d]=%g",
						i-1, knots[i-1], i, knots[i])));
			}
		}
	}

	/* Create NURBS curve with specified parameters */
	ctrl_pts = ptarray_clone_deep(line->points);
	nurbs = lwnurbscurve_construct(control_geom->srid, NULL, degree, ctrl_pts,
		weights, knots, weight_count, knot_count);

	if (!nurbs) {
		ptarray_free(ctrl_pts);
		if (weights) pfree(weights);
		if (knots) pfree(knots);
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Failed to construct NURBS curve")));
	}

	/* Ensure dimensional consistency */
	if (nurbs->points) {
		nurbs->flags = nurbs->points->flags;
	}

	result = geometry_serialize((LWGEOM*)nurbs);

	if (weights) pfree(weights);
	if (knots) pfree(knots);
	lwgeom_free(control_geom);
	lwnurbscurve_free(nurbs);

	PG_RETURN_POINTER(result);
}

/* ST_NurbsCurveControlPoints(nurbscurve) - Extract control points */
PG_FUNCTION_INFO_V1(ST_NurbsCurveControlPoints);
/**
 * Return the control points of a NURBS curve as a MULTIPOINT geometry.
 *
 * Accepts a serialized NURBS curve and returns a serialized MULTIPOINT containing
 * the curve's control points. If the input is NULL, the function returns NULL.
 *
 * Errors:
 * - Raises an error if the input is not a NURBS curve.
 * - Raises an error if the NURBS curve has no control points.
 * - Raises an internal error if the MULTIPOINT cannot be created.
 */
Datum ST_NurbsCurveControlPoints(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	LWMPOINT *mpoint;
	GSERIALIZED *result;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(pnurbs);

	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input must be a NURBS curve")));
	}

	nurbs = (LWNURBSCURVE*)geom;
	if (!nurbs->points || nurbs->points->npoints == 0) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("NURBS curve has no control points")));
	}

	/* Create MULTIPOINT from control points */
	mpoint = lwmpoint_construct(nurbs->srid, ptarray_clone_deep(nurbs->points));
	if (!mpoint) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Failed to create control points")));
	}

	result = geometry_serialize((LWGEOM*)mpoint);
	lwgeom_free(geom);
	lwmpoint_free(mpoint);

	PG_RETURN_POINTER(result);
}

/* ST_NurbsCurveDegree(nurbscurve) - Get degree */
PG_FUNCTION_INFO_V1(ST_NurbsCurveDegree);
/**
 * Return the polynomial degree of a NURBS curve.
 *
 * Given a NURBS curve geometry, returns its degree as an int32. If the input
 * is NULL the function returns SQL NULL. If the input is not a NURBS curve
 * an error is raised.
 *
 * @param nurbsgs A GSERIALIZED representing a NURBS curve (NULL allowed).
 * @return degree of the NURBS curve as int32, or NULL when input is NULL.
 */
Datum ST_NurbsCurveDegree(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	int32 degree;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(pnurbs);

	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input must be a NURBS curve")));
	}

	nurbs = (LWNURBSCURVE*)geom;
	degree = (int32)nurbs->degree;
	lwgeom_free(geom);

	PG_RETURN_INT32(degree);
}

/* ST_NurbsCurveWeights(nurbscurve) - Get weights array */
PG_FUNCTION_INFO_V1(ST_NurbsCurveWeights);
/**
 * Return the weight vector of a NURBS curve as a PostgreSQL float8 array.
 *
 * Given a serialized NURBS curve, returns an SQL float8[] containing the
 * per-control-point weights. If the input is NULL, or the NURBS curve has
 * no weights, or the internal conversion fails, the function returns NULL.
 *
 * The function raises an ERROR if the provided geometry is not a NURBS curve.
 *
 * @return float8[] of length equal to the number of control points, or NULL.
 */
Datum ST_NurbsCurveWeights(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	ArrayType *result;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(pnurbs);

	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input must be a NURBS curve")));
	}

	nurbs = (LWNURBSCURVE*)geom;
	if (!nurbs->weights) {
		lwgeom_free(geom);
		PG_RETURN_NULL();
	}

	result = float8_array_from_double_array(nurbs->weights, (int)nurbs->nweights);
	lwgeom_free(geom);

	if (!result) {
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);
}

/* ST_NurbsCurveKnots(nurbscurve) - Get knots array */
PG_FUNCTION_INFO_V1(ST_NurbsCurveKnots);
/**
 * Return the knot vector of a NURBS curve as a PostgreSQL float8 array.
 *
 * If the input is not a NURBS curve, an error is raised. If the NURBS
 * curve has no knot vector or the conversion to a float8 array fails,
 * the function returns NULL.
 *
 * @param pnurbs GSERIALIZED pointer to a NURBS curve
 * @return float8[] PostgreSQL array containing the knot vector, or NULL
 */
Datum ST_NurbsCurveKnots(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	ArrayType *result;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(pnurbs);

	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input must be a NURBS curve")));
	}

	nurbs = (LWNURBSCURVE*)geom;
	if (!nurbs->knots || nurbs->nknots == 0) {
		lwgeom_free(geom);
		PG_RETURN_NULL();
	}

	/* Use only the actual knot count, no inference */
	if (nurbs->nknots <= 0) {
		lwgeom_free(geom);
		PG_RETURN_NULL();
	}

	result = float8_array_from_double_array(nurbs->knots, (int)nurbs->nknots);

	lwgeom_free(geom);

	if (!result) {
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);
}

/* ST_NurbsCurveNumControlPoints(nurbscurve) - Get number of control points */
PG_FUNCTION_INFO_V1(ST_NurbsCurveNumControlPoints);
/**
 * Get number of control points in a NURBS curve.
 *
 * Returns the count of control points for the provided NURBS curve GSERIALIZED.
 *
 * @param pnurbs GSERIALIZED representing a NURBS curve; if NULL the function returns NULL.
 * @returns int32 number of control points (0 if the curve has no control points).
 * @throws ERROR if the input is not a NURBS curve.
 */
Datum ST_NurbsCurveNumControlPoints(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	int32 npoints;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(pnurbs);

	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input must be a NURBS curve")));
	}

	nurbs = (LWNURBSCURVE*)geom;
	npoints = nurbs->points ? (int32)nurbs->points->npoints : 0;
	lwgeom_free(geom);

	PG_RETURN_INT32(npoints);
}

/* ST_NurbsCurveIsRational(nurbscurve) - Check if curve is rational */
PG_FUNCTION_INFO_V1(ST_NurbsCurveIsRational);
/**
 * Determine whether a NURBS curve is rational (has weights).
 *
 * Returns true if the provided NURBS curve contains a weight array, false otherwise.
 * If the input is NULL the function returns SQL NULL. If the input is not a NURBS
 * curve an error is raised.
 *
 * @param pnurbs GSERIALIZED NURBS curve to inspect.
 * @return boolean true when the curve is rational, false when it is not.
 */
Datum ST_NurbsCurveIsRational(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	const LWNURBSCURVE *nurbs;
	bool is_rational;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(pnurbs);

	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input must be a NURBS curve")));
	}

	nurbs = (LWNURBSCURVE*)geom;
	is_rational = (nurbs->weights != NULL);
	lwgeom_free(geom);

	PG_RETURN_BOOL(is_rational);
}

/* ST_NurbsCurveIsValid(nurbscurve) - Validate NURBS curve */
PG_FUNCTION_INFO_V1(ST_NurbsCurveIsValid);
/**
 * Validate a NURBS curve geometry.
 *
 * Returns whether the given GSERIALIZED NURBS curve satisfies basic validity rules:
 * - control points exist and their count >= degree + 1,
 * - all weights are strictly positive (if a weight vector is present),
 * - knot vector values are non-decreasing (checked up to the expected knot count).
 *
 * Note: If the input is NULL this function returns NULL. If the input is not a NURBS
 * geometry, the function returns false. This routine performs basic checks only and
 * does not fully verify knot-vector length or other advanced mathematical consistency.
 *
 * @param nurbscurve GSERIALIZED NURBSCURVE geometry to validate.
 * @return boolean true if the basic validity checks pass, false otherwise; NULL if input is NULL.
 */
Datum ST_NurbsCurveIsValid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	bool is_valid = true;
	uint32_t i, expected_knots;
	uint32_t actual_check_count = 0;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);
	geom = lwgeom_from_gserialized(pnurbs);

	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		PG_RETURN_BOOL(false);
	}

	nurbs = (LWNURBSCURVE*)geom;

	/* Check basic requirements */
	if (!nurbs->points || nurbs->points->npoints < nurbs->degree + 1) {
		is_valid = false;
		goto cleanup;
	}

	/* Check weights if present */
	if (nurbs->weights && nurbs->nweights > 0) {
		/* Verify nweights matches npoints before looping */
		if (nurbs->nweights < nurbs->points->npoints) {
			is_valid = false;
			goto cleanup;
		}
		for (i = 0; i < nurbs->nweights && i < nurbs->points->npoints; i++) {
			if (nurbs->weights[i] <= 0.0) {
				is_valid = false;
				goto cleanup;
			}
		}
	}

	/* Check knot vector if present */
	if (nurbs->knots && nurbs->nknots > 0) {
		expected_knots = nurbs->points->npoints + nurbs->degree + 1;

		/* Verify nknots is consistent before checking values */
		if (nurbs->nknots < expected_knots) {
			is_valid = false;
			goto cleanup;
		}

		/* Check knot vector is non-decreasing using actual count */
		actual_check_count = (nurbs->nknots < expected_knots) ? nurbs->nknots : expected_knots;
		for (i = 1; i < actual_check_count; i++) {
			if (nurbs->knots[i] < nurbs->knots[i-1]) {
				is_valid = false;
				goto cleanup;
			}
		}
	}

cleanup:
	lwgeom_free(geom);
	PG_RETURN_BOOL(is_valid);
}

/* ST_NurbsEvaluate(nurbscurve, parameter) - Evaluate NURBS curve at parameter */
PG_FUNCTION_INFO_V1(ST_NurbsEvaluate);
/**
 * Evaluate a NURBS curve at a specific parameter value.
 *
 * Returns a POINT representing the position on the NURBS curve at the given
 * parameter value. The parameter should typically be in the range [0,1], where
 * 0 corresponds to the start of the curve and 1 to the end.
 *
 * Parameters:
 * - nurbscurve: A GSERIALIZED NURBS curve
 * - parameter: A float8 parameter value for evaluation
 *
 * Returns:
 * - A POINT geometry representing the evaluated position
 * - NULL if either input is NULL or the curve is invalid
 *
 * Example:
 * SELECT ST_NurbsEvaluate(nurbs_curve, 0.5); -- Get midpoint of curve
 */
Datum ST_NurbsEvaluate(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	double parameter;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	LWPOINT *result_point;
	GSERIALIZED *result;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);
	parameter = PG_GETARG_FLOAT8(1);

	geom = lwgeom_from_gserialized(pnurbs);
	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input must be a NURBS curve")));
	}

	nurbs = (LWNURBSCURVE*)geom;

	/* Evaluate the curve at the given parameter */
	result_point = lwnurbscurve_evaluate(nurbs, parameter);
	if (!result_point) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Failed to evaluate NURBS curve at parameter %g", parameter)));
	}

	result = geometry_serialize((LWGEOM*)result_point);

	lwgeom_free(geom);
	lwpoint_free(result_point);

	PG_RETURN_POINTER(result);
}

/* ST_NurbsToLineString(nurbscurve, num_segments) - Convert NURBS to LineString */
PG_FUNCTION_INFO_V1(ST_NurbsToLineString);
/**
 * Convert a NURBS curve to a LineString by sampling at uniform intervals.
 *
 * Creates a piecewise linear approximation of the NURBS curve by evaluating
 * it at uniformly distributed parameter values and connecting the resulting
 * points with straight line segments.
 *
 * Parameters:
 * - nurbscurve: A GSERIALIZED NURBS curve
 * - num_segments: Number of line segments in the output (default: 32)
 *
 * Returns:
 * - A LINESTRING geometry approximating the NURBS curve
 * - NULL if the input is NULL or invalid
 *
 * Example:
 * SELECT ST_NurbsToLineString(nurbs_curve, 64); -- High-quality approximation
 * SELECT ST_NurbsToLineString(nurbs_curve, 16); -- Lower-quality, faster
 */
Datum ST_NurbsToLineString(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	uint32_t num_segments = 32; /* Default number of segments */
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	LWLINE *result_line;
	GSERIALIZED *result;

	if (PG_ARGISNULL(0)) {
		PG_RETURN_NULL();
	}

	pnurbs = PG_GETARG_GSERIALIZED_P(0);

	/* Get optional number of segments parameter */
	if (!PG_ARGISNULL(1)) {
		int32_t segments_arg = PG_GETARG_INT32(1);
		if (segments_arg < 2) {
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Number of segments must be at least 2, got %d", segments_arg)));
		}
		if (segments_arg > 10000) {
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Number of segments too large, got %d (maximum 10000)", segments_arg)));
		}
		num_segments = (uint32_t)segments_arg;
	}

	geom = lwgeom_from_gserialized(pnurbs);
	if (!geom || geom->type != NURBSCURVETYPE) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input must be a NURBS curve")));
	}

	nurbs = (LWNURBSCURVE*)geom;

	/* Convert NURBS to LineString */
	result_line = lwnurbscurve_to_linestring(nurbs, num_segments);
	if (!result_line) {
		lwgeom_free(geom);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Failed to convert NURBS curve to LineString")));
	}

	result = geometry_serialize((LWGEOM*)result_line);

	lwgeom_free(geom);
	lwline_free(result_line);

	PG_RETURN_POINTER(result);
}
