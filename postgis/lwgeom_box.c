/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2009 Refractions Research Inc.
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "postgres.h"
#include "utils/geo_decls.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"


void box_to_box3d(BOX *box, BOX3D *out);
void box3d_to_box_p(BOX3D *box, BOX *out);


/* convert postgresql BOX to BOX3D */
void
box_to_box3d(BOX *box, BOX3D *out)
{
#if PARANOIA_LEVEL > 0
	if (box == NULL) return;
#endif

	out->xmin = box->low.x;
	out->ymin = box->low.y;

	out->xmax = box->high.x;
	out->ymax = box->high.y;

}

/* convert BOX3D to postgresql BOX */
void
box3d_to_box_p(BOX3D *box, BOX *out)
{
#if PARANOIA_LEVEL > 0
	if (box == NULL) return;
#endif

	out->low.x = box->xmin;
	out->low.y = box->ymin;

	out->high.x = box->xmax;
	out->high.y = box->ymax;
}

