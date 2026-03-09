/*
 * parseaddres.c - utility to crack a string into address, city st zip
 *
 * Copyright 2006 Stephen Woodbridge
 * Copyright 2026 Darafei Praliaskouski <me@komzpa.net>
 *
 * This code is released under and MIT-X style license,
 *
 * Stphen Woodbridge
 * woodbri@swoodbridge.com
 * woodbr@imaptools.com
 *
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#if PCRE_VERSION <= 1
#include <pcre.h>
#define PARSE_CASELESS PCRE_CASELESS
#else
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#define PARSE_CASELESS PCRE2_CASELESS
#endif

#include "parseaddress-api.h"

#undef DEBUG
// #define DEBUG 1

#ifdef DEBUG
#define DBG(format, arg...) elog(NOTICE, format, ##arg)
#else
#define DBG(format, arg...) \
	do \
	{ \
		; \
	} while (0)
#endif

const char *get_state_regex(const char *st);
const char *parseaddress_cvsid();
char *clean_leading_punct(char *s);
static int is_parse_separator(unsigned char c);
static size_t normalize_address_input(char *s);
static size_t normalize_country_token(const char *src, char *dst, size_t dst_size);
static bool ends_with_supported_macro(const char *s);
static bool extract_trailing_country(char *s, char *country_code);

static const char *const us_zip_regex = "\\b(\\d{5})[-\\s]{0,1}?(\\d{0,4})?$";
static const char *const canada_zip_regex = "\\b([a-z]\\d[a-z]\\s?\\d[a-z]\\d)$";
static const char *const canada_province_regex = "^(?-xism:(?i:(?=[abmnopqsy])(?:n[ltsu]|[am]b|[bq]c|on|pe|sk|yt)))$";
static const char *const us_ca_state_regex =
    "\\b(?-xism:(?i:(?=[abcdfghiklmnopqrstuvwy])(?:a(?:l(?:a(?:bam|sk)a|berta)?|mer(?:ican)?\\ samoa|r(?:k(?:ansas)?|izona)?|[kszb])|s(?:a(?:moa|skatchewan)|outh\\ (?:carolin|dakot)a|\\ (?:carolin|dakot)a|[cdk])|c(?:a(?:lif(?:ornia)?)?|o(?:nn(?:ecticut)?|lorado)?|t)|d(?:e(?:la(?:ware)?)?|istrict\\ of\\ columbia|c)|f(?:l(?:(?:orid)?a)?|ederal\\ states\\ of\\ micronesia|m)|m(?:i(?:c(?:h(?:igan)?|ronesia)|nn(?:esota)?|ss(?:(?:issipp|our)i)?)?|a(?:r(?:shall(?:\\ is(?:l(?:and)?)?)?|yland)|ss(?:achusetts)?|ine|nitoba)?|o(?:nt(?:ana)?)?|[ehdnstpb])|g(?:u(?:am)?|(?:eorgi)?a)|h(?:awai)?i|i(?:d(?:aho)?|l(?:l(?:inois)?)?|n(?:d(?:iana)?)?|(?:ow)?a)|k(?:(?:ansa)?s|(?:entuck)?y)|l(?:a(?:bordor)?|ouisiana)|n(?:e(?:w(?:\\ (?:foundland(?:\\ and\\ labordor)?|hampshire|jersey|mexico|(?:yor|brunswic)k)|foundland)|(?:brask|vad)a)?|o(?:rth(?:\\ (?:mariana(?:\\ is(?:l(?:and)?)?)?|(?:carolin|dakot)a)|west\\ territor(?:ies|y))|va\\ scotia)|\\ (?:carolin|dakot)a|u(?:navut)?|[vhjmycdblsf]|w?t)|o(?:h(?:io)?|k(?:lahoma)?|r(?:egon)?|n(?:t(?:ario)?)?)|p(?:a(?:lau)?|e(?:nn(?:sylvania)?|i)?|r(?:ince\\ edward\\ island)?|w|uerto\\ rico)|r(?:hode\\ island|i)|t(?:e(?:nn(?:essee)?|xas)|[nx])|ut(?:ah)?|v(?:i(?:rgin(?:\\ islands|ia))?|(?:ermon)?t|a)|w(?:a(?:sh(?:ington)?)?|i(?:sc(?:onsin)?)?|y(?:oming)?|(?:est)?\\ virginia|v)|b(?:ritish\\ columbia|c)|q(?:uebe)?c|y(?:ukon|t))))$";
typedef struct country_alias {
	const char *alias;
	const char *alpha2;
} CountryAlias;

static const CountryAlias country_aliases[] = {
#include "parseaddress-countries.h"
};

static int
is_parse_separator(unsigned char c)
{
	return ispunct(c) || isspace(c);
}

static size_t
normalize_address_input(char *s)
{
	size_t write_pos = 0;
	bool previous_was_space = false;

	if (!s)
		return 0;

	/*
	 * Canonicalize the input in place before regex parsing:
	 *   * periods behave like separators
	 *   * repeated whitespace collapses to one space
	 *   * trailing whitespace is removed
	 */
	for (size_t read_pos = 0; s[read_pos] != '\0'; read_pos++)
	{
		unsigned char current = (s[read_pos] == '.') ? ' ' : (unsigned char)s[read_pos];

		if (isspace(current))
		{
			if (!write_pos || previous_was_space)
				continue;

			previous_was_space = true;
		}
		else
		{
			previous_was_space = false;
		}

		s[write_pos] = (char)current;
		write_pos++;
	}

	if (write_pos && isspace((unsigned char)s[write_pos - 1]))
		write_pos--;

	s[write_pos] = '\0';
	return write_pos;
}

static size_t
normalize_country_token(const char *src, char *dst, size_t dst_size)
{
	size_t write_pos = 0;
	bool previous_was_space = false;

	if (!src || !dst || !dst_size)
		return 0;

	for (; *src != '\0' && write_pos + 1 < dst_size; src++)
	{
		unsigned char current = (unsigned char)*src;

		if (isalpha(current))
		{
			dst[write_pos++] = (char)toupper(current);
			previous_was_space = false;
			continue;
		}

		if (isspace(current) || ispunct(current))
		{
			if (!write_pos || previous_was_space)
				continue;

			dst[write_pos++] = ' ';
			previous_was_space = true;
		}
	}

	if (write_pos && dst[write_pos - 1] == ' ')
		write_pos--;

	dst[write_pos] = '\0';
	return write_pos;
}

static int
compare_country_alias(const void *lhs, const void *rhs)
{
	const CountryAlias *left = (const CountryAlias *)lhs;
	const CountryAlias *right = (const CountryAlias *)rhs;

	return strcmp(left->alias, right->alias);
}

const char *
country_code_from_name(const char *country_name)
{
	CountryAlias key;
	char normalized_country[128];
	const CountryAlias *match;

	if (!normalize_country_token(country_name, normalized_country, sizeof(normalized_country)))
		return NULL;

	key.alias = normalized_country;
	key.alpha2 = NULL;
	match = bsearch(
	    &key, country_aliases, lengthof(country_aliases), sizeof(country_aliases[0]), compare_country_alias);

	return match ? match->alpha2 : NULL;
}

/*
 * Recognize the right-most macro shape that this parser already knows how to
 * consume. That lets us conservatively strip trailing countries without
 * erasing ordinary trailing qualifiers, and still supports short ZIP fragments
 * once they are paired with a valid trailing state/province.
 */
static bool
ends_with_supported_macro(const char *s)
{
	int macro_ovect[OVECCOUNT];
	int rc;
	char *state_probe;

	if (!s || *s == '\0')
		return false;

	rc = match((char *)us_zip_regex, (char *)s, macro_ovect, 0);
	if (rc >= 2)
		return true;

	rc = match((char *)canada_zip_regex, (char *)s, macro_ovect, PARSE_CASELESS);
	if (rc >= 1)
		return true;

	rc = match((char *)us_ca_state_regex, (char *)s, macro_ovect, PARSE_CASELESS);
	if (rc > 0)
		return true;

	rc = match("\\b(\\d{2,4})$", (char *)s, macro_ovect, 0);
	if (rc < 2)
		return false;

	state_probe = pstrdup(s);
	*(state_probe + macro_ovect[0]) = '\0';
	(void)clean_trailing_punct(state_probe);

	rc = match((char *)us_ca_state_regex, state_probe, macro_ovect, PARSE_CASELESS);
	pfree(state_probe);

	return rc > 0;
}

/*
 * Strip a trailing country token before macro parsing so postcode and state
 * extraction still see the expected right-most fields. Known US/CA country
 * tokens update the country code; other recognized country names are stripped
 * without changing the default country inference.
 */
static bool
extract_trailing_country(char *s, char *country_code)
{
	int split_ovect[OVECCOUNT];
	int rc;
	const char *matched_country_code;
	char saved;

	if (!s || !country_code)
		return false;

	rc = match("([\\s,]+)([A-Za-z][A-Za-z\\s]{1,})$", s, split_ovect, 0);
	if (rc < 3)
		return false;

	saved = s[split_ovect[0]];
	s[split_ovect[0]] = '\0';

	matched_country_code = country_code_from_name(s + split_ovect[4]);
	if (!matched_country_code)
	{
		s[split_ovect[0]] = saved;
		return false;
	}

	if (!ends_with_supported_macro(s))
	{
		s[split_ovect[0]] = saved;
		return false;
	}

	strcpy(country_code, matched_country_code);

	(void)clean_trailing_punct(s);
	return true;
}

bool
strip_explicit_country_token(char *s, char *country_code)
{
	size_t normalized_length;

	if (!s || !country_code)
		return false;

	normalized_length = normalize_address_input(s);
	if (!normalized_length)
		return false;

	(void)clean_trailing_punct(s);
	return extract_trailing_country(s, country_code);
}

const char *
get_state_regex(const char *st)
{
#include "parseaddress-stcities.h"

	if (!st || strlen(st) != 2)
		return NULL;

	for (int i = 0; i < NUM_STATES; i++)
	{
		int cmp = strcmp(states[i], st);

		if (!cmp)
			return stcities[i];
		else if (cmp > 0)
			return NULL;
	}
	return NULL;
}

/*
 * Trim trailing punctuation and whitespace.
 * Returns 1 if a trailing comma was removed, 0 otherwise.
 */
int
clean_trailing_punct(char *s)
{
	size_t trim_pos;
	int ret = 0;

	if (!s || *s == '\0')
		return 0;

	trim_pos = strlen(s);
	while (trim_pos > 0)
	{
		char trailing = s[--trim_pos];

		if (!is_parse_separator((unsigned char)trailing))
			break;

		if (trailing == ',')
			ret = 1;
		s[trim_pos] = '\0';
	}
	return ret;
}

char *
clean_leading_punct(char *s)
{
	if (!s)
		return NULL;

	while (*s != '\0' && is_parse_separator((unsigned char)*s))
		s++;

	return s;
}

void
strtoupper(char *s)
{
	if (!s)
		return;

	while (*s != '\0')
	{
		*s = toupper((unsigned char)*s);
		s++;
	}
}

#if PCRE_VERSION <= 1
int
match(char *pattern, char *s, int *ovect, int options)
{
	const char *error;
	int erroffset;
	pcre *re;
	int rc;

	re = pcre_compile(pattern, options, &error, &erroffset, NULL);
	if (!re)
		return -99;

	rc = pcre_exec(re, NULL, s, strlen(s), 0, 0, ovect, OVECCOUNT);
	free(re);

	if (rc < 0)
		return rc;
	else if (!rc)
		rc = OVECPAIRS; // more matches than ovect can hold

	return rc;
}
#else
int
match(char *pattern, char *s, int *ovect, int options)
{
	int errorcode;
	PCRE2_SIZE erroffset;
	pcre2_code *re;
	int rc;
	pcre2_match_data *match_data;
	const PCRE2_SIZE *ovect2;

	re = pcre2_compile((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED, options, &errorcode, &erroffset, NULL);
	if (!re)
		return -99;

	match_data = pcre2_match_data_create(OVECPAIRS, NULL);

	rc = pcre2_match(re, (PCRE2_SPTR8)s, strlen(s), 0, 0, match_data, NULL);

	if (rc < 0)
	{ // no match or error
		pcre2_code_free(re);
		pcre2_match_data_free(match_data);
		return rc;
	}

	if (!rc)
	{ // more matches than ovect can hold
		rc = OVECPAIRS;
	}

	// copy the results out so we can free everything
	// before returning
	ovect2 = pcre2_get_ovector_pointer(match_data);
	for (int i = 0; i < rc; i++)
	{
		ovect[2 * i] = ovect2[2 * i];
		ovect[2 * i + 1] = ovect2[2 * i + 1];
	}

	pcre2_code_free(re);
	pcre2_match_data_free(match_data);
	return rc;
}
#endif

#define RET_ERROR(a, e) \
	if (!a) \
	{ \
		*reterr = e; \
		return NULL; \
	}

ADDRESS *
parseaddress(HHash *stH, char *s, int *reterr)
{

#include "parseaddress-regex.h"

	int ovect[OVECCOUNT];
	char *state = NULL;
	char *regx;
	size_t normalized_length;
	bool explicit_country = false;
	int rc;
	ADDRESS *ret;
#ifdef USE_HSEARCH
	ENTRY e, *ep;
	int err;
#else
	char *key;
	const char *val;
#endif

	ret = (ADDRESS *)palloc0(sizeof(ADDRESS));

	if (!s || *s == '\0')
		return ret;

	/* check if we were passed a lat lon */
	rc = match("^\\s*([-+]?\\d+(\\.\\d*)?)[\\,\\s]+([-+]?\\d+(\\.\\d*)?)\\s*$", s, ovect, 0);
	if (rc >= 3)
	{
		*(s + ovect[3]) = '\0';
		ret->lat = strtod(s + ovect[2], NULL);
		ret->lon = strtod(s + ovect[6], NULL);
		return ret;
	}

	/* Normalize spacing before the heavier regex-based parsing begins. */
	normalized_length = normalize_address_input(s);
	if (!normalized_length)
		return ret;

	/* clean trailing punctuation */
	(void)clean_trailing_punct(s);

	if (*s == '\0')
		return ret;

	/* assume country code is US */

	ret->cc = (char *)palloc0(3 * sizeof(char));
	strcpy(ret->cc, "US");

	explicit_country = extract_trailing_country(s, ret->cc);

	/* get US zipcode components */

	rc = match((char *)us_zip_regex, s, ovect, 0);
	if (rc >= 2)
	{
		ret->zip = (char *)palloc0((ovect[3] - ovect[2] + 1) * sizeof(char));
		strncpy(ret->zip, s + ovect[2], ovect[3] - ovect[2]);
		if (rc >= 3 && ovect[5] > ovect[4])
		{
			ret->zipplus = (char *)palloc0((ovect[5] - ovect[4] + 1) * sizeof(char));
			strncpy(ret->zipplus, s + ovect[4], ovect[5] - ovect[4]);
		}
		/* truncate the postalcode off the string */
		*(s + ovect[0]) = '\0';
	}
	/* get canada zipcode components */
	else
	{
		rc = match((char *)canada_zip_regex, s, ovect, PARSE_CASELESS);
		if (rc >= 1)
		{
			ret->zip = (char *)palloc0((ovect[1] - ovect[0] + 1) * sizeof(char));
			strncpy(ret->zip, s + ovect[0], ovect[1] - ovect[0]);
			if (!explicit_country)
				strcpy(ret->cc, "CA");
			/* truncate the postalcode off the string */
			*(s + ovect[0]) = '\0';
		}
		else
		{
			int state_ovect[OVECCOUNT];
			char *state_probe;

			/*
			 * Accept a short trailing ZIP fragment only when removing it exposes
			 * a valid trailing state/province. That keeps "MA 021" working
			 * without turning arbitrary suffixes like "Suite 201" into ZIPs.
			 */
			rc = match("\\b(\\d{2,4})$", s, ovect, 0);
			if (rc >= 2)
			{
				state_probe = pstrdup(s);
				*(state_probe + ovect[0]) = '\0';
				(void)clean_trailing_punct(state_probe);

				rc = match((char *)us_ca_state_regex, state_probe, state_ovect, PARSE_CASELESS);
				pfree(state_probe);

				if (rc > 0)
				{
					ret->zip = (char *)palloc0((ovect[3] - ovect[2] + 1) * sizeof(char));
					strncpy(ret->zip, s + ovect[2], ovect[3] - ovect[2]);
					*(s + ovect[0]) = '\0';
				}
			}
		}
	}

	/* clean trailing punctuation */
	(void)clean_trailing_punct(s);
	explicit_country = extract_trailing_country(s, ret->cc) || explicit_country;
	(void)clean_trailing_punct(s);

	/* get state components */

	rc = match((char *)us_ca_state_regex, s, ovect, PARSE_CASELESS);
	if (rc > 0)
	{
		state = (char *)palloc0((ovect[1] - ovect[0] + 1) * sizeof(char));
		strncpy(state, s + ovect[0], ovect[1] - ovect[0]);

		/* truncate the state/province off the string */
		*(s + ovect[0]) = '\0';

		/* lookup state in hash and get abbreviation */
		strtoupper(state);
#ifdef USE_HSEARCH
		e.key = state;
		err = hsearch_r(e, FIND, &ep, stH);
		if (err)
		{
			ret->st = (char *)palloc0(3 * sizeof(char));
			strcpy(ret->st, ep->data);
		}
#else
		key = state;
		val = (char *)hash_get(stH, key);
		if (val)
		{
			ret->st = pstrdup(val);
		}
#endif
		else
		{
			*reterr = 1002;
			return NULL;
		}

		/* check if it a Canadian Province */
		rc = match((char *)canada_province_regex, ret->st, ovect, PARSE_CASELESS);
		if (rc > 0 && !explicit_country)
		{
			strcpy(ret->cc, "CA");
			// if (ret->cc) printf("  CC: %s\n", ret->cc);
		}
	}

	/* clean trailing punctuation */
	(void)clean_trailing_punct(s);

	/* get city components */

	/*
	 * This part is ambiguous without punctuation after the street
	 * because we can have any of the following forms:
	 *
	 * num predir? prefixtype? street+ suffixtype? suffdir?,
	 *     ((north|south|east|west)? city)? state? zip?
	 *
	 * and technically num can be of the form:
	 *
	 *   pn1? n1 pn2? n2? sn2?
	 * where
	 * pn1 is a prefix character
	 * n1  is a number
	 * pn2 is a prefix character
	 * n2  is a number
	 * sn2 is a suffix character
	 *
	 * and a trailing letter might be [NSEW] which predir can also be
	 *
	 * So it is ambiguous whether a directional between street and city
	 * belongs to which component. Further since the the street and the city
	 * are both just a string of arbitrary words, it is difficult if not
	 * impossible to determine if an give word belongs to sone side or the
	 * other.
	 *
	 * So for the best results users should include a comma after the street.
	 *
	 * The approach will be as follows:
	 *   1. look for a comma and assume this is the separator
	 *   2. if we can find a state specific regex try that
	 *   3. else loop through an array of possible regex patterns
	 *   4. fail and assume there is not city
	 */

	/* look for a comma */
	DBG("parse_address: s=%s", s);

	regx = "(?:,\\s*)([^,]+)$";
	rc = match((char *)regx, s, ovect, 0);
	if (rc <= 0)
	{
		/* look for state specific regex */
		regx = (char *)get_state_regex(ret->st);
		if (regx)
			rc = match((char *)regx, s, ovect, 0);
	}
	DBG("Checked for comma: %d", rc);
	if (rc <= 0 && ret->st && strlen(ret->st))
	{
		/* look for state specific regex */
		regx = (char *)get_state_regex(ret->st);
		if (regx)
			rc = match((char *)regx, s, ovect, 0);
	}
	DBG("Checked for state-city: %d", rc);
	if (rc <= 0)
	{
		/* run through the regx's and see if we get a match */
		for (int i = 0; i < nreg; i++)
		{
			rc = match((char *)t_regx[i], s, ovect, 0);
			DBG("    rc=%d, i=%d", rc, i);
			if (rc > 0)
				break;
		}
	}
	if (rc > 0)
		DBG("Checked regexs: %d, %d, %d", rc, ovect[2], ovect[3]);
	else
		DBG("Checked regexs: %d", rc);
	if (rc >= 2 && ovect[3] > ovect[2])
	{
		/* we have a match so process it */
		ret->city = (char *)palloc0((ovect[3] - ovect[2] + 1) * sizeof(char));
		strncpy(ret->city, s + ovect[2], ovect[3] - ovect[2]);
		/* truncate the state/province off the string */
		*(s + ovect[2]) = '\0';
	}

	/* clean trailing punctuation */
	clean_trailing_punct(s);

	/* check for [@] that would indicate a intersection */
	/* -- 2010-12-11 : per Nancy R. we are using @ to indicate an intersection
	   ampersand is used in both street names and landmarks so it is highly
	   ambiguous -- */
	rc = match("^([^@]+)\\s*[@]\\s*([^@]+)$", s, ovect, 0);
	if (rc > 0)
	{
		s[ovect[3]] = '\0';
		clean_trailing_punct(s + ovect[2]);
		ret->street = pstrdup(s + ovect[2]);

		s[ovect[5]] = '\0';
		clean_leading_punct(s + ovect[4]);
		ret->street2 = pstrdup(s + ovect[4]);
	}
	else
	{

		/* and the remainder must be the address components */
		ret->address1 = pstrdup(clean_leading_punct(s));

		/* split the number off the street if it exists */
		rc = match("^((?i)[nsew]?\\d+[-nsew]*\\d*[nsew]?\\b)", s, ovect, 0);
		if (rc > 0)
		{
			ret->num = (char *)palloc0((ovect[1] - ovect[0] + 1) * sizeof(char));
			strncpy(ret->num, s, ovect[1] - ovect[0]);
			ret->street = pstrdup(clean_leading_punct(s + ovect[1]));
		}
	}

	return ret;
}

int
load_state_hash(HHash *stH)
{
	char *words[][2] = {{"ALABAMA", "AL"},
			    {"ALASKA", "AK"},
			    {"AMERICAN SAMOA", "AS"},
			    {"AMER SAMOA", "AS"},
			    {"SAMOA", "AS"},
			    {"ARIZONA", "AZ"},
			    {"ARKANSAS", "AR"},
			    {"ARK", "AR"},
			    {"CALIFORNIA", "CA"},
			    {"CALIF", "CA"},
			    {"COLORADO", "CO"},
			    {"CONNECTICUT", "CT"},
			    {"CONN", "CT"},
			    {"DELAWARE", "DE"},
			    {"DELA", "DE"},
			    {"DISTRICT OF COLUMBIA", "DC"},
			    {"FEDERAL STATES OF MICRONESIA", "FM"},
			    {"MICRONESIA", "FM"},
			    {"FLORIDA", "FL"},
			    {"FLA", "FL"},
			    {"GEORGIA", "GA"},
			    {"GUAM", "GU"},
			    {"HAWAII", "HI"},
			    {"IDAHO", "ID"},
			    {"ILLINOIS", "IL"},
			    {"ILL", "IL"},
			    {"INDIANA", "IN"},
			    {"IND", "IN"},
			    {"IOWA", "IA"},
			    {"KANSAS", "KS"},
			    {"KENTUCKY", "KY"},
			    {"LOUISIANA", "LA"},
			    {"MAINE", "ME"},
			    {"MARSHALL ISLAND", "MH"},
			    {"MARSHALL ISL", "MH"},
			    {"MARSHALL IS", "MH"},
			    {"MARSHALL", "MH"},
			    {"MARYLAND", "MD"},
			    {"MASSACHUSETTS", "MA"},
			    {"MASS", "MA"},
			    {"MICHIGAN", "MI"},
			    {"MICH", "MI"},
			    {"MINNESOTA", "MN"},
			    {"MINN", "MN"},
			    {"MISSISSIPPI", "MS"},
			    {"MISS", "MS"},
			    {"MISSOURI", "MO"},
			    {"MONTANA", "MT"},
			    {"MONT", "MT"},
			    {"NEBRASKA", "NE"},
			    {"NEVADA", "NV"},
			    {"NEW HAMPSHIRE", "NH"},
			    {"NEW JERSEY", "NJ"},
			    {"NEW MEXICO", "NM"},
			    {"NEW YORK", "NY"},
			    {"NORTH CAROLINA", "NC"},
			    {"N CAROLINA", "NC"},
			    {"NORTH DAKOTA", "ND"},
			    {"N DAKOTA", "ND"},
			    {"NORTH MARIANA ISL", "MP"},
			    {"NORTH MARIANA IS", "MP"},
			    {"NORTH MARIANA", "MP"},
			    {"NORTH MARIANA ISLAND", "MP"},
			    {"OHIO", "OH"},
			    {"OKLAHOMA", "OK"},
			    {"OREGON", "OR"},
			    {"PALAU", "PW"},
			    {"PENNSYLVANIA", "PA"},
			    {"PENN", "PA"},
			    {"PUERTO RICO", "PR"},
			    {"RHODE ISLAND", "RI"},
			    {"SOUTH CAROLINA", "SC"},
			    {"S CAROLINA", "SC"},
			    {"SOUTH DAKOTA", "SD"},
			    {"S DAKOTA", "SD"},
			    {"TENNESSEE", "TN"},
			    {"TENN", "TN"},
			    {"TEXAS", "TX"},
			    {"UTAH", "UT"},
			    {"VERMONT", "VT"},
			    {"VIRGIN ISLANDS", "VI"},
			    {"VIRGINIA", "VA"},
			    {"WASHINGTON", "WA"},
			    {"WASH", "WA"},
			    {"WEST VIRGINIA", "WV"},
			    {"W VIRGINIA", "WV"},
			    {"WISCONSIN", "WI"},
			    {"WISC", "WI"},
			    {"WYOMING", "WY"},
			    {"ALBERTA", "AB"},
			    {"BRITISH COLUMBIA", "BC"},
			    {"MANITOBA", "MB"},
			    {"NEW BRUNSWICK", "NB"},
			    {"NEW FOUNDLAND AND LABORDOR", "NL"},
			    {"NEW FOUNDLAND", "NL"},
			    {"NEWFOUNDLAND", "NL"},
			    {"LABORDOR", "NL"},
			    {"NORTHWEST TERRITORIES", "NT"},
			    {"NORTHWEST TERRITORY", "NT"},
			    {"NWT", "NT"},
			    {"NOVA SCOTIA", "NS"},
			    {"NUNAVUT", "NU"},
			    {"ONTARIO", "ON"},
			    {"ONT", "ON"},
			    {"PRINCE EDWARD ISLAND", "PE"},
			    {"PEI", "PE"},
			    {"QUEBEC", "QC"},
			    {"SASKATCHEWAN", "SK"},
			    {"YUKON", "YT"},
			    {"NF", "NL"},
			    {NULL, NULL}};

#ifdef USE_HSEARCH
	ENTRY e, *ep;
	int err;
#else
	char *key;
	const char *val;
#endif
	int cnt;

	/* count the entries above */
	cnt = 0;
	while (words[cnt][0])
		cnt++;

	DBG("Words cnt=%d", cnt);

#ifdef USE_HSEARCH
	if (!hcreate_r(cnt * 2, stH))
		return 1001;
	for (int i = 0; i < cnt; i++)
	{
		e.key = words[i][0];
		e.data = words[i][1];
		err = hsearch_r(e, ENTER, &ep, stH);
		/* there should be no failures */
		if (!err)
			return 1003;
		e.key = words[i][1];
		e.data = words[i][1];
		err = hsearch_r(e, ENTER, &ep, stH);
		/* there should be no failures */
		if (!err)
			return 1003;
	}
#else
	if (!stH)
		return 1001;
	for (int i = 0; i < cnt; i++)
	{
		// DBG("load_hash i=%d", i);
		key = words[i][0];
		val = words[i][1];
		hash_set(stH, key, (void *)val);
		key = words[i][1];
		hash_set(stH, key, (void *)words[i][1]);
	}
#endif
	return 0;
}

void
free_state_hash(HHash *stH)
{
// #if 0
#ifdef USE_HSEARCH
	if (stH)
		hdestroy_r(stH);
#else
	if (stH)
		hash_free(stH);
#endif
	// #endif
}
