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
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/syscache.h"

/* PostGIS */
#include "liblwgeom.h"


/* Globals to hold GEOMETRYOID and GEOGRAPHYOID */
Oid GEOMETRYOID  = InvalidOid;
Oid GEOGRAPHYOID = InvalidOid;

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


#if 0
CREATE OR REPLACE FUNCTION geos_intersects_new_support(internal)
	RETURNS internal
	AS '/Users/pramsey/Code/postgis-git/postgis/postgis-3.so','geos_intersects_new_support'
	LANGUAGE 'c' IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION geos_intersects_new(geom1 geometry, geom2 geometry)
	RETURNS boolean
	AS '/Users/pramsey/Code/postgis-git/postgis/postgis-3.so','geos_intersects_new'
	SUPPORT geos_intersects_new_support
	LANGUAGE 'c' IMMUTABLE STRICT PARALLEL SAFE;
#endif

Datum geos_intersects_new(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(geos_intersects_new);
Datum geos_intersects_new(PG_FUNCTION_ARGS)
{
	// Oid geometry_oid = ogrGetGeometryOid();

	elog(NOTICE, "%s", __func__);

	// elog(NOTICE, "GEOMETRYOID = %u", geometry_oid);
	PG_RETURN_BOOL(true);

	// return DirectFunctionCall2(geos_intersects,
	// 	PG_GETARG_DATUM(0),
	// 	PG_GETARG_DATUM(1));
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
	{"st_covers", RTContainsStrategyNumber, 2, 0},
	{"st_crosses", RTOverlapStrategyNumber, 2, 0},
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

Datum postgis_index_supportfn(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(postgis_index_supportfn);
Datum postgis_index_supportfn(PG_FUNCTION_ARGS)
{
	Node *rawreq = (Node *) PG_GETARG_POINTER(0);
	Node *ret = NULL;

	// elog(NOTICE, "%s", __func__);

	// elog(NOTICE, "%s [%d]", supportRequestType(rawreq->type), rawreq->type);

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

			// elog(NOTICE, "req->node == FuncExpr");

			if (needsSpatialIndex(funcid, &idxfn))
			{
				HeapTuple familytup;
				Form_pg_opfamily familyform;
				char *opfamilyname;
				Oid opfamilyam; /* Access method, GIST or SPGIST */
				Node *larg, *rarg;
				Oid ldatatype, rdatatype;
				Oid oproid;

				// elog(NOTICE, "needsSpatialIndex == true");
				// elog(NOTICE, "opfamily == %u", opfamilyoid);
				// elog(NOTICE, "fn_name == %s", idxfn.fn_name);

				/*
				* We need to lookup the operator family, either to test
				* the family name against known families we will support or
				* (currently) to ensure we're only using families
				* that have the search strategies we support (overlaps, etc)
				*/
				familytup = SearchSysCache1(OPFAMILYOID, ObjectIdGetDatum(opfamilyoid));
				if (!HeapTupleIsValid(familytup))
					elog(ERROR, "cache lookup failed for operator family %u", opfamilyoid);
				familyform = (Form_pg_opfamily) GETSTRUCT(familytup);
				opfamilyname = NameStr(familyform->opfname);
				opfamilyam = familyform->opfmethod;
				// elog(NOTICE, "opfamily relname == %s", opfamilyname);
				// elog(NOTICE, "opfamily am = %u", opfamilyam);
				ReleaseSysCache(familytup);

				/*
				* Only add an operator condition for GIST and SPGIST indexes.
				* Effectively this means only these opclasses will get automatic
				* indexing when used with one of the indexable functions
				* gist_geometry_ops_2d, gist_geometry_ops_nd,
				* spgist_geometry_ops_2d, spgist_geometry_ops_nd
				*/
				if (opfamilyam != GIST_AM_OID && opfamilyam != SPGIST_AM_OID && opfamilyam != BRIN_AM_OID)
				{
					PG_RETURN_POINTER((Node *)NULL);
				}

				/*
				* Need the argument types (which should always be geometry or geography
				* since this function is only bound to those functions)
				* to use in the operator function lookup
				*/
				Assert(list_length(clause->args) == 2);
				larg = (Node *) linitial(clause->args);
				rarg = (Node *) lsecond(clause->args);
				ldatatype = exprType(larg);
				rdatatype = exprType(rarg);

				// elog(NOTICE, "ldatatype == %u", ldatatype);
				// elog(NOTICE, "rdatatype == %u", rdatatype);

				/*
				* Given the index operator family and the arguments and the
				* desired strategy number we can now lookup the operator
				* we want (usually && or &&&).
				*/
				oproid = get_opfamily_member(opfamilyoid, ldatatype, rdatatype, idxfn.strategy_number);
				// elog(NOTICE, "oproid == %u", oproid);

				if (oproid == InvalidOid)
					elog(ERROR, "no spatial operator found for opfamily %u", opfamilyoid);

				/*
				* Bind the operator into a operator clause with our unaltered
				* arguments on each side
				*/
				Expr *expr = make_opclause(oproid, BOOLOID, false,
				                     (Expr *) larg, (Expr *) rarg,
				                     InvalidOid, req->indexcollation);
				ret = (Node *)(list_make1(expr));

				/*
				* Set the return field on the SupportRequestIndexCondition parameter
				* to indicate that the index alone is not sufficient to evaluate
				* the condition. The function also must still be applied.
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

