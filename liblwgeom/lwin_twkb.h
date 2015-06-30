/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 *
 * Copyright (C) 2015 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <math.h>
#include "liblwgeom_internal.h"
#include "lwgeom_log.h"
#include "varint.h"

#define TWKB_IN_MAXCOORDS 4

/**
* Used for passing the parse state between the parsing functions.
*/
typedef struct
{
	/* Pointers to the bytes */
	uint8_t *twkb; /* Points to start of TWKB */
	uint8_t *twkb_end; /* Points to end of TWKB */
	uint8_t *pos; /* Current read position */

	uint32_t check; /* Simple validity checks on geometries */
	uint32_t lwtype; /* Current type we are handling */

	uint8_t has_bbox;
	uint8_t has_size;
	uint8_t has_idlist;
	uint8_t has_z;
	uint8_t has_m;
	uint8_t is_empty;

	/* Precision factors to convert ints to double */
	double factor;
	double factor_z;
	double factor_m;

	uint64_t size;

	/* Info about current geometry */
	uint8_t magic_byte; /* the magic byte contain info about if twkb contain id, size info, bboxes and precision */

	int ndims; /* Number of dimensions */

	int64_t *coords; /* An array to keep delta values from 4 dimensions */

} twkb_parse_state;


void header_from_twkb_state(twkb_parse_state *s);
inline double twkb_parse_state_double(twkb_parse_state *s, double factor);
