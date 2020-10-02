
#include "../postgis_config.h"

/* PostgreSQL headers */
#include "postgres.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "access/htup_details.h"
#include "access/transam.h"
#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"
#include "mb/pg_wchar.h"
#include "parser/parse_coerce.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/lsyscache.h"
#include "utils/json.h"
#if POSTGIS_PGSQL_VERSION < 130
#include "utils/jsonapi.h"
#else
#include "common/jsonapi.h"
#endif
#include "utils/typcache.h"
#include "utils/syscache.h"

/* PostGIS headers */
#include "lwgeom_pg.h"
#include "lwgeom_log.h"
#include "liblwgeom.h"


typedef enum					/* type categories for datum_to_json */
{
	JSONTYPE_NULL,				/* null, so we didn't bother to identify */
	JSONTYPE_BOOL,				/* boolean (built-in types only) */
	JSONTYPE_NUMERIC,			/* numeric (ditto) */
	JSONTYPE_DATE,				/* we use special formatting for datetimes */
	JSONTYPE_TIMESTAMP,
	JSONTYPE_TIMESTAMPTZ,
	JSONTYPE_JSON,				/* JSON itself (and JSONB) */
	JSONTYPE_ARRAY,				/* array */
	JSONTYPE_COMPOSITE,			/* composite */
	JSONTYPE_CAST,				/* something with an explicit cast to JSON */
	JSONTYPE_OTHER				/* all else */
} JsonTypeCategory;

static void array_dim_to_json(StringInfo result, int dim, int ndims, int *dims,
				  Datum *vals, bool *nulls, int *valcount,
				  JsonTypeCategory tcategory, Oid outfuncoid,
				  bool use_line_feeds);
static void array_to_json_internal(Datum array, StringInfo result,
								   bool use_line_feeds);
static void composite_to_geojson(FunctionCallInfo fcinfo,
				 Datum composite,
				 char *geom_column_name,
				 int32 maxdecimaldigits,
				 StringInfo result,
				 bool use_line_feeds,
				 Oid geom_oid,
				 Oid geog_oid);
static void composite_to_json(Datum composite, StringInfo result,
							  bool use_line_feeds);
static void datum_to_json(Datum val, bool is_null, StringInfo result,
						  JsonTypeCategory tcategory, Oid outfuncoid,
						  bool key_scalar);
static void json_categorize_type(Oid typoid,
								 JsonTypeCategory *tcategory,
								 Oid *outfuncoid);
static char * postgis_JsonEncodeDateTime(char *buf, Datum value, Oid typid);
static int postgis_time2tm(TimeADT time, struct pg_tm *tm, fsec_t *fsec);
static int postgis_timetz2tm(TimeTzADT *time, struct pg_tm *tm, fsec_t *fsec, int *tzp);

Datum row_to_geojson(PG_FUNCTION_ARGS);
extern Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS);

/*
 * SQL function row_to_geojson(row)
 */
PG_FUNCTION_INFO_V1(ST_AsGeoJsonRow);
Datum
ST_AsGeoJsonRow(PG_FUNCTION_ARGS)
{
	Datum		array = PG_GETARG_DATUM(0);
	text        *geom_column_text = PG_GETARG_TEXT_P(1);
	int32       maxdecimaldigits = PG_GETARG_INT32(2);
	bool		do_pretty = PG_GETARG_BOOL(3);
	StringInfo	result;
	char        *geom_column = text_to_cstring(geom_column_text);
	Oid geom_oid = InvalidOid;
	Oid geog_oid = InvalidOid;

	/* We need to initialize the internal cache to access it later via postgis_oid() */
	postgis_initialize_cache(fcinfo);
	geom_oid = postgis_oid(GEOMETRYOID);
	geog_oid = postgis_oid(GEOGRAPHYOID);

	if (strlen(geom_column) == 0)
		geom_column = NULL;

	result = makeStringInfo();

	composite_to_geojson(fcinfo, array, geom_column, maxdecimaldigits, result, do_pretty, geom_oid, geog_oid);

	PG_RETURN_TEXT_P(cstring_to_text_with_len(result->data, result->len));
}

/*
 * Turn a composite / record into GEOJSON.
 */
static void
composite_to_geojson(FunctionCallInfo fcinfo,
		     Datum composite,
		     char *geom_column_name,
		     int32 maxdecimaldigits,
		     StringInfo result,
		     bool use_line_feeds,
		     Oid geom_oid,
		     Oid geog_oid)
{
	HeapTupleHeader td;
	Oid			tupType;
	int32		tupTypmod;
	TupleDesc	tupdesc;
	HeapTupleData tmptup,
			   *tuple;
	int			i;
	bool		needsep = false;
	const char *sep;
	StringInfo	props = makeStringInfo();
	bool		geom_column_found = false;

	sep = use_line_feeds ? ",\n " : ", ";

	td = DatumGetHeapTupleHeader(composite);

	/* Extract rowtype info and find a tupdesc */
	tupType = HeapTupleHeaderGetTypeId(td);
	tupTypmod = HeapTupleHeaderGetTypMod(td);
	tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

	/* Build a temporary HeapTuple control structure */
	tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
	tmptup.t_data = td;
	tuple = &tmptup;

	appendStringInfoString(result, "{\"type\": \"Feature\", \"geometry\": ");

	for (i = 0; i < tupdesc->natts; i++)
	{
		Datum		val;
		bool		isnull;
		char	   *attname;
		JsonTypeCategory tcategory;
		Oid			outfuncoid;
		Form_pg_attribute att = TupleDescAttr(tupdesc, i);
		bool        is_geom_column = false;

		if (att->attisdropped)
			continue;

		attname = NameStr(att->attname);
		/* Use the column name if provided, use the first geometry column otherwise */
		if (geom_column_name)
			is_geom_column = (strcmp(attname, geom_column_name) == 0);
		else
			is_geom_column = (att->atttypid == geom_oid || att->atttypid == geog_oid);

		if ((!geom_column_found) && is_geom_column)
		{
			/* this is our geom column */
			geom_column_found = true;

			val = heap_getattr(tuple, i + 1, tupdesc, &isnull);
			if (!isnull)
			{
				appendStringInfo(
				    result,
				    "%s",
				    TextDatumGetCString(CallerFInfoFunctionCall2(LWGEOM_asGeoJson,
										 fcinfo->flinfo,
										 InvalidOid,
										 val,
										 Int32GetDatum(maxdecimaldigits))));
			}
			else
			{
				appendStringInfoString(result, "{\"type\": null}");
			}
		}
		else
		{
			if (needsep)
				appendStringInfoString(props, sep);
			needsep = true;

			escape_json(props, attname);
			appendStringInfoString(props, ": ");

			val = heap_getattr(tuple, i + 1, tupdesc, &isnull);

			if (isnull)
			{
				tcategory = JSONTYPE_NULL;
				outfuncoid = InvalidOid;
			}
			else
				json_categorize_type(att->atttypid, &tcategory, &outfuncoid);

			datum_to_json(val, isnull, props, tcategory, outfuncoid, false);
		}
	}

	if (!geom_column_found)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("geometry column is missing")));

	appendStringInfoString(result, ", \"properties\": {");
	appendStringInfo(result, "%s", props->data);

	appendStringInfoString(result, "}}");
	ReleaseTupleDesc(tupdesc);
}

/*
 * The following code was all cut and pasted directly from
 * json.c from the Postgres source tree as of 2019-03-28.
 * It would be far better if these were exported from the
 * backend so we could just use them here. Maybe someday.
 */

/*
 * Determine how we want to print values of a given type in datum_to_json.
 *
 * Given the datatype OID, return its JsonTypeCategory, as well as the type's
 * output function OID.  If the returned category is JSONTYPE_CAST, we
 * return the OID of the type->JSON cast function instead.
 */
static void
json_categorize_type(Oid typoid,
					 JsonTypeCategory *tcategory,
					 Oid *outfuncoid)
{
	bool		typisvarlena;

	/* Look through any domain */
	typoid = getBaseType(typoid);

	*outfuncoid = InvalidOid;

	/*
	 * We need to get the output function for everything except date and
	 * timestamp types, array and composite types, booleans, and non-builtin
	 * types where there's a cast to json.
	 */

	switch (typoid)
	{
		case BOOLOID:
			*tcategory = JSONTYPE_BOOL;
			break;

		case INT2OID:
		case INT4OID:
		case INT8OID:
		case FLOAT4OID:
		case FLOAT8OID:
		case NUMERICOID:
			getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
			*tcategory = JSONTYPE_NUMERIC;
			break;

		case DATEOID:
			*tcategory = JSONTYPE_DATE;
			break;

		case TIMESTAMPOID:
			*tcategory = JSONTYPE_TIMESTAMP;
			break;

		case TIMESTAMPTZOID:
			*tcategory = JSONTYPE_TIMESTAMPTZ;
			break;

		case JSONOID:
		case JSONBOID:
			getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
			*tcategory = JSONTYPE_JSON;
			break;

		default:
			/* Check for arrays and composites */
			if (OidIsValid(get_element_type(typoid)) || typoid == ANYARRAYOID
				|| typoid == RECORDARRAYOID)
				*tcategory = JSONTYPE_ARRAY;
			else if (type_is_rowtype(typoid))	/* includes RECORDOID */
				*tcategory = JSONTYPE_COMPOSITE;
			else
			{
				/* It's probably the general case ... */
				*tcategory = JSONTYPE_OTHER;
				/* but let's look for a cast to json, if it's not built-in */
				if (typoid >= FirstNormalObjectId)
				{
					Oid			castfunc;
					CoercionPathType ctype;

					ctype = find_coercion_pathway(JSONOID, typoid,
												  COERCION_EXPLICIT,
												  &castfunc);
					if (ctype == COERCION_PATH_FUNC && OidIsValid(castfunc))
					{
						*tcategory = JSONTYPE_CAST;
						*outfuncoid = castfunc;
					}
					else
					{
						/* non builtin type with no cast */
						getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
					}
				}
				else
				{
					/* any other builtin type */
					getTypeOutputInfo(typoid, outfuncoid, &typisvarlena);
				}
			}
			break;
	}
}

/*
 * Turn a Datum into JSON text, appending the string to "result".
 *
 * tcategory and outfuncoid are from a previous call to json_categorize_type,
 * except that if is_null is true then they can be invalid.
 *
 * If key_scalar is true, the value is being printed as a key, so insist
 * it's of an acceptable type, and force it to be quoted.
 */
static void
datum_to_json(Datum val, bool is_null, StringInfo result,
			  JsonTypeCategory tcategory, Oid outfuncoid,
			  bool key_scalar)
{
	char	   *outputstr;
	text	   *jsontext;

	check_stack_depth();

	/* callers are expected to ensure that null keys are not passed in */
	Assert(!(key_scalar && is_null));

	if (is_null)
	{
		appendStringInfoString(result, "null");
		return;
	}

	if (key_scalar &&
		(tcategory == JSONTYPE_ARRAY ||
		 tcategory == JSONTYPE_COMPOSITE ||
		 tcategory == JSONTYPE_JSON ||
		 tcategory == JSONTYPE_CAST))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("key value must be scalar, not array, composite, or json")));

	switch (tcategory)
	{
		case JSONTYPE_ARRAY:
			array_to_json_internal(val, result, false);
			break;
		case JSONTYPE_COMPOSITE:
			composite_to_json(val, result, false);
			break;
		case JSONTYPE_BOOL:
			outputstr = DatumGetBool(val) ? "true" : "false";
			if (key_scalar)
				escape_json(result, outputstr);
			else
				appendStringInfoString(result, outputstr);
			break;
		case JSONTYPE_NUMERIC:
			outputstr = OidOutputFunctionCall(outfuncoid, val);

			/*
			 * Don't call escape_json for a non-key if it's a valid JSON
			 * number.
			 */
			if (!key_scalar && IsValidJsonNumber(outputstr, strlen(outputstr)))
				appendStringInfoString(result, outputstr);
			else
				escape_json(result, outputstr);
			pfree(outputstr);
			break;
		case JSONTYPE_DATE:
			{
				char		buf[MAXDATELEN + 1];

				postgis_JsonEncodeDateTime(buf, val, DATEOID);
				appendStringInfo(result, "\"%s\"", buf);
			}
			break;
		case JSONTYPE_TIMESTAMP:
			{
				char		buf[MAXDATELEN + 1];

				postgis_JsonEncodeDateTime(buf, val, TIMESTAMPOID);
				appendStringInfo(result, "\"%s\"", buf);
			}
			break;
		case JSONTYPE_TIMESTAMPTZ:
			{
				char		buf[MAXDATELEN + 1];

				postgis_JsonEncodeDateTime(buf, val, TIMESTAMPTZOID);
				appendStringInfo(result, "\"%s\"", buf);
			}
			break;
		case JSONTYPE_JSON:
			/* JSON and JSONB output will already be escaped */
			outputstr = OidOutputFunctionCall(outfuncoid, val);
			appendStringInfoString(result, outputstr);
			pfree(outputstr);
			break;
		case JSONTYPE_CAST:
			/* outfuncoid refers to a cast function, not an output function */
			jsontext = DatumGetTextPP(OidFunctionCall1(outfuncoid, val));
			outputstr = text_to_cstring(jsontext);
			appendStringInfoString(result, outputstr);
			pfree(outputstr);
			pfree(jsontext);
			break;
		default:
			outputstr = OidOutputFunctionCall(outfuncoid, val);
			escape_json(result, outputstr);
			pfree(outputstr);
			break;
	}
}

/*
 * Turn an array into JSON.
 */
static void
array_to_json_internal(Datum array, StringInfo result, bool use_line_feeds)
{
	ArrayType  *v = DatumGetArrayTypeP(array);
	Oid			element_type = ARR_ELEMTYPE(v);
	int		   *dim;
	int			ndim;
	int			nitems;
	int			count = 0;
	Datum	   *elements;
	bool	   *nulls;
	int16		typlen;
	bool		typbyval;
	char		typalign;
	JsonTypeCategory tcategory;
	Oid			outfuncoid;

	ndim = ARR_NDIM(v);
	dim = ARR_DIMS(v);
	nitems = ArrayGetNItems(ndim, dim);

	if (nitems <= 0)
	{
		appendStringInfoString(result, "[]");
		return;
	}

	get_typlenbyvalalign(element_type,
						 &typlen, &typbyval, &typalign);

	json_categorize_type(element_type,
						 &tcategory, &outfuncoid);

	deconstruct_array(v, element_type, typlen, typbyval,
					  typalign, &elements, &nulls,
					  &nitems);

	array_dim_to_json(result, 0, ndim, dim, elements, nulls, &count, tcategory,
					  outfuncoid, use_line_feeds);

	pfree(elements);
	pfree(nulls);
}

/*
 * Turn a composite / record into JSON.
 */
static void
composite_to_json(Datum composite, StringInfo result, bool use_line_feeds)
{
	HeapTupleHeader td;
	Oid			tupType;
	int32		tupTypmod;
	TupleDesc	tupdesc;
	HeapTupleData tmptup,
			   *tuple;
	int			i;
	bool		needsep = false;
	const char *sep;

	sep = use_line_feeds ? ",\n " : ",";

	td = DatumGetHeapTupleHeader(composite);

	/* Extract rowtype info and find a tupdesc */
	tupType = HeapTupleHeaderGetTypeId(td);
	tupTypmod = HeapTupleHeaderGetTypMod(td);
	tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

	/* Build a temporary HeapTuple control structure */
	tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
	tmptup.t_data = td;
	tuple = &tmptup;

	appendStringInfoChar(result, '{');

	for (i = 0; i < tupdesc->natts; i++)
	{
		Datum		val;
		bool		isnull;
		char	   *attname;
		JsonTypeCategory tcategory;
		Oid			outfuncoid;
		Form_pg_attribute att = TupleDescAttr(tupdesc, i);

		if (att->attisdropped)
			continue;

		if (needsep)
			appendStringInfoString(result, sep);
		needsep = true;

		attname = NameStr(att->attname);
		escape_json(result, attname);
		appendStringInfoChar(result, ':');

		val = heap_getattr(tuple, i + 1, tupdesc, &isnull);

		if (isnull)
		{
			tcategory = JSONTYPE_NULL;
			outfuncoid = InvalidOid;
		}
		else
			json_categorize_type(att->atttypid, &tcategory, &outfuncoid);

		datum_to_json(val, isnull, result, tcategory, outfuncoid, false);
	}

	appendStringInfoChar(result, '}');
	ReleaseTupleDesc(tupdesc);
}

/*
 * Process a single dimension of an array.
 * If it's the innermost dimension, output the values, otherwise call
 * ourselves recursively to process the next dimension.
 */
static void
array_dim_to_json(StringInfo result, int dim, int ndims, int *dims, Datum *vals,
				  bool *nulls, int *valcount, JsonTypeCategory tcategory,
				  Oid outfuncoid, bool use_line_feeds)
{
	int			i;
	const char *sep;

	Assert(dim < ndims);

	sep = use_line_feeds ? ",\n " : ",";

	appendStringInfoChar(result, '[');

	for (i = 1; i <= dims[dim]; i++)
	{
		if (i > 1)
			appendStringInfoString(result, sep);

		if (dim + 1 == ndims)
		{
			datum_to_json(vals[*valcount], nulls[*valcount], result, tcategory,
						  outfuncoid, false);
			(*valcount)++;
		}
		else
		{
			/*
			 * Do we want line feeds on inner dimensions of arrays? For now
			 * we'll say no.
			 */
			array_dim_to_json(result, dim + 1, ndims, dims, vals, nulls,
							  valcount, tcategory, outfuncoid, false);
		}
	}

	appendStringInfoChar(result, ']');
}

static int
postgis_time2tm(TimeADT time, struct pg_tm *tm, fsec_t *fsec)
{
	tm->tm_hour = time / USECS_PER_HOUR;
	time -= tm->tm_hour * USECS_PER_HOUR;
	tm->tm_min = time / USECS_PER_MINUTE;
	time -= tm->tm_min * USECS_PER_MINUTE;
	tm->tm_sec = time / USECS_PER_SEC;
	time -= tm->tm_sec * USECS_PER_SEC;
	*fsec = time;
	return 0;
}

static int
postgis_timetz2tm(TimeTzADT *time, struct pg_tm *tm, fsec_t *fsec, int *tzp)
{
	TimeOffset	trem = time->time;

	tm->tm_hour = trem / USECS_PER_HOUR;
	trem -= tm->tm_hour * USECS_PER_HOUR;
	tm->tm_min = trem / USECS_PER_MINUTE;
	trem -= tm->tm_min * USECS_PER_MINUTE;
	tm->tm_sec = trem / USECS_PER_SEC;
	*fsec = trem - tm->tm_sec * USECS_PER_SEC;

	if (tzp != NULL)
		*tzp = time->zone;

	return 0;
}

static char *
postgis_JsonEncodeDateTime(char *buf, Datum value, Oid typid)
{
	if (!buf)
		buf = palloc(MAXDATELEN + 1);

	switch (typid)
	{
		case DATEOID:
			{
				DateADT		date;
				struct pg_tm tm;

				date = DatumGetDateADT(value);

				/* Same as date_out(), but forcing DateStyle */
				if (DATE_NOT_FINITE(date))
					EncodeSpecialDate(date, buf);
				else
				{
					j2date(date + POSTGRES_EPOCH_JDATE,
						   &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday));
					EncodeDateOnly(&tm, USE_XSD_DATES, buf);
				}
			}
			break;
		case TIMEOID:
			{
				TimeADT		time = DatumGetTimeADT(value);
				struct pg_tm tt,
						   *tm = &tt;
				fsec_t		fsec;

				/* Same as time_out(), but forcing DateStyle */
				postgis_time2tm(time, tm, &fsec);
				EncodeTimeOnly(tm, fsec, false, 0, USE_XSD_DATES, buf);
			}
			break;
		case TIMETZOID:
			{
				TimeTzADT  *time = DatumGetTimeTzADTP(value);
				struct pg_tm tt,
						   *tm = &tt;
				fsec_t		fsec;
				int			tz;

				/* Same as timetz_out(), but forcing DateStyle */
				postgis_timetz2tm(time, tm, &fsec, &tz);
				EncodeTimeOnly(tm, fsec, true, tz, USE_XSD_DATES, buf);
			}
			break;
		case TIMESTAMPOID:
			{
				Timestamp	timestamp;
				struct pg_tm tm;
				fsec_t		fsec;

				timestamp = DatumGetTimestamp(value);
				/* Same as timestamp_out(), but forcing DateStyle */
				if (TIMESTAMP_NOT_FINITE(timestamp))
					EncodeSpecialTimestamp(timestamp, buf);
				else if (timestamp2tm(timestamp, NULL, &tm, &fsec, NULL, NULL) == 0)
					EncodeDateTime(&tm, fsec, false, 0, NULL, USE_XSD_DATES, buf);
				else
					ereport(ERROR,
							(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
							 errmsg("timestamp out of range")));
			}
			break;
		case TIMESTAMPTZOID:
			{
				TimestampTz timestamp;
				struct pg_tm tm;
				int			tz;
				fsec_t		fsec;
				const char *tzn = NULL;

				timestamp = DatumGetTimestampTz(value);
				/* Same as timestamptz_out(), but forcing DateStyle */
				if (TIMESTAMP_NOT_FINITE(timestamp))
					EncodeSpecialTimestamp(timestamp, buf);
				else if (timestamp2tm(timestamp, &tz, &tm, &fsec, &tzn, NULL) == 0)
					EncodeDateTime(&tm, fsec, true, tz, tzn, USE_XSD_DATES, buf);
				else
					ereport(ERROR,
							(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
							 errmsg("timestamp out of range")));
			}
			break;
		default:
			elog(ERROR, "unknown jsonb value datetime type oid %d", typid);
			return NULL;
	}

	return buf;
}
