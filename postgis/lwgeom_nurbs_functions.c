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

static ArrayType* float8_array_from_double_array(double *values, int count);
static double* double_array_from_float8_array(ArrayType *array, int *count);

/* Utility function to create float8 array from double array */
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

/* Utility function to extract double array from float8 array */
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
	nurbs = lwnurbscurve_construct(control_geom->srid, degree, ctrl_pts,
		NULL, NULL, 0, 0);

	if (!nurbs) {
		lwgeom_free(control_geom);
		ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("Failed to construct NURBS curve")));
	}

	/* Ensure dimensional consistency */
	if (nurbs->points) {
		nurbs->flags = nurbs->points->flags;
	}

	result = geometry_serialize((LWGEOM*)nurbs);

	lwgeom_free(control_geom);
	lwnurbscurve_free(nurbs);

	PG_RETURN_POINTER(result);
}

/* ST_MakeNurbsCurve(degree, control_points, weights) - Constructor with weights */
PG_FUNCTION_INFO_V1(ST_MakeNurbsCurveWithWeights);
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
	nurbs = lwnurbscurve_construct(control_geom->srid, degree, ctrl_pts,
		weights, NULL, 0, 0);

	if (!nurbs) {
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
	nurbs = lwnurbscurve_construct(control_geom->srid, degree, ctrl_pts,
		weights, knots, 0, 0);

	if (!nurbs) {
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

	result = float8_array_from_double_array(nurbs->weights, nurbs->points->npoints);
	lwgeom_free(geom);

	if (!result) {
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);
}

/* ST_NurbsCurveKnots(nurbscurve) - Get knots array */
PG_FUNCTION_INFO_V1(ST_NurbsCurveKnots);
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
	if (!nurbs->knots) {
		lwgeom_free(geom);
		PG_RETURN_NULL();
	}

	result = float8_array_from_double_array(nurbs->knots, nurbs->degree + nurbs->points->npoints + 1);
	lwgeom_free(geom);

	if (!result) {
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);
}

/* ST_NurbsCurveNumControlPoints(nurbscurve) - Get number of control points */
PG_FUNCTION_INFO_V1(ST_NurbsCurveNumControlPoints);
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
Datum ST_NurbsCurveIsRational(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
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
Datum ST_NurbsCurveIsValid(PG_FUNCTION_ARGS)
{
	GSERIALIZED *pnurbs;
	LWGEOM *geom;
	LWNURBSCURVE *nurbs;
	bool is_valid = true;
	int i, expected_knots;

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
	if (nurbs->weights) {
		for (i = 0; i < (int)nurbs->points->npoints; i++) {
			if (nurbs->weights[i] <= 0.0) {
				is_valid = false;
				goto cleanup;
			}
		}
	}

	/* Check knot vector if present */
	if (nurbs->knots) {
		expected_knots = nurbs->points->npoints + nurbs->degree + 1;
		/* Note: we can't easily verify knot count here without additional metadata */

		/* Check knot vector is non-decreasing - we'll check what we can */
		for (i = 1; i < expected_knots; i++) {
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
