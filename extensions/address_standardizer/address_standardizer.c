#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "catalog/pg_type.h"
#include "utils/memutils.h"
#include "executor/spi.h"
#include "utils/builtins.h"

#undef DEBUG
//#define DEBUG 1

#include "pagc_api.h"
#include "pagc_std_api.h"
#include "std_pg_hash.h"
#include "parseaddress-api.h"

#include "../../postgis_config.h"

#ifdef PG_MODULE_MAGIC_EXT
PG_MODULE_MAGIC_EXT(
	.name = "address_standardizer",
	.version = POSTGIS_LIB_VERSION
	);
#endif

Datum debug_standardize_address(PG_FUNCTION_ARGS);
Datum standardize_address(PG_FUNCTION_ARGS);
Datum standardize_address1(PG_FUNCTION_ARGS);

/*
 * The signature for standardize_address follows. The lextab, gaztab and
 * rultab should not change once the reference has been standardized and
 * the same tables must be used for a geocode request as were used on the
 * reference set or the matching will get degregated.
 *
 *   select * from standardize_address(
 *       lextab text,  -- name of table of view
 *       gaztab text,  -- name of table or view
 *       rultab text,  -- name of table of view
 *       micro text,   -- '123 main st'
 *       macro text);  -- 'boston ma 01002'
 *
 * If you want to standardize a whole table then call it like:
 *
 *   insert into stdaddr (...)
 *       select (std).* from (
 *           select standardize_address(
 *               'lextab', 'gaztab', 'rultab', micro, marco) as std
 *             from table_to_standardize) as foo;
 *
 * The structure of the lextab and gaztab tables of views must be:
 *
 *    seq int4
 *    word text
 *    stdword text
 *    token int4
 *
 * the rultab table or view must have columns:
 *
 *    rule text
*/
PG_FUNCTION_INFO_V1(debug_standardize_address);

Datum
debug_standardize_address(PG_FUNCTION_ARGS)
{
	STANDARDIZER        *std;
	char                *lextab;
	char                *gaztab;
	char                *rultab;
	char                *micro;
	StringInfo macro = makeStringInfo();
	STDADDR             *stdaddr;
	char rule_in[100];
	char rule_out[100];
	char temp[100];
	int stz_no , n ;
	DEF *__def__ ;
	STZ **__stz_list__;
	STAND_PARAM *ms;
	STZ_PARAM *__stz_info__ ;
	int lex_pos;
	int started;
	STZ *__cur_stz__;
	/**start: variables for filtering rules **/
	StringInfo	sql;
	SPIPlanPtr plan = NULL;
	Datum datrul;
	bool isnull;
	Datum values[1];
  Oid argtypes[1];
	int spi_result;
	int spi_conn_ret;
	/**stop: variables for filtering rules **/
	StringInfo	result  = makeStringInfo();

	elog(DEBUG2, "Start %s", __func__);
	initStringInfo(result);

	appendStringInfoChar(result, '{');

	lextab = text_to_cstring(PG_GETARG_TEXT_P(0));
	gaztab = text_to_cstring(PG_GETARG_TEXT_P(1));
	rultab = text_to_cstring(PG_GETARG_TEXT_P(2));
	micro  = text_to_cstring(PG_GETARG_TEXT_P(3));

	if ( (PG_NARGS()  > 4) && (!PG_ARGISNULL(4)) ) {
		initStringInfo(macro);
		appendStringInfo(macro, "%s",  text_to_cstring(PG_GETARG_TEXT_P(4)));
	}
	else {
		ADDRESS             *paddr;
		HHash               *stH;
		int                  err;
		stH = (HHash *) palloc0(sizeof(HHash));
		if (!stH) {
			elog(ERROR, "%s: Failed to allocate memory for hash!", __func__);
			return -1;
		}

		elog(DEBUG1, "going to load_state_hash");

		err = load_state_hash(stH);
		if (err) {
			elog(DEBUG2, "got err=%d from load_state_hash().", err);
#ifdef USE_HSEARCH
			elog(DEBUG2, "calling hdestroy_r(stH).");
			hdestroy_r(stH);
#endif
			elog(ERROR, "standardize_address: load_state_hash() failed(%d)!", err);
			return -1;
		}

		elog(DEBUG2, "calling parseaddress()");
		paddr = parseaddress(stH, micro, &err);

		if (!paddr) {
			elog(ERROR, "parse_address: parseaddress() failed!");
			return -1;
		}

		/* check for errors and comput length of macro string */
		if (paddr->street2)
			elog(ERROR, "standardize_address() can not be passed an intersection.");
		if (! paddr-> address1)
			elog(ERROR, "standardize_address() could not parse the address into components.");


		/* create micro and macro from paddr */
		micro = pstrdup(paddr->address1);
		initStringInfo(macro);

		if (paddr->city) { appendStringInfo(macro, "%s,", paddr->city); }
		if (paddr->st  ) { appendStringInfo(macro, "%s,", paddr->st); }
		if (paddr->zip ) { appendStringInfo(macro, "%s,", paddr->zip); }
		if (paddr->cc  ) { appendStringInfo(macro, "%s,", paddr->cc); }
	}
	appendStringInfoString(result, "\"micro\":");
	appendStringInfo(result, "%s", quote_identifier(micro));
	appendStringInfoString(result, ",");

	appendStringInfoString(result, "\"macro\":");
	appendStringInfo(result, "%s", quote_identifier(macro->data));
	appendStringInfoString(result, ",");

	std = GetStdUsingFCInfo(fcinfo, lextab, gaztab, rultab);
	if (!std){
		elog(ERROR, "%s failed to create the address standardizer object!",  __func__);
	}

	elog(DEBUG2, "%s: calling std_standardize_mm('%s', '%s')", __func__ , micro, macro->data);
    stdaddr = std_standardize_mm( std, micro, macro->data, 0 );

	ms = std->misc_stand;
	__stz_info__ = ms->stz_info ;
	elog(DEBUG2, "%s back from fetch_stdaddr",  __func__);

	elog(DEBUG2, "Input tokenization candidates:\n");
	appendStringInfoString(result, "\"input_tokens\":[");
	started = 0;

	for (lex_pos = FIRST_LEX_POS;lex_pos < ms->LexNum;lex_pos ++)
	{

		for ( __def__ = ms->lex_vector[lex_pos].DefList; __def__ != NULL; __def__ = __def__->Next)
		{
			if (started > 0 ){
				appendStringInfoChar(result, ',');
			}
			appendStringInfo(result, "{\"pos\": %u,", lex_pos);
			appendStringInfoString(result, "\"word\":");
			appendStringInfoString(result, quote_identifier(ms->lex_vector[lex_pos].Text) );
			appendStringInfoString(result, ",\"stdword\":");
			appendStringInfoString(result, quote_identifier(((__def__->Protect )? ms->lex_vector[lex_pos].Text : __def__->Standard)) );
			appendStringInfoString(result, ",\"token\":");
			appendStringInfoString(result, quote_identifier(in_symb_name(__def__->Type)) );
			appendStringInfo(result, ",\"token-code\": %u}", __def__->Type);
			elog(DEBUG2, "\t(%d) stdword: %s, tok: %d (%s)\n",lex_pos,((__def__->Protect )? ms->lex_vector[lex_pos].Text : __def__->Standard),__def__->Type,in_symb_name(__def__->Type));
			started++;

		}
	}
	appendStringInfoChar(result, ']');

	n = __stz_info__->stz_list_size ;
	__stz_list__ = __stz_info__->stz_array ;
	started = 0;

	appendStringInfoString(result, ", \"rules\":[");
	sql = makeStringInfo();
	appendStringInfo(sql, "SELECT id, rule FROM %s ", quote_identifier(rultab));
	appendStringInfoString(sql, "WHERE rule LIKE $1::varchar");
	argtypes[0] = CSTRINGOID;

	spi_conn_ret = 1;
	spi_conn_ret = SPI_connect();
	if (spi_conn_ret != SPI_OK_CONNECT) {
		elog(ERROR, "%s: Could not connect to the SPI manager", __func__);
		return -1;
	};
	plan = SPI_prepare(sql->data, 1, argtypes);
	//plan = SPI_prepare("SELECT id, rule FROM us_rules WHERE rule LIKE '1234%'", 0, NULL);
	if ( ! plan )
    {
      elog(ERROR, "%s: unexpected return (%d) from query preparation: %s, (%d)",
              __func__, SPI_result, sql->data, SPI_ERROR_UNCONNECTED);
      return -1;
    }
	SPI_keepplan(plan);

	for ( stz_no = 0 ; stz_no < n ; stz_no ++ )
	{
		if  (stz_no > 0 ){
			appendStringInfoChar(result, ',');
		}
		strcpy(rule_in, "");
		strcpy(rule_out, "");

		__cur_stz__ = __stz_list__[stz_no] ;
		elog( DEBUG2, "Raw standardization %d with score %f:\n" , ( stz_no  ) , __cur_stz__->score ) ;
		appendStringInfo(result, "{\"score\": %f,", __cur_stz__->score);
		appendStringInfo(result, "\"raw_score\": %f,", __cur_stz__->raw_score);
		appendStringInfo(result, "\"no\": %d,", stz_no);

		appendStringInfoString(result, "\"rule_tokens\":[");
		started = 0;
		for ( lex_pos = FIRST_LEX_POS ; lex_pos < ms->LexNum ; lex_pos ++ )
		{
			SYMB k2;
			__def__ = __cur_stz__->definitions[lex_pos];
			k2 = __cur_stz__->output[lex_pos] ;
			if (started > 0){
				appendStringInfoChar(result, ',');
				strcat(rule_out, " ");
				strcat(rule_in, " ");
			}
			sprintf(temp, "%d", __def__->Type);
			strcat(rule_in, temp);
			sprintf(temp, "%d", k2);
			strcat(rule_out, temp);


			appendStringInfo(result, "{\"pos\": %u,", lex_pos);
			appendStringInfo(result, "\"input-token-code\": %d,",  __def__->Type);
			appendStringInfo(result, "\"input-token\": %s,", quote_identifier( in_symb_name( __def__->Type ) ) );
			appendStringInfo(result, "\"input-word\": %s,", quote_identifier(ms->lex_vector[lex_pos].Text) );
			appendStringInfo(result, "\"mapped-word\": %s,",
				quote_identifier((( __def__->Protect )? ms->lex_vector[lex_pos].Text : __def__->Standard )) );
			appendStringInfo(result, "\"output-token-code\": %d,",  k2);
			appendStringInfo(result, "\"output-token\": %s", quote_identifier( out_symb_name( k2 ) ) );
			appendStringInfoChar(result, '}');
			elog( DEBUG2, "\t(%d) Input %d (%s) text %s mapped to output %d (%s)\n" , lex_pos , __def__->Type , in_symb_name( __def__->Type ) , (( __def__->Protect )? ms->lex_vector[lex_pos].Text : __def__->Standard ) , k2 , (( k2 == FAIL )? "NONE" : out_symb_name( k2 ))) ;
			started++;
			if ( k2 == FAIL ) break ;
		}

		appendStringInfoString(result, "]");
		/* get the sql for the matching rule records */
		strcpy(temp, "");
		strcat(temp, rule_in);
		strcat(temp, " -1 ");
		strcat(temp, rule_out);
		strcat(temp, " -1 %");
		 /* execute */
 		values[0] = CStringGetDatum(temp);
  	spi_result = SPI_execute_plan(plan, values, NULL, true, 1);

		//MemoryContextSwitchTo( oldcontext ); /* switch back */
		if ( spi_result != SPI_OK_SELECT )
		{
			elog(ERROR, "%s: unexpected return (%d) from query execution: %s", __func__, spi_result, sql->data);
			return -1;
		}
		else {

			elog(DEBUG2, "%s: query success, sql: %s, parameter: %s", __func__, sql->data, temp);
		}

		appendStringInfo(result, ", \"rule_stub_string\": %s", quote_identifier(temp));

		if (SPI_processed > 0 && SPI_tuptable != NULL) {
			elog(DEBUG2, "%s: Processing results for: %s, rule: %s", __func__, sql->data, temp );
			datrul = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
			if ( isnull )
			{
				elog(NOTICE, "%s: No match %s",  __func__, sql->data);
				SPI_freetuptable(SPI_tuptable);
				return -1;
			}


			appendStringInfo(result, ", \"rule_id\": %d",DatumGetInt32(datrul));
			datrul = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 2, &isnull);
			//char *srule = TextDatumGetCString(datrul);
			appendStringInfo(result, ", \"rule_string\": %s", quote_identifier(TextDatumGetCString(datrul)));
			//appendStringInfoString(result, " -1 ");
			//appendStringInfo(result, "%s",rule_out);
			SPI_freetuptable(SPI_tuptable);
		}
		else {
			/** TODO: Figure out why this happens a lot **/
			elog(DEBUG2, "%s: no match found for, sql: %s, parameter: %s", __func__, sql->data, temp);
			appendStringInfoString(result, ", \"rule_id\": -1");
		}

		/**
		 * TODO:  Add type and weight of rule (parsed from rulestring)
		 * **/
		//appendStringInfo(result, " %d", ruleref->Type);
		//elog(DEBUG2, "Rule  type  %d",  ruleref->Type);
		/** rule weight **/
		//appendStringInfo(result, " %d", ruleref->Weight);

		appendStringInfoString(result, "}");
	}
	SPI_finish();
	appendStringInfoChar(result, ']');
	elog(DEBUG2, "%s: setup values json", __func__);
	appendStringInfoString(result, ",\"stdaddr\": {");
	if (stdaddr) {
			appendStringInfo(result, "\"building\": %s", (stdaddr->building ?
			quote_identifier(pstrdup(stdaddr->building)) : "null") );

		appendStringInfo(result, ",\"house_num\": %s", (stdaddr->house_num ?
				quote_identifier(pstrdup(stdaddr->house_num)) : "null") );

		appendStringInfo(result, ",\"predir\": %s", (stdaddr->predir ?
				quote_identifier(pstrdup(stdaddr->predir)) : "null") );

		appendStringInfo(result, ",\"qual\": %s", (stdaddr->qual ?
				quote_identifier(pstrdup(stdaddr->qual)) : "null") );

		appendStringInfo(result, ",\"pretype\": %s", (stdaddr->pretype ?
				quote_identifier(pstrdup(stdaddr->pretype)) : "null") );

		appendStringInfo(result, ",\"name\": %s", (stdaddr->name ?
				quote_identifier(pstrdup(stdaddr->name)) : "null") );

		appendStringInfo(result, ",\"suftype\": %s", (stdaddr->suftype ?
				quote_identifier(pstrdup(stdaddr->suftype)) : "null") );

		appendStringInfo(result, ",\"sufdir\": %s", (stdaddr->sufdir ?
				quote_identifier(pstrdup(stdaddr->sufdir)) : "null") );

		appendStringInfo(result, ",\"ruralroute\": %s", (stdaddr->ruralroute ?
			quote_identifier(pstrdup(stdaddr->ruralroute)) : "null") );

		appendStringInfo(result, ",\"extra\": %s", (stdaddr->extra ?
		quote_identifier(pstrdup(stdaddr->extra)) : "null") );

		appendStringInfo(result, ",\"city\": %s", (stdaddr->city ?
			quote_identifier(pstrdup(stdaddr->city)) : "null") );

		appendStringInfo(result, ",\"state\": %s", (stdaddr->state ?
			quote_identifier(pstrdup(stdaddr->state)) : "null") );

		appendStringInfo(result, ",\"country\": %s", (stdaddr->country ?
			quote_identifier(pstrdup(stdaddr->country)) : "null") );

		appendStringInfo(result, ",\"postcode\": %s", (stdaddr->postcode ?
			quote_identifier(pstrdup(stdaddr->postcode)) : "null") );

		appendStringInfo(result, ",\"box\": %s", (stdaddr->box ?
			quote_identifier(pstrdup(stdaddr->box)) : "null") );

		appendStringInfo(result, ",\"unit\": %s", (stdaddr->unit ?
			quote_identifier(pstrdup(stdaddr->unit)) : "null") );
		}
		stdaddr_free(stdaddr);
		appendStringInfoString(result, "}");
		appendStringInfoString(result, "}");
		PG_RETURN_TEXT_P(cstring_to_text_with_len(result->data, result->len));
}

PG_FUNCTION_INFO_V1(standardize_address);

Datum standardize_address(PG_FUNCTION_ARGS)
{
    TupleDesc            tuple_desc;
    AttInMetadata       *attinmeta;
    STANDARDIZER        *std;
    char                *lextab;
    char                *gaztab;
    char                *rultab;
    char                *micro;
    char                *macro;
    Datum                result;
    STDADDR             *stdaddr;
    char               **values;
    int                  k;
    HeapTuple            tuple;

    DBG("Start standardize_address");

    lextab = text_to_cstring(PG_GETARG_TEXT_P(0));
    gaztab = text_to_cstring(PG_GETARG_TEXT_P(1));
    rultab = text_to_cstring(PG_GETARG_TEXT_P(2));
    micro  = text_to_cstring(PG_GETARG_TEXT_P(3));
    macro  = text_to_cstring(PG_GETARG_TEXT_P(4));

    DBG("calling RelationNameGetTupleDesc");
    if (get_call_result_type( fcinfo, NULL, &tuple_desc ) != TYPEFUNC_COMPOSITE ) {
        elog(ERROR, "standardize_address() was called in a way that cannot accept record as a result");
    }
    BlessTupleDesc(tuple_desc);
    attinmeta = TupleDescGetAttInMetadata(tuple_desc);

    DBG("calling GetStdUsingFCInfo(fcinfo, '%s', '%s', '%s')", lextab, gaztab, rultab);
    std = GetStdUsingFCInfo(fcinfo, lextab, gaztab, rultab);
    if (!std)
        elog(ERROR, "standardize_address() failed to create the address standardizer object!");

    DBG("calling std_standardize_mm('%s', '%s')", micro, macro);
    stdaddr = std_standardize_mm( std, micro, macro, 0 );

    DBG("back from fetch_stdaddr");

    values = (char **) palloc(16 * sizeof(char *));
    for (k=0; k<16; k++) {
        values[k] = NULL;
    }
    DBG("setup values array for natts=%d", tuple_desc->natts);
    if (stdaddr) {
        values[0] = stdaddr->building   ? pstrdup(stdaddr->building) : NULL;
        values[1] = stdaddr->house_num  ? pstrdup(stdaddr->house_num) : NULL;
        values[2] = stdaddr->predir     ? pstrdup(stdaddr->predir) : NULL;
        values[3] = stdaddr->qual       ? pstrdup(stdaddr->qual) : NULL;
        values[4] = stdaddr->pretype    ? pstrdup(stdaddr->pretype) : NULL;
        values[5] = stdaddr->name       ? pstrdup(stdaddr->name) : NULL;
        values[6] = stdaddr->suftype    ? pstrdup(stdaddr->suftype) : NULL;
        values[7] = stdaddr->sufdir     ? pstrdup(stdaddr->sufdir) : NULL;
        values[8] = stdaddr->ruralroute ? pstrdup(stdaddr->ruralroute) : NULL;
        values[9] = stdaddr->extra      ? pstrdup(stdaddr->extra) : NULL;
        values[10] = stdaddr->city      ? pstrdup(stdaddr->city) : NULL;
        values[11] = stdaddr->state     ? pstrdup(stdaddr->state) : NULL;
        values[12] = stdaddr->country   ? pstrdup(stdaddr->country) : NULL;
        values[13] = stdaddr->postcode  ? pstrdup(stdaddr->postcode) : NULL;
        values[14] = stdaddr->box       ? pstrdup(stdaddr->box) : NULL;
        values[15] = stdaddr->unit      ? pstrdup(stdaddr->unit) : NULL;
    }

    DBG("calling heap_form_tuple");
    tuple = BuildTupleFromCStrings(attinmeta, values);

    /* make the tuple into a datum */
    DBG("calling HeapTupleGetDatum");
    result = HeapTupleGetDatum(tuple);

    /* clean up (this is not really necessary */
    DBG("freeing values, nulls, and stdaddr");
    stdaddr_free(stdaddr);

    DBG("returning standardized result");
    PG_RETURN_DATUM(result);
}


PG_FUNCTION_INFO_V1(standardize_address1);

Datum standardize_address1(PG_FUNCTION_ARGS)
{
    TupleDesc            tuple_desc;
    AttInMetadata       *attinmeta;
    STANDARDIZER        *std;
    char                *lextab;
    char                *gaztab;
    char                *rultab;
    char                *addr;
    char                *micro;
		StringInfo macro = makeStringInfo();
    Datum                result;
    STDADDR             *stdaddr;
    char               **values;
    int                  k;
    HeapTuple            tuple;
    ADDRESS             *paddr;
    HHash               *stH;
    int                  err;

    DBG("Start standardize_address");

    lextab = text_to_cstring(PG_GETARG_TEXT_P(0));
    gaztab = text_to_cstring(PG_GETARG_TEXT_P(1));
    rultab = text_to_cstring(PG_GETARG_TEXT_P(2));
    addr   = text_to_cstring(PG_GETARG_TEXT_P(3));

    DBG("calling RelationNameGetTupleDesc");
    if (get_call_result_type( fcinfo, NULL, &tuple_desc ) != TYPEFUNC_COMPOSITE ) {
        elog(ERROR, "standardize_address() was called in a way that cannot accept record as a result");
    }
    BlessTupleDesc(tuple_desc);
    attinmeta = TupleDescGetAttInMetadata(tuple_desc);

    DBG("Got tupdesc, allocating HHash");

    stH = (HHash *) palloc0(sizeof(HHash));
    if (!stH) {
         elog(ERROR, "standardize_address: Failed to allocate memory for hash!");
         return -1;
    }

    DBG("going to load_state_hash");

    err = load_state_hash(stH);
    if (err) {
        DBG("got err=%d from load_state_hash().", err);
#ifdef USE_HSEARCH
        DBG("calling hdestroy_r(stH).");
        hdestroy_r(stH);
#endif
        elog(ERROR, "standardize_address: load_state_hash() failed(%d)!", err);
        return -1;
    }

    DBG("calling parseaddress()");
    paddr = parseaddress(stH, addr, &err);
    if (!paddr) {
        elog(ERROR, "parse_address: parseaddress() failed!");
        return -1;
    }

    /* check for errors and comput length of macro string */
    if (paddr->street2)
        elog(ERROR, "standardize_address() can not be passed an intersection.");
    if (! paddr-> address1)
        elog(ERROR, "standardize_address() could not parse the address into components.");


    /* create micro and macro from paddr */
    micro = pstrdup(paddr->address1);
		initStringInfo(macro);

		if (paddr->city) { appendStringInfo(macro, "%s,", paddr->city); }
		if (paddr->st  ) { appendStringInfo(macro, "%s,", paddr->st); }
		if (paddr->zip ) { appendStringInfo(macro, "%s,", paddr->zip); }
		if (paddr->cc  ) { appendStringInfo(macro, "%s,", paddr->cc); }

    DBG("calling GetStdUsingFCInfo(fcinfo, '%s', '%s', '%s')", lextab, gaztab, rultab);
    std = GetStdUsingFCInfo(fcinfo, lextab, gaztab, rultab);
    if (!std)
        elog(ERROR, "standardize_address() failed to create the address standardizer object!");

    DBG("calling std_standardize_mm('%s', '%s')", micro, macro->data);
    stdaddr = std_standardize_mm( std, micro, macro->data, 0 );

    DBG("back from fetch_stdaddr");

    values = (char **) palloc(16 * sizeof(char *));
    for (k=0; k<16; k++) {
        values[k] = NULL;
    }
    DBG("setup values array for natts=%d", tuple_desc->natts);
    if (stdaddr) {
        values[0] = stdaddr->building   ? pstrdup(stdaddr->building) : NULL;
        values[1] = stdaddr->house_num  ? pstrdup(stdaddr->house_num) : NULL;
        values[2] = stdaddr->predir     ? pstrdup(stdaddr->predir) : NULL;
        values[3] = stdaddr->qual       ? pstrdup(stdaddr->qual) : NULL;
        values[4] = stdaddr->pretype    ? pstrdup(stdaddr->pretype) : NULL;
        values[5] = stdaddr->name       ? pstrdup(stdaddr->name) : NULL;
        values[6] = stdaddr->suftype    ? pstrdup(stdaddr->suftype) : NULL;
        values[7] = stdaddr->sufdir     ? pstrdup(stdaddr->sufdir) : NULL;
        values[8] = stdaddr->ruralroute ? pstrdup(stdaddr->ruralroute) : NULL;
        values[9] = stdaddr->extra      ? pstrdup(stdaddr->extra) : NULL;
        values[10] = stdaddr->city      ? pstrdup(stdaddr->city) : NULL;
        values[11] = stdaddr->state     ? pstrdup(stdaddr->state) : NULL;
        values[12] = stdaddr->country   ? pstrdup(stdaddr->country) : NULL;
        values[13] = stdaddr->postcode  ? pstrdup(stdaddr->postcode) : NULL;
        values[14] = stdaddr->box       ? pstrdup(stdaddr->box) : NULL;
        values[15] = stdaddr->unit      ? pstrdup(stdaddr->unit) : NULL;
    }

    DBG("calling heap_form_tuple");
    tuple = BuildTupleFromCStrings(attinmeta, values);

    /* make the tuple into a datum */
    DBG("calling HeapTupleGetDatum");
    result = HeapTupleGetDatum(tuple);

    /* clean up (this is not really necessary */
    DBG("freeing values, nulls, and stdaddr");
    stdaddr_free(stdaddr);

    DBG("freeing values, hash, and paddr");
    free_state_hash(stH);

    DBG("returning standardized result");
    PG_RETURN_DATUM(result);
}


