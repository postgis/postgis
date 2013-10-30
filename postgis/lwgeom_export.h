/**********************************************************************
 * $Id: lwgeom_export.h 9324 2012-02-27 22:08:12Z pramsey $
 *
 * PostGIS - Export functions for PostgreSQL/PostGIS
 * Copyright 2009 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

char * getSRSbySRID(int SRID, bool short_crs);
int getSRIDbySRS(const char* SRS);
