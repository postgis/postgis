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

/* Prototypes */
Datum postgis_index_supportfn(PG_FUNCTION_ARGS);


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
	int   strategy_number; /* Index strategy to add */
	int   nargs;           /* Expected number of function arguments */
	int   expand_arg;      /* Radius argument for "within distance" search */
} IndexableFunction;

/*
* Metadata currently scanned from start to back,
* so most common functions first. Could be sorted
* and searched with binary search.
*/
const IndexableFunction IndexableFunctions[] = {
	{"st_intersects", RTOverlapStrategyNumber, 2, 0},
	{"st_dwithin", RTOverlapStrategyNumber, 3, 3},
	{"st_contains", RTContainsStrategyNumber, 2, 0},
	{"st_within", RTContainedByStrategyNumber, 2, 0},
	{"st_touches", RTOverlapStrategyNumber, 2, 0},
	{"st_3dintersects", RTOverlapStrategyNumber, 2, 0},
	{"st_containsproperly", RTOverlapStrategyNumber, 2, 0},
	{"st_coveredby", RTContainedByStrategyNumber, 2, 0},
	{"st_overlaps", RTOverlapStrategyNumber, 2, 0},
	{"st_covers", RTContainsStrategyNumber, 2, 0},
	{"st_crosses", RTOverlapStrategyNumber, 2, 0},
	{"st_dfullywithin", RTOverlapStrategyNumber, 3, 3},
	{"st_3dintersects", RTOverlapStrategyNumber, 3, 3},
	{"st_3ddwithin", RTOverlapStrategyNumber, 3, 3},
	{"st_3ddfullywithin", RTOverlapStrategyNumber, 3, 3},
	{"st_linecrossingdirection", RTOverlapStrategyNumber, 2, 0},
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
				oproid = get_opfamily_member(opfamilyoid, leftdatatype, rightdatatype, idxfn.strategy_number);
				if (!OidIsValid(oproid))
					elog(ERROR, "no spatial operator found for opfamily %u strategy %d", opfamilyoid, idxfn.strategy_number);

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
