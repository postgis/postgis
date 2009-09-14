/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* Useful functions for all GEOGRAPHY handlers. */
GSERIALIZED* geography_serialize(LWGEOM *lwgeom);
void geography_valid_typmod(LWGEOM *lwgeom, int32 typmod);
void geography_valid_type(uchar type);
