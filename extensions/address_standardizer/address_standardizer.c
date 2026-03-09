/*
 * Copyright (c) 2026 Darafei Praliaskouski <me@komzpa.net>
 */

#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "catalog/pg_type.h"
#include "utils/memutils.h"
#include "executor/spi.h"
#include "utils/builtins.h"

#undef DEBUG
// #define DEBUG 1

#include "pagc_api.h"
#include "pagc_std_api.h"
#include "std_pg_hash.h"
#include "parseaddress-api.h"

#include "../../postgis_config.h"

#ifdef PG_MODULE_MAGIC_EXT
PG_MODULE_MAGIC_EXT(.name = "address_standardizer", .version = POSTGIS_LIB_VERSION);
#else
PG_MODULE_MAGIC;
#endif

Datum debug_standardize_address(PG_FUNCTION_ARGS);
Datum standardize_address(PG_FUNCTION_ARGS);
Datum standardize_address1(PG_FUNCTION_ARGS);

/*
 * The debug serializer walks partially constructed candidates. These helpers
 * keep that path descriptive and non-fatal even when a candidate is sparse.
 */
static const char *
debug_effective_standardized_word(const DEF *definition, const char *input_word)
{
	if (!input_word)
		return NULL;

	if (!definition || definition->Protect || !definition->Standard)
		return input_word;

	return definition->Standard;
}

/* Debug output should label invalid input symbols instead of crashing. */
static const char *
debug_safe_input_symbol_name(SYMB input_symbol)
{
	const char *input_name;

	if (input_symbol < 0 || input_symbol >= MAXINSYM)
		return "INVALID";

	input_name = in_symb_name(input_symbol);
	return input_name ? input_name : "NONE";
}

static const char *
debug_safe_output_symbol_name(SYMB output_symbol)
{
	const char *output_name;

	/*
	 * FAIL terminates a candidate path; it is not part of the printable
	 * output-symbol table. The original analyzer debug output rendered this
	 * sentinel as "NONE".
	 */
	if (output_symbol == FAIL)
		return "NONE";

	if (output_symbol < 0 || output_symbol >= MAXOUTSYM)
		return "INVALID";

	output_name = out_symb_name(output_symbol);
	return output_name ? output_name : "NONE";
}

/* Mirror the analyzer's rule-type labels in the JSON debug output. */
static const char *
debug_rule_type_name(SYMB rule_type)
{
	switch (rule_type)
	{
	case MACRO_C:
		return "MACRO";
	case MICRO_C:
		return "MICRO";
	case ARC_C:
		return "ARC";
	case CIVIC_C:
		return "CIVIC";
	case EXTRA_C:
		return "EXTRA";
	default:
		return "NONE";
	}
}

/*
 * The debug rule lookup returns the stored rule text. Parse the trailing
 * "<rule_type> <rule_weight>" pair from that string so the JSON does not
 * depend on internal analyzer pointers.
 */
static bool
debug_parse_rule_metadata(const char *rule_string, SYMB *rule_type, SYMB *rule_weight)
{
	char *end_ptr;
	long parsed_value;
	SYMB penultimate_value = FAIL;
	SYMB last_value = FAIL;
	size_t value_count = 0;

	if (!rule_string || !rule_type || !rule_weight)
		return false;

	/* The stored rule text ends with "... <rule_type> <rule_weight>". */
	while (*rule_string != '\0')
	{
		while (*rule_string == ' ')
			rule_string++;

		if (*rule_string == '\0')
			break;

		parsed_value = strtol(rule_string, &end_ptr, 10);
		if (end_ptr == rule_string)
			return false;

		penultimate_value = last_value;
		last_value = (SYMB)parsed_value;
		value_count++;
		rule_string = end_ptr;
	}

	if (value_count < 2)
		return false;

	*rule_type = penultimate_value;
	*rule_weight = last_value;
	return true;
}

/* Keep rule_string even when the trailing metadata is absent or malformed. */
static void
debug_append_rule_metadata(StringInfo result, const char *matched_rule_string)
{
	SYMB rule_type;
	SYMB rule_weight;

	appendStringInfo(result, ", \"rule_string\": %s", quote_identifier(matched_rule_string));

	if (debug_parse_rule_metadata(matched_rule_string, &rule_type, &rule_weight))
	{
		const char *rule_type_name = debug_rule_type_name(rule_type);

		appendStringInfo(result,
				 ", \"rule_type_code\": %d"
				 ", \"rule_type\": %s"
				 ", \"rule_weight\": %d",
				 rule_type,
				 quote_identifier(rule_type_name),
				 rule_weight);
		return;
	}

	appendStringInfoString(result,
			       ", \"rule_type_code\": null"
			       ", \"rule_type\": null"
			       ", \"rule_weight\": null");
}

/* Emit every lexical interpretation that the standardizer considered. */
static void
debug_append_input_tokens_json(StringInfo result, const STAND_PARAM *stand_param)
{
	bool need_comma = false;

	appendStringInfoString(result, "\"input_tokens\":[");
	for (int lex_pos = FIRST_LEX_POS; lex_pos < stand_param->LexNum; lex_pos++)
	{
		const char *input_word = stand_param->lex_vector[lex_pos].Text;

		for (DEF *definition = stand_param->lex_vector[lex_pos].DefList; definition;
		     definition = definition->Next)
		{
			const char *input_token = debug_safe_input_symbol_name(definition->Type);
			const char *mapped_word = debug_effective_standardized_word(definition, input_word);

			if (!mapped_word)
				continue;

			if (need_comma)
				appendStringInfoChar(result, ',');

			appendStringInfo(result,
					 "{\"pos\": %d,\"word\":%s,\"stdword\":%s,\"token\":%s,\"token-code\": %d}",
					 lex_pos,
					 quote_identifier(input_word),
					 quote_identifier(mapped_word),
					 quote_identifier(input_token),
					 definition->Type);
			elog(DEBUG2,
			     "\t(%d) stdword: %s, tok: %d (%s)\n",
			     lex_pos,
			     mapped_word,
			     definition->Type,
			     input_token);
			need_comma = true;
		}
	}
	appendStringInfoChar(result, ']');
}

/*
 * Build the emitted rule token list and the compact input/output sequences
 * used for rule-table lookup. Sparse candidates remain visible in the debug
 * output, but the returned flag lets the caller suppress untrustworthy lookup.
 */
static bool
debug_append_candidate_rule_tokens_json(StringInfo result,
					StringInfo rule_in,
					StringInfo rule_out,
					const STAND_PARAM *stand_param,
					const STZ *candidate)
{
	bool need_comma = false;
	bool candidate_has_sparse_tokens = false;

	resetStringInfo(rule_in);
	resetStringInfo(rule_out);
	appendStringInfoString(result, "\"rule_tokens\":[");
	for (int lex_pos = FIRST_LEX_POS; lex_pos < stand_param->LexNum; lex_pos++)
	{
		DEF *definition = candidate->definitions[lex_pos];
		SYMB output_symbol = candidate->output[lex_pos];
		const char *input_word = stand_param->lex_vector[lex_pos].Text;
		const char *input_token;
		const char *mapped_word;
		const char *output_token;

		/*
		 * Debug output should never crash the backend if a candidate has
		 * a sparse definition array or an unset token text.
		 */
		if (!definition || !input_word)
		{
			candidate_has_sparse_tokens = true;
			continue;
		}

		input_token = debug_safe_input_symbol_name(definition->Type);
		mapped_word = debug_effective_standardized_word(definition, input_word);
		output_token = debug_safe_output_symbol_name(output_symbol);

		if (need_comma)
		{
			appendStringInfoChar(result, ',');
			appendStringInfoChar(rule_in, ' ');
			appendStringInfoChar(rule_out, ' ');
		}
		appendStringInfo(rule_in, "%d", definition->Type);
		appendStringInfo(rule_out, "%d", output_symbol);

		appendStringInfo(result,
				 "{\"pos\": %d,\"input-token-code\": %d,\"input-token\": %s,"
				 "\"input-word\": %s,\"mapped-word\": %s,\"output-token-code\": %d,"
				 "\"output-token\": %s}",
				 lex_pos,
				 definition->Type,
				 quote_identifier(debug_safe_input_symbol_name(definition->Type)),
				 quote_identifier(input_word),
				 quote_identifier(mapped_word),
				 output_symbol,
				 quote_identifier(debug_safe_output_symbol_name(output_symbol)));
		elog(DEBUG2,
		     "\t(%d) Input %d (%s) text %s mapped to output %d (%s)\n",
		     lex_pos,
		     definition->Type,
		     input_token,
		     mapped_word,
		     output_symbol,
		     output_token);
		need_comma = true;
		if (output_symbol == FAIL)
			break;
	}
	appendStringInfoChar(result, ']');
	return candidate_has_sparse_tokens;
}

/* Attach the best matching stored rule row for a fully described candidate. */
static void
debug_lookup_matching_rule_json(StringInfo result,
				const char *function_name,
				const char *rule_sql,
				SPIPlanPtr rule_plan,
				StringInfo rule_stub,
				StringInfo rule_in,
				StringInfo rule_out)
{
	Datum rule_values[1];

	resetStringInfo(rule_stub);
	appendStringInfo(rule_stub, "%s -1 %s -1 %%", rule_in->data, rule_out->data);
	appendStringInfo(result, ", \"rule_stub_string\": %s", quote_identifier(rule_stub->data));

	/*
	 * Match the candidate token sequence against the rule table. The pattern
	 * is reliable once the candidate sequence is complete, whether it stopped
	 * on FAIL or by consuming all input lexemes.
	 */
	rule_values[0] = CStringGetDatum(rule_stub->data);
	if (SPI_execute_plan(rule_plan, rule_values, NULL, true, 1) != SPI_OK_SELECT)
	{
		elog(ERROR, "%s: unexpected return (%d) from query execution: %s", function_name, SPI_result, rule_sql);
		return;
	}

	elog(DEBUG2, "%s: query success, sql: %s, parameter: %s", function_name, rule_sql, rule_stub->data);

	if (SPI_processed > 0 && SPI_tuptable)
	{
		char *matched_rule_string;
		Datum rule_value;
		bool isnull;

		elog(DEBUG2, "%s: Processing results for: %s, rule: %s", function_name, rule_sql, rule_stub->data);
		rule_value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
		if (isnull)
		{
			elog(ERROR, "%s: rule lookup returned a NULL id for %s", function_name, rule_sql);
			SPI_freetuptable(SPI_tuptable);
			return;
		}

		appendStringInfo(result, ", \"rule_id\": %d", DatumGetInt32(rule_value));
		rule_value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 2, &isnull);
		matched_rule_string = TextDatumGetCString(rule_value);
		debug_append_rule_metadata(result, matched_rule_string);
		SPI_freetuptable(SPI_tuptable);
		return;
	}

	/*
	 * Some candidate paths do not correspond to a stored rule row. Keep the
	 * candidate in the debug stream and mark the lookup miss explicitly.
	 */
	elog(DEBUG2, "%s: no match found for, sql: %s, parameter: %s", function_name, rule_sql, rule_stub->data);
	appendStringInfoString(result, ", \"rule_id\": -1");
	appendStringInfoString(result,
			       ", \"rule_string\": null"
			       ", \"rule_type_code\": null"
			       ", \"rule_type\": null"
			       ", \"rule_weight\": null");
}

typedef struct StdAddrField {
	const char *json_name;
	size_t offset;
} StdAddrField;

/*
 * Keep the debug JSON and composite tuple materialization in lockstep by
 * describing the STDADDR layout once, next to the code that emits it.
 */
static const StdAddrField stdaddr_fields[] = {
    {"building", offsetof(STDADDR, building)},
    {"house_num", offsetof(STDADDR, house_num)},
    {"predir", offsetof(STDADDR, predir)},
    {"qual", offsetof(STDADDR, qual)},
    {"pretype", offsetof(STDADDR, pretype)},
    {"name", offsetof(STDADDR, name)},
    {"suftype", offsetof(STDADDR, suftype)},
    {"sufdir", offsetof(STDADDR, sufdir)},
    {"ruralroute", offsetof(STDADDR, ruralroute)},
    {"extra", offsetof(STDADDR, extra)},
    {"city", offsetof(STDADDR, city)},
    {"state", offsetof(STDADDR, state)},
    {"country", offsetof(STDADDR, country)},
    {"postcode", offsetof(STDADDR, postcode)},
    {"box", offsetof(STDADDR, box)},
    {"unit", offsetof(STDADDR, unit)},
};

static const char *
stdaddr_field_value(const STDADDR *stdaddr, size_t offset)
{
	const char *const *field_value;
	const char *canonical_country;

	if (!stdaddr)
		return NULL;

	field_value = (const char *const *)((const char *)stdaddr + offset);
	if (offset == offsetof(STDADDR, country) && *field_value)
	{
		canonical_country = country_code_from_name(*field_value);
		if (canonical_country)
			return canonical_country;
	}

	return *field_value;
}

/* Render the normalized address in the same field order as tuple output. */
static void
append_debug_stdaddr_json(StringInfo result, const STDADDR *stdaddr)
{
	bool need_comma = false;

	appendStringInfoString(result, ",\"stdaddr\": {");
	for (size_t i = 0; i < lengthof(stdaddr_fields); i++)
	{
		const char *field_value = stdaddr_field_value(stdaddr, stdaddr_fields[i].offset);

		if (need_comma)
			appendStringInfoChar(result, ',');

		appendStringInfo(result,
				 "\"%s\": %s",
				 stdaddr_fields[i].json_name,
				 field_value ? quote_identifier(field_value) : "null");
		need_comma = true;
	}
	appendStringInfoChar(result, '}');
}

/*
 * Parse an address-like string with the state hash loaded. parseaddress()
 * rewrites its input buffer, so always hand it a private copy.
 */
static ADDRESS *
parse_address_input(const char *function_name, const char *raw_address)
{
	ADDRESS *parsed_address;
	char *parse_input;
	HHash state_hash;
	int err;

	memset(&state_hash, 0, sizeof(state_hash));

	err = load_state_hash(&state_hash);
	if (err)
	{
		free_state_hash(&state_hash);
		elog(ERROR, "%s: load_state_hash() failed(%d)!", function_name, err);
		return NULL;
	}

	parse_input = raw_address ? pstrdup(raw_address) : NULL;
	parsed_address = parseaddress(&state_hash, parse_input, &err);
	free_state_hash(&state_hash);
	if (parse_input)
		pfree(parse_input);
	if (!parsed_address)
	{
		elog(ERROR, "%s: parseaddress() failed!", function_name);
		return NULL;
	}

	return parsed_address;
}

/*
 * Shared input preparation for the one-string SQL entry points. It returns the
 * parsed ADDRESS, materializes the micro string, and optionally renders a
 * macro display string for debug output.
 */
static ADDRESS *
parse_address_wrapper_input(const char *function_name, const char *raw_address, char **micro_out, StringInfo macro_out)
{
	ADDRESS *parsed_address = parse_address_input(function_name, raw_address);

	if (parsed_address->street2)
		elog(ERROR, "standardize_address() can not be passed an intersection.");
	if (!parsed_address->address1)
		elog(ERROR, "standardize_address() could not parse the address into components.");

	*micro_out = pstrdup(parsed_address->address1);
	if (macro_out)
	{
		const char *components[] = {
		    parsed_address->city,
		    parsed_address->st,
		    parsed_address->zip,
		    parsed_address->cc,
		};

		resetStringInfo(macro_out);
		for (size_t i = 0; i < lengthof(components); i++)
		{
			if (components[i] && components[i][0] != '\0')
				appendStringInfo(macro_out, "%s,", components[i]);
		}
	}
	return parsed_address;
}

/*
 * Preserve ZIP+4 details from structured parses so callers can reuse the
 * standardized postcode without reparsing the original address text.
 */
static char *
build_postcode_component(const ADDRESS *parsed_address)
{
	if (!parsed_address || !parsed_address->zip || parsed_address->zip[0] == '\0')
		return NULL;

	if (parsed_address->zipplus && parsed_address->zipplus[0] != '\0')
		return psprintf("%s-%s", parsed_address->zip, parsed_address->zipplus);

	return pstrdup(parsed_address->zip);
}

/*
 * The explicit SQL macro argument models city/state/postcode text, not a full
 * address. parseaddress() mostly gives us that split already, but when a city
 * prefix is mistaken for address1 (for example "ST PAUL, MN 55105"), fold it
 * back into the city component. Also ignore parseaddress()'s default country
 * inference here, because the caller did not supply a country field.
 */
static ADDRESS *
parse_macro_input(const char *function_name, const char *raw_macro)
{
	ADDRESS *parsed_macro = parse_address_input(function_name, raw_macro);
	bool has_macro_tail =
	    (parsed_macro->st && parsed_macro->st[0] != '\0') || (parsed_macro->zip && parsed_macro->zip[0] != '\0');

	if (has_macro_tail && parsed_macro->address1 && parsed_macro->address1[0] != '\0' && !parsed_macro->num &&
	    !parsed_macro->street && !parsed_macro->street2)
	{
		parsed_macro->city = (parsed_macro->city && parsed_macro->city[0] != '\0')
					 ? psprintf("%s %s", parsed_macro->address1, parsed_macro->city)
					 : pstrdup(parsed_macro->address1);
		parsed_macro->address1 = NULL;
	}

	parsed_macro->cc = NULL;
	return parsed_macro;
}

/*
 * Diagnostic entry point that returns tokenization, rule matching, and the
 * final standardized address for either split or one-line input.
 */
PG_FUNCTION_INFO_V1(debug_standardize_address);

Datum
debug_standardize_address(PG_FUNCTION_ARGS)
{
	STANDARDIZER *std;
	char *lextab;
	char *gaztab;
	char *rultab;
	char *micro;
	StringInfo macro = makeStringInfo();
	STDADDR *stdaddr;
	ADDRESS *parsed_address = NULL;
	ADDRESS *parsed_macro = NULL;
	StringInfo result = makeStringInfo();

	elog(DEBUG2, "Start %s", __func__);
	initStringInfo(result);

	appendStringInfoChar(result, '{');

	lextab = text_to_cstring(PG_GETARG_TEXT_P(0));
	gaztab = text_to_cstring(PG_GETARG_TEXT_P(1));
	rultab = text_to_cstring(PG_GETARG_TEXT_P(2));
	micro = text_to_cstring(PG_GETARG_TEXT_P(3));

	if ((PG_NARGS() > 4) && (!PG_ARGISNULL(4)))
	{
		appendStringInfoString(macro, text_to_cstring(PG_GETARG_TEXT_P(4)));
		if (macro->len > 0)
			parsed_macro = parse_macro_input(__func__, macro->data);
	}
	else
	{
		parsed_address = parse_address_wrapper_input(__func__, micro, &micro, macro);
	}

	appendStringInfo(result, "\"micro\":%s,\"macro\":%s,", quote_identifier(micro), quote_identifier(macro->data));

	std = GetStdUsingFCInfo(fcinfo, lextab, gaztab, rultab);
	if (!std)
	{
		elog(ERROR, "%s failed to create the address standardizer object!", __func__);
	}

	if (parsed_address)
	{
		elog(DEBUG2, "%s: calling std_standardize('%s', ...)", __func__, micro);
		stdaddr = std_standardize(
		    std, micro, parsed_address->city, parsed_address->st, parsed_address->zip, parsed_address->cc, 0);
	}
	else if (parsed_macro)
	{
		elog(DEBUG2, "%s: calling std_standardize('%s', ... parsed macro)", __func__, micro);
		stdaddr = std_standardize(
		    std, micro, parsed_macro->city, parsed_macro->st, parsed_macro->zip, parsed_macro->cc, 0);
	}
	else
	{
		elog(DEBUG2, "%s: calling std_standardize_mm('%s', '%s')", __func__, micro, macro->data);
		stdaddr = std_standardize_mm(std, micro, macro->data, 0);
	}

	{
		bool need_rule_comma = false;
		STAND_PARAM *stand_param = std->misc_stand;
		STZ_PARAM *stz_info = stand_param->stz_info;
		STZ **stz_list = stz_info->stz_array;
		int stz_count = stz_info->stz_list_size;
		StringInfoData rule_in;
		StringInfoData rule_out;
		StringInfoData rule_stub;
		StringInfo rule_sql;
		SPIPlanPtr rule_plan = NULL;
		Oid rule_argtypes[1] = {CSTRINGOID};
		int spi_connect_result;

		elog(DEBUG2, "%s back from fetch_stdaddr", __func__);
		initStringInfo(&rule_in);
		initStringInfo(&rule_out);
		initStringInfo(&rule_stub);

		elog(DEBUG2, "Input tokenization candidates:\n");
		debug_append_input_tokens_json(result, stand_param);

		appendStringInfoString(result, ", \"rules\":[");
		rule_sql = makeStringInfo();
		appendStringInfo(
		    rule_sql, "SELECT id, rule FROM %s WHERE rule LIKE $1::varchar", quote_identifier(rultab));

		spi_connect_result = SPI_connect();
		if (spi_connect_result != SPI_OK_CONNECT)
		{
			elog(ERROR, "%s: Could not connect to the SPI manager", __func__);
			return -1;
		}
		rule_plan = SPI_prepare(rule_sql->data, 1, rule_argtypes);
		if (!rule_plan)
		{
			elog(ERROR,
			     "%s: unexpected return (%d) from query preparation: %s, (%d)",
			     __func__,
			     SPI_result,
			     rule_sql->data,
			     SPI_ERROR_UNCONNECTED);
			return -1;
		}
		SPI_keepplan(rule_plan);

		for (int stz_no = 0; stz_no < stz_count; stz_no++)
		{
			bool candidate_has_sparse_tokens;
			STZ *candidate = stz_list[stz_no];

			if (need_rule_comma)
				appendStringInfoChar(result, ',');

			need_rule_comma = true;

			elog(DEBUG2, "Raw standardization %d with score %f:\n", stz_no, candidate->score);
			appendStringInfo(result, "{\"score\": %f,", candidate->score);
			appendStringInfo(result, "\"raw_score\": %f,", candidate->raw_score);
			appendStringInfo(result, "\"no\": %d,", stz_no);

			candidate_has_sparse_tokens = debug_append_candidate_rule_tokens_json(
			    result, &rule_in, &rule_out, stand_param, candidate);
			/*
			 * This implementation only treats the rule-table lookup as reliable
			 * when the candidate covers every lexeme position it claims to
			 * describe. Sparse candidates may still be informative in debug
			 * output, but a prefix lookup may bind them to an unrelated stored
			 * rule row.
			 */
			if (!candidate_has_sparse_tokens)
			{
				debug_lookup_matching_rule_json(
				    result, __func__, rule_sql->data, rule_plan, &rule_stub, &rule_in, &rule_out);
			}
			else
			{
				/*
				 * Sparse or unterminated candidates may still be useful to inspect,
				 * but their token stream is not complete enough for a trustworthy
				 * rule-table lookup.
				 */
				appendStringInfoString(result,
						       ", \"rule_stub_string\": null"
						       ", \"rule_id\": null"
						       ", \"rule_string\": null"
						       ", \"rule_type_code\": null"
						       ", \"rule_type\": null"
						       ", \"rule_weight\": null");
			}

			appendStringInfoChar(result, '}');
		}
		SPI_finish();
	}
	appendStringInfoChar(result, ']');
	elog(DEBUG2, "%s: setup values json", __func__);
	append_debug_stdaddr_json(result, stdaddr);
	stdaddr_free(stdaddr);
	appendStringInfoChar(result, '}');
	PG_RETURN_TEXT_P(cstring_to_text_with_len(result->data, result->len));
}

/* Mirror stdaddr_fields when materializing the SQL composite result. */
static void
fill_stdaddr_values(char **values, const STDADDR *stdaddr)
{
	for (size_t i = 0; i < lengthof(stdaddr_fields); i++)
	{
		const char *field_value = stdaddr_field_value(stdaddr, stdaddr_fields[i].offset);

		values[i] = field_value ? pstrdup(field_value) : NULL;
	}
}

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
PG_FUNCTION_INFO_V1(standardize_address);

Datum
standardize_address(PG_FUNCTION_ARGS)
{
	TupleDesc tuple_desc;
	AttInMetadata *attinmeta;
	STANDARDIZER *std;
	char *lextab;
	char *gaztab;
	char *rultab;
	char *micro;
	char *macro;
	Datum result;
	STDADDR *stdaddr;
	char **values;
	HeapTuple tuple;
	ADDRESS *parsed_macro = NULL;
	char *postcode = NULL;

	DBG("Start standardize_address");

	lextab = text_to_cstring(PG_GETARG_TEXT_P(0));
	gaztab = text_to_cstring(PG_GETARG_TEXT_P(1));
	rultab = text_to_cstring(PG_GETARG_TEXT_P(2));
	micro = text_to_cstring(PG_GETARG_TEXT_P(3));
	macro = text_to_cstring(PG_GETARG_TEXT_P(4));

	DBG("calling RelationNameGetTupleDesc");
	if (get_call_result_type(fcinfo, NULL, &tuple_desc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "standardize_address() was called in a way that cannot accept record as a result");

	BlessTupleDesc(tuple_desc);
	attinmeta = TupleDescGetAttInMetadata(tuple_desc);

	DBG("calling GetStdUsingFCInfo(fcinfo, '%s', '%s', '%s')", lextab, gaztab, rultab);
	std = GetStdUsingFCInfo(fcinfo, lextab, gaztab, rultab);
	if (!std)
		elog(ERROR, "standardize_address() failed to create the address standardizer object!");

	if (macro && macro[0] != '\0')
		parsed_macro = parse_macro_input(__func__, macro);

	if (parsed_macro)
	{
		postcode = build_postcode_component(parsed_macro);
		DBG("calling std_standardize('%s', ... parsed macro)", micro);
		stdaddr = std_standardize(
		    std, micro, parsed_macro->city, parsed_macro->st, postcode, parsed_macro->cc, 0);
	}
	else
	{
		DBG("calling std_standardize_mm('%s', '%s')", micro, macro);
		stdaddr = std_standardize_mm(std, micro, macro, 0);
	}

	DBG("back from fetch_stdaddr");

	values = (char **)palloc0(16 * sizeof(char *));
	DBG("setup values array for natts=%d", tuple_desc->natts);
	fill_stdaddr_values(values, stdaddr);

	DBG("calling heap_form_tuple");
	tuple = BuildTupleFromCStrings(attinmeta, values);

	DBG("calling HeapTupleGetDatum");
	result = HeapTupleGetDatum(tuple);

	DBG("freeing values, nulls, and stdaddr");
	stdaddr_free(stdaddr);

	DBG("returning standardized result");
	PG_RETURN_DATUM(result);
}

/*
 * One-line standardization entry point. Parse the raw address first so the
 * structured postcode and country can be preserved during normalization.
 */
PG_FUNCTION_INFO_V1(standardize_address1);

Datum
standardize_address1(PG_FUNCTION_ARGS)
{
	TupleDesc tuple_desc;
	AttInMetadata *attinmeta;
	STANDARDIZER *std;
	char *lextab;
	char *gaztab;
	char *rultab;
	char *addr;
	char *micro;
	Datum result;
	STDADDR *stdaddr;
	char **values;
	HeapTuple tuple;
	ADDRESS *parsed_address;
	char *postcode;

	DBG("Start standardize_address");

	lextab = text_to_cstring(PG_GETARG_TEXT_P(0));
	gaztab = text_to_cstring(PG_GETARG_TEXT_P(1));
	rultab = text_to_cstring(PG_GETARG_TEXT_P(2));
	addr = text_to_cstring(PG_GETARG_TEXT_P(3));

	DBG("calling RelationNameGetTupleDesc");
	if (get_call_result_type(fcinfo, NULL, &tuple_desc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "standardize_address() was called in a way that cannot accept record as a result");

	BlessTupleDesc(tuple_desc);
	attinmeta = TupleDescGetAttInMetadata(tuple_desc);

	parsed_address = parse_address_wrapper_input(__func__, addr, &micro, NULL);
	postcode = build_postcode_component(parsed_address);

	DBG("calling GetStdUsingFCInfo(fcinfo, '%s', '%s', '%s')", lextab, gaztab, rultab);
	std = GetStdUsingFCInfo(fcinfo, lextab, gaztab, rultab);
	if (!std)
		elog(ERROR, "standardize_address() failed to create the address standardizer object!");

	DBG("calling std_standardize('%s', ...)", micro);
	stdaddr = std_standardize(
	    std, micro, parsed_address->city, parsed_address->st, postcode, parsed_address->cc, 0);

	DBG("back from fetch_stdaddr");

	values = (char **)palloc0(16 * sizeof(char *));
	DBG("setup values array for natts=%d", tuple_desc->natts);
	fill_stdaddr_values(values, stdaddr);

	DBG("calling heap_form_tuple");
	tuple = BuildTupleFromCStrings(attinmeta, values);

	DBG("calling HeapTupleGetDatum");
	result = HeapTupleGetDatum(tuple);

	DBG("freeing values, nulls, and stdaddr");
	stdaddr_free(stdaddr);

	DBG("returning standardized result");
	PG_RETURN_DATUM(result);
}
