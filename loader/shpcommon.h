/**********************************************************************
 * $Id: shpcommon.h 5646 2010-05-27 13:19:12Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

typedef struct shp_connection_state
{
	/* PgSQL username to log in with */
	char *username;

	/* PgSQL password to log in with */
	char *password;

	/* PgSQL database to connect to */
	char *database;

	/* PgSQL port to connect to */
	char *port;

	/* PgSQL server to connect to */
	char *host;

} SHPCONNECTIONCONFIG;

/* External shared functions */
char *escape_connection_string(char *str);
