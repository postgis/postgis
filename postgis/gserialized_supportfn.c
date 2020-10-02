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
 **********************************************************************/


#include "../postgis_config.h"

#if POSTGIS_PGSQL_VERSION >= 120

/* PostgreSQL */
#include "postgres.h"
#include "funcapi.h"
#include "access/htup_details.h"
#include "access/stratnum.h"
#include "catalog/namespace.h"
#include "catalog/pg_opfamily.h"
#include "catalog/pg_type_d.h"
#include "catalog/pg_am_d.h"
#include "nodes/supportnodes.h"
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"
#include "optimizer/optimizer.h"
#include "parser/parse_func.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/syscache.h"

/* PostGIS */
#include "liblwgeom.h"
#include "lwgeom_pg.h"

/* Local prototypes */
Datum postgis_index_supportfn(PG_FUNCTION_ARGS);

/* From gserialized_estimate.c */
float8 gserialized_joinsel_internal(PlannerInfo *root, List *args, JoinType jointype, int mode);
float8 gserialized_sel_internal(PlannerInfo *root, List *args, int varRelid, int mode);

enum ST_FUNCTION_IDX
{
	ST_INTERSECTS_IDX = 0,
	ST_DWITHIN_IDX = 1,
	ST_CONTAINS_IDX = 2,
	ST_WITHIN_IDX = 3,
	ST_TOUCHES_IDX = 4,
	ST_3DINTERSECTS_IDX = 5,
	ST_CONTAINSPROPERLY_IDX = 6,
	ST_COVEREDBY_IDX = 7,
	ST_OVERLAPS_IDX = 8,
	ST_COVERS_IDX = 9,
	ST_CROSSES_IDX = 10,
	ST_DFULLYWITHIN_IDX = 11,
	ST_3DDWITHIN_IDX = 12,
	ST_3DDFULLYWITHIN_IDX = 13,
	ST_LINECROSSINGDIRECTION_IDX = 14,
	ST_ORDERINGEQUALS_IDX = 15,
	ST_EQUALS_IDX = 16
};

static const int16 GeometryStrategies[] = {
	[ST_INTERSECTS_IDX]             = RTOverlapStrategyNumber,
	[ST_DWITHIN_IDX]                = RTOverlapStrategyNumber,
	[ST_CONTAINS_IDX]               = RTContainsStrategyNumber,
	[ST_WITHIN_IDX]                 = RTContainedByStrategyNumber,
	[ST_TOUCHES_IDX]                = RTOverlapStrategyNumber,
	[ST_3DINTERSECTS_IDX]           = RTOverlapStrategyNumber,
	[ST_CONTAINSPROPERLY_IDX]       = RTContainsStrategyNumber,
	[ST_COVEREDBY_IDX]              = RTContainedByStrategyNumber,
	[ST_OVERLAPS_IDX]               = RTOverlapStrategyNumber,
	[ST_COVERS_IDX]                 = RTContainsStrategyNumber,
	[ST_CROSSES_IDX]                = RTOverlapStrategyNumber,
	[ST_DFULLYWITHIN_IDX]           = RTOverlapStrategyNumber,
	[ST_3DDWITHIN_IDX]              = RTOverlapStrategyNumber,
	[ST_3DDFULLYWITHIN_IDX]         = RTOverlapStrategyNumber,
	[ST_LINECROSSINGDIRECTION_IDX]  = RTOverlapStrategyNumber,
	[ST_ORDERINGEQUALS_IDX]         = RTSameStrategyNumber,
	[ST_EQUALS_IDX]                 = RTSameStrategyNumber
};

/* We use InvalidStrategy for the functions that don't currently exist for geography */
static const int16 GeographyStrategies[] = {
	[ST_INTERSECTS_IDX]             = RTOverlapStrategyNumber,
	[ST_DWITHIN_IDX]                = RTOverlapStrategyNumber,
	[ST_CONTAINS_IDX]               = InvalidStrategy,
	[ST_WITHIN_IDX]                 = InvalidStrategy,
	[ST_TOUCHES_IDX]                = InvalidStrategy,
	[ST_3DINTERSECTS_IDX]           = InvalidStrategy,
	[ST_CONTAINSPROPERLY_IDX]       = InvalidStrategy,
	[ST_COVEREDBY_IDX]              = RTOverlapStrategyNumber, /* This is different than geometry */
	[ST_OVERLAPS_IDX]               = InvalidStrategy,
	[ST_COVERS_IDX]                 = RTOverlapStrategyNumber, /* This is different than geometry */
	[ST_CROSSES_IDX]                = InvalidStrategy,
	[ST_DFULLYWITHIN_IDX]           = InvalidStrategy,
	[ST_3DDWITHIN_IDX]              = InvalidStrategy,
	[ST_3DDFULLYWITHIN_IDX]         = InvalidStrategy,
	[ST_LINECROSSINGDIRECTION_IDX]  = InvalidStrategy,
	[ST_ORDERINGEQUALS_IDX]         = InvalidStrategy,
	[ST_EQUALS_IDX]                 = InvalidStrategy
};

static int16
get_strategy_by_type(Oid first_type, uint16_t index)
{
	if (first_type == postgis_oid(GEOMETRYOID))
	{
		return GeometryStrategies[index];
	}

	if (first_type == postgis_oid(GEOGRAPHYOID))
	{
		return GeographyStrategies[index];
	}

	return InvalidStrategy;
}

/*
* Depending on the function, we will deploy different
* index enhancement strategies. Containment functions
* can use a more strict index strategy than overlapping
* functions. For within-distance functions, we need
* to construct expanded boxes, on the non-indexed
* function argument. We store the metadata to drive
* these choices in the IndexableFunctions array.
*/
typedef struct
{
	const char *fn_name;
	uint16_t index;     /* Position of the strategy in the arrays */
	uint8_t nargs;      /* Expected number of function arguments */
	uint8_t expand_arg; /* Radius argument for "within distance" search */
} IndexableFunction;

/*
* Metadata currently scanned from start to back,
* so most common functions first. Could be sorted
* and searched with binary search.
*/
static const IndexableFunction IndexableFunctions[] = {
	{"st_intersects", ST_INTERSECTS_IDX, 2, 0},
	{"st_dwithin", ST_DWITHIN_IDX, 3, 3},
	{"st_contains", ST_CONTAINS_IDX, 2, 0},
	{"st_within", ST_WITHIN_IDX, 2, 0},
	{"st_touches", ST_TOUCHES_IDX, 2, 0},
	{"st_3dintersects", ST_3DINTERSECTS_IDX, 2, 0},
	{"st_containsproperly", ST_CONTAINSPROPERLY_IDX, 2, 0},
	{"st_coveredby", ST_COVEREDBY_IDX, 2, 0},
	{"st_overlaps", ST_OVERLAPS_IDX, 2, 0},
	{"st_covers", ST_COVERS_IDX, 2, 0},
	{"st_crosses", ST_CROSSES_IDX, 2, 0},
	{"st_dfullywithin", ST_DFULLYWITHIN_IDX, 3, 3},
	{"st_3ddwithin", ST_3DDWITHIN_IDX, 3, 3},
	{"st_3ddfullywithin", ST_3DDFULLYWITHIN_IDX, 3, 3},
	{"st_linecrossingdirection", ST_LINECROSSINGDIRECTION_IDX, 2, 0},
	{"st_orderingequals", ST_ORDERINGEQUALS_IDX, 2, 0},
	{"st_equals", ST_EQUALS_IDX, 2, 0},
	{NULL, 0, 0, 0}
};

/*
* Is the function calling the support function
* one of those we will enhance with index ops? If
* so, copy the metadata for the function into
* idxfn and return true. If false... how did the
* support function get added, anyways?
*/
static bool
needsSpatialIndex(Oid funcid, IndexableFunction *idxfn)
{
	const IndexableFunction *idxfns = IndexableFunctions;
	const char *fn_name = get_func_name(funcid);

	do
	{
		if(strcmp(idxfns->fn_name, fn_name) == 0)
		{
			*idxfn = *idxfns;
			return true;
		}
		idxfns++;
	}
	while (idxfns->fn_name);

	return false;
}

/*
* We only add spatial index enhancements for
* indexes that support spatial searches (range
* based searches like the && operator), so only
* implementations based on GIST, SPGIST and BRIN.
*/
static Oid
opFamilyAmOid(Oid opfamilyoid)
{
	Form_pg_opfamily familyform;
	// char *opfamilyname;
	Oid opfamilyam;
	HeapTuple familytup = SearchSysCache1(OPFAMILYOID, ObjectIdGetDatum(opfamilyoid));
	if (!HeapTupleIsValid(familytup))
		elog(ERROR, "cache lookup failed for operator family %u", opfamilyoid);
	familyform = (Form_pg_opfamily) GETSTRUCT(familytup);
	opfamilyam = familyform->opfmethod;
	// opfamilyname = NameStr(familyform->opfname);
	// elog(NOTICE, "%s: found opfamily %s [%u]", __func__, opfamilyname, opfamilyam);
	ReleaseSysCache(familytup);
	return opfamilyam;
}

/*
* To apply the "expand for radius search" pattern
* we need access to the expand function, so lookup
* the function Oid using the function name and
* type number.
*/
static Oid
expandFunctionOid(Oid geotype, Oid callingfunc)
{
	const Oid radiustype = FLOAT8OID; /* Should always be FLOAT8OID */
	const Oid expandfn_args[2] = {geotype, radiustype};
	const bool noError = true;
	/* Expand function must be in same namespace as the caller */
	char *nspname = get_namespace_name(get_func_namespace(callingfunc));
	List *expandfn_name = list_make2(makeString(nspname), makeString("st_expand"));
	Oid expandfn_oid = LookupFuncName(expandfn_name, 2, expandfn_args, noError);
	if (expandfn_oid == InvalidOid)
	{
		/*
		* This is ugly, but we first lookup the geometry variant of expand
		* and if we fail, we look up the geography variant. The alternative
		* is re-naming the geography variant to match the geometry
		* one, which would not be the end of the world.
		*/
		expandfn_name = list_make2(makeString(nspname), makeString("_st_expand"));
		expandfn_oid = LookupFuncName(expandfn_name, 2, expandfn_args, noError);
		if (expandfn_oid == InvalidOid)
			elog(ERROR, "%s: unable to lookup 'st_expand(Oid[%u], Oid[%u])'", __func__, geotype, radiustype);
	}
	return expandfn_oid;
}

/*
* For functions that we want enhanced with spatial
* index lookups, add this support function to the
* SQL function defintion, for example:
*
* CREATE OR REPLACE FUNCTION ST_Intersects(g1 geometry, g2 geometry)
*	RETURNS boolean
*	AS 'MODULE_PATHNAME','ST_Intersects'
*	SUPPORT postgis_index_supportfn
*	LANGUAGE 'c' IMMUTABLE STRICT PARALLEL SAFE
*	COST 100;
*
* The function must also have an entry above in the
* IndexableFunctions array so that we know what
* index search strategy we want to apply.
*/
PG_FUNCTION_INFO_V1(postgis_index_supportfn);
Datum postgis_index_supportfn(PG_FUNCTION_ARGS)
{
	Node *rawreq = (Node *) PG_GETARG_POINTER(0);
	Node *ret = NULL;

	/* The support function need the cache to be populated to know what the type Oids are.
	 * Otherwise it will need look them up dynamically, which only works in the schema where Postgis
	 * is installed is part of the search path (Trac #4739)
	 */
	postgis_initialize_cache(fcinfo);

	if (IsA(rawreq, SupportRequestSelectivity))
	{
		SupportRequestSelectivity *req = (SupportRequestSelectivity *) rawreq;

		if (req->is_join)
		{
			req->selectivity = gserialized_joinsel_internal(req->root, req->args, req->jointype, 2);
		}
		else
		{
			req->selectivity = gserialized_sel_internal(req->root, req->args, req->varRelid, 2);
		}
		POSTGIS_DEBUGF(2, "%s: got selectivity %g", __func__, req->selectivity);
		PG_RETURN_POINTER(req);
	}

	/*
	* This support function is strictly for adding spatial index
	* support.
	*/
	if (IsA(rawreq, SupportRequestIndexCondition))
	{
		SupportRequestIndexCondition *req = (SupportRequestIndexCondition *) rawreq;

		if (is_funcclause(req->node))	/* ST_Something() */
		{
			FuncExpr *clause = (FuncExpr *) req->node;
			Oid funcid = clause->funcid;
			IndexableFunction idxfn = {NULL, 0, 0, 0};
			Oid opfamilyoid = req->opfamily; /* OPERATOR FAMILY of the index */

			if (needsSpatialIndex(funcid, &idxfn))
			{
				int nargs = list_length(clause->args);
				Node *leftarg, *rightarg;
				Oid leftdatatype, rightdatatype, oproid;
				bool swapped = false;

				/*
				* Only add an operator condition for GIST, SPGIST, BRIN indexes.
				* Effectively this means only these opclasses will get automatic
				* indexing when used with one of the indexable functions
				* gist_geometry_ops_2d, gist_geometry_ops_nd,
				* spgist_geometry_ops_2d, spgist_geometry_ops_nd
				*/
				Oid opfamilyam = opFamilyAmOid(opfamilyoid);
				if (opfamilyam != GIST_AM_OID &&
				    opfamilyam != SPGIST_AM_OID &&
				    opfamilyam != BRIN_AM_OID)
				{
					PG_RETURN_POINTER((Node *)NULL);
				}

				/*
				* We can only do something with index matches on the first
				* or second argument.
				*/
				if (req->indexarg > 1)
					PG_RETURN_POINTER((Node *)NULL);

				/*
				* Make sure we have enough arguments.
				*/
				if (nargs < 2 || nargs < idxfn.expand_arg)
					elog(ERROR, "%s: associated with function with %d arguments", __func__, nargs);

				/*
				* Extract "leftarg" as the arg matching
				* the index and "rightarg" as the other, even if
				* they were in the opposite order in the call.
				* NOTE: The functions we deal with here treat
				* their first two arguments symmetrically
				* enough that we needn't distinguish between
				* the two cases beyond this. Could be more
				* complications in the future.
				*/
				if (req->indexarg == 0)
				{
					leftarg = linitial(clause->args);
					rightarg = lsecond(clause->args);
				}
				else
				{
					rightarg = linitial(clause->args);
					leftarg = lsecond(clause->args);
					swapped = true;
				}
				/*
				* Need the argument types (which should always be geometry/geography) as
				* this support function is only ever bound to functions
				* using those types.
				*/
				leftdatatype = exprType(leftarg);
				rightdatatype = exprType(rightarg);

				/*
				* Given the index operator family and the arguments and the
				* desired strategy number we can now lookup the operator
				* we want (usually && or &&&).
				*/
				oproid = get_opfamily_member(opfamilyoid,
							     leftdatatype,
							     rightdatatype,
							     get_strategy_by_type(leftdatatype, idxfn.index));
				if (!OidIsValid(oproid))
					elog(ERROR,
					     "no spatial operator found for '%s': opfamily %u type %d",
					     idxfn.fn_name,
					     opfamilyoid,
					     leftdatatype);

				/*
				* For the ST_DWithin variants we need to build a more complex return.
				* We want to expand the non-indexed side of the call by the
				* radius and then apply the operator.
				* st_dwithin(g1, g2, radius) yields this, if g1 is the indexarg:
				* g1 && st_expand(g2, radius)
				*/
				if (idxfn.expand_arg)
				{
					Expr *expr;
					Node *radiusarg = (Node *) list_nth(clause->args, idxfn.expand_arg-1);
					Oid expandfn_oid = expandFunctionOid(rightdatatype, clause->funcid);

					FuncExpr *expandexpr = makeFuncExpr(expandfn_oid, rightdatatype,
					    list_make2(rightarg, radiusarg),
						InvalidOid, InvalidOid, COERCE_EXPLICIT_CALL);

					/*
					* The comparison expression has to be a pseudo constant,
					* (not volatile or dependent on the target index table)
					*/
					if (!is_pseudo_constant_for_index((Node*)expandexpr, req->index))
						PG_RETURN_POINTER((Node*)NULL);

					/* OK, we can make an index expression */
					expr = make_opclause(oproid, BOOLOID, false,
					              (Expr *) leftarg, (Expr *) expandexpr,
					              InvalidOid, InvalidOid);

					ret = (Node *)(list_make1(expr));
				}
				/*
				* For the ST_Intersects variants we just need to return
				* an index OpExpr with the original arguments on each
				* side.
				* st_intersects(g1, g2) yields: g1 && g2
				*/
				else
				{
					Expr *expr;
					/*
					* The comparison expression has to be a pseudoconstant
					* (not volatile or dependent on the target index's table)
					*/
					if (!is_pseudo_constant_for_index(rightarg, req->index))
						PG_RETURN_POINTER((Node*)NULL);

					/*
					* Arguments were swapped to put the index value on the
					* left, so we need the commutated operator for
					* the OpExpr
					*/
					if (swapped)
					{
						oproid = get_commutator(oproid);
						if (!OidIsValid(oproid))
							PG_RETURN_POINTER((Node *)NULL);
					}

					expr = make_opclause(oproid, BOOLOID, false,
					                (Expr *) leftarg, (Expr *) rightarg,
					                InvalidOid, InvalidOid);

					ret = (Node *)(list_make1(expr));
				}

				/*
				* Set the lossy field on the SupportRequestIndexCondition parameter
				* to indicate that the index alone is not sufficient to evaluate
				* the condition. The function must also still be applied.
				*/
				req->lossy = true;

				PG_RETURN_POINTER(ret);
			}
			else
			{
				elog(WARNING, "support function '%s' called from unsupported spatial function", __func__);
			}
		}
	}

	PG_RETURN_POINTER(ret);
}

#endif /* POSTGIS_PGSQL_VERSION >= 120 */
