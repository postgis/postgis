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

#if POSTGIS_PGSQL_VERSION >= 120

#include "../postgis_config.h"

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
#include "parser/parse_func.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/syscache.h"

/* PostGIS */
#include "liblwgeom.h"


static Oid
getGeographyOid(void)
{
	if (GEOGRAPHYOID == InvalidOid) {
		Oid typoid = TypenameGetTypid("geography");
		if (OidIsValid(typoid) && get_typisdefined(typoid))
		{
			GEOGRAPHYOID = typoid;
		}
	}

	return GEOGRAPHYOID;
}

static Oid
getGeometryOid(void)
{
	if (GEOMETRYOID == InvalidOid) {
		Oid typoid = TypenameGetTypid("geometry");
		if (OidIsValid(typoid) && get_typisdefined(typoid))
		{
			GEOMETRYOID = typoid;
		}
	}

	return GEOMETRYOID;
}

static char*
supportRequestType(NodeTag t)
{
	switch (t)
	{
		case T_SupportRequestSimplify: return "T_SupportRequestSimplify";
		case T_SupportRequestSelectivity: return "T_SupportRequestSelectivity";
		case T_SupportRequestCost: return "T_SupportRequestCost";
		case T_SupportRequestRows: return "T_SupportRequestRows";
		case T_SupportRequestIndexCondition: return "T_SupportRequestIndexCondition";
		default: return "UNKNOWN";
	}
}

typedef struct
{
	char *fn_name;
	int   strategy_number;
	int   nargs;
	int   expand_arg;
} IndexableFunction;


// extern char *get_func_name(Oid funcid);
// extern Oid	get_func_namespace(Oid funcid);
// extern Oid	get_func_rettype(Oid funcid);
// extern int	get_func_nargs(Oid funcid);
// extern Oid	get_func_signature(Oid funcid, Oid **argtypes, int *nargs);
// extern Oid	get_func_variadictype(Oid funcid);
// extern bool get_func_retset(Oid funcid);


const IndexableFunction IndexableFunctions[] = {
	{"st_intersects", RTOverlapStrategyNumber, 2, 0},
	{"st_dwithin", RTOverlapStrategyNumber, 3, 3},
	{"st_contains", RTContainsStrategyNumber, 2, 0},
	{"st_within", RTContainedByStrategyNumber, 2, 0},
	{"st_touches", RTOverlapStrategyNumber, 2, 0},
	{"st_3dintersects", RTOverlapStrategyNumber, 2, 0},
	{"st_containsproperly", RTContainsStrategyNumber, 2, 0},
	{"st_coveredby", RTContainsStrategyNumber, 2, 0},
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

static Oid
expandFunctionOid(Oid geo_datatype)
{
	List *expandfn_name = list_make1(makeString("st_expand"));
	Oid radius_datatype = FLOAT8OID; /* Should always be FLOAT8OID */
	const int expandfn_nargs = 2;
	Oid expandfn_args[expandfn_nargs];
	const bool noError = true;
	Oid expandfn_oid;

	expandfn_args[0] = geo_datatype;
	expandfn_args[1] = radius_datatype;
	expandfn_oid = LookupFuncName(expandfn_name, expandfn_nargs, expandfn_args, noError);
	if (expandfn_oid == InvalidOid)
		elog(ERROR, "%s: unable to lookup 'st_expand(Oid[%u], Oid[%u])'", __func__, geo_datatype, radius_datatype);
	return expandfn_oid;
}

Datum postgis_index_supportfn(PG_FUNCTION_ARGS);
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
		/* Try to convert operator/function call to index conditions */
		SupportRequestIndexCondition *req = (SupportRequestIndexCondition *) rawreq;

		if (is_funcclause(req->node))	/* ST_Something() */
		{
			FuncExpr *clause = (FuncExpr *) req->node;
			Oid funcid = clause->funcid;
			IndexableFunction idxfn = {NULL, 0};
			Oid opfamilyoid = req->opfamily; /* OPERATOR FAMILY of the index */

			if (needsSpatialIndex(funcid, &idxfn))
			{
				Node *indexarg, *otherarg, *radiusarg;
				Oid indexdatatype, otherdatatype;
				Oid oproid;
				int nargs = list_length(clause->args);

				/*
				* Only add an operator condition for GIST and SPGIST indexes.
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
				* Need the argument types (which should always be geometry or geography
				* since this function is only bound to those functions)
				* to use in the operator function lookup
				*/
				if (nargs < 2)
					elog(ERROR, "%s: associated with function with %d arguments", __func__, nargs);
				if (req->indexarg > 1)
					elog(ERROR, "%s: indexarg '%d' out of range", __func__, req->indexarg);

				indexarg = req->indexarg ? lsecond(clause->args) : linitial(clause->args);
				otherarg = req->indexarg ? linitial(clause->args) : lsecond(clause->args);
				indexdatatype = exprType(indexarg);
				otherdatatype = exprType(otherarg);

				/*
				* Given the index operator family and the arguments and the
				* desired strategy number we can now lookup the operator
				* we want (usually && or &&&).
				*/
				oproid = get_opfamily_member(opfamilyoid, indexdatatype, otherdatatype, idxfn.strategy_number);
				if (oproid == InvalidOid)
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
					Node *radiusarg = (Node *) list_nth(clause->args, idxfn.expand_arg-1);
					Oid expandfn_oid = expandFunctionOid(otherdatatype);

					FuncExpr *expandexpr = makeFuncExpr(expandfn_oid, otherdatatype,
					    list_make2(otherarg, radiusarg),
						InvalidOid, req->indexcollation, COERCE_EXPLICIT_CALL);

					Expr *expr = make_opclause(oproid, BOOLOID, false,
					              (Expr *) indexarg, (Expr *) expandexpr,
					              InvalidOid, req->indexcollation);

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
					Expr *expr = make_opclause(oproid, BOOLOID, false,
					                     (Expr *) indexarg, (Expr *) otherarg,
					                     InvalidOid, req->indexcollation);
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
