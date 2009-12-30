/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 OpenGeo.org
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * Maintainer: Paul Ramsey <pramsey@opengeo.org>
 *
 **********************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "libpq-fe.h"
#include "shp2pgsql-core.h"

/*
** Global variables for GUI only
*/

/* Main window */
static GtkWidget *window_main = NULL;
static GtkWidget *entry_pg_user = NULL;
static GtkWidget *entry_pg_pass = NULL;
static GtkWidget *entry_pg_host = NULL;
static GtkWidget *entry_pg_port = NULL;
static GtkWidget *entry_pg_db = NULL;
static GtkWidget *entry_config_table = NULL;
static GtkWidget *entry_config_schema = NULL;
static GtkWidget *entry_config_srid = NULL;
static GtkWidget *entry_config_geocolumn = NULL;
static GtkWidget *label_pg_connection_test = NULL;
static GtkWidget *textview_log = NULL;
static GtkWidget *file_chooser_button_shape = NULL;
static GtkWidget *progress = NULL;
static GtkTextBuffer *textbuffer_log = NULL;

/* Options window */
static GtkWidget *entry_options_encoding = NULL;	
static GtkWidget *entry_options_nullpolicy = NULL;
static GtkWidget *checkbutton_options_preservecase = NULL;
static GtkWidget *checkbutton_options_forceint = NULL;
static GtkWidget *checkbutton_options_autoindex = NULL;
static GtkWidget *checkbutton_options_dbfonly = NULL;
static GtkWidget *checkbutton_options_dumpformat = NULL;
static GtkWidget *checkbutton_options_geography = NULL;

/* Other */
static char *pgui_errmsg = NULL;
static PGconn *pg_connection = NULL;
static SHPLOADERCONFIG *config = NULL;
static SHPLOADERSTATE *state = NULL;
static SHPCONNECTIONCONFIG *conn = NULL;

static volatile int import_running = 0;

/* Local prototypes */
static void pgui_create_options_dialogue(void);


/*
** Write a message to the Import Log text area.
*/
static void
pgui_log_va(const char *fmt, va_list ap)
{
	char *msg;

	if (!lw_vasprintf (&msg, fmt, ap)) return;

	gtk_text_buffer_insert_at_cursor(textbuffer_log, msg, -1);
	gtk_text_buffer_insert_at_cursor(textbuffer_log, "\n", -1);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(textview_log), gtk_text_buffer_get_insert(textbuffer_log) );

	/* Allow GTK to process events */
	while (gtk_events_pending())
		gtk_main_iteration();

	free(msg);
	return;
}

/*
** Write a message to the Import Log text area.
*/
static void
pgui_logf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	pgui_log_va(fmt, ap);

	va_end(ap);
	return;
}

static void
pgui_seterr(const char *errmsg)
{
	if ( pgui_errmsg )
	{
		free(pgui_errmsg);
	}
	pgui_errmsg = strdup(errmsg);
	return;
}

static void
pgui_raise_error_dialogue(void)
{
	GtkWidget *dialog, *label;
	gint result;

	label = gtk_label_new(pgui_errmsg);
	dialog = gtk_dialog_new_with_buttons("Error", GTK_WINDOW(window_main), 
	                GTK_DIALOG_MODAL & GTK_DIALOG_NO_SEPARATOR & GTK_DIALOG_DESTROY_WITH_PARENT, 
	                GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_has_separator ( GTK_DIALOG(dialog), FALSE );
	gtk_container_set_border_width (GTK_CONTAINER(dialog), 5);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), 15);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
	gtk_widget_show_all (dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return;
}

/* Terminate the main loop and exit the application. */
static void
pgui_quit (GtkWidget *widget, gpointer data)
{
	if ( pg_connection) PQfinish(pg_connection);
	pg_connection = NULL;
	gtk_main_quit ();
}

/* Set the global configuration based upon the current UI */
static void
pgui_set_config_from_ui()
{
	const char *pg_table = gtk_entry_get_text(GTK_ENTRY(entry_config_table));
	const char *pg_schema = gtk_entry_get_text(GTK_ENTRY(entry_config_schema));
	const char *pg_geom = gtk_entry_get_text(GTK_ENTRY(entry_config_geocolumn));

	const char *source_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser_button_shape));

	const char *entry_srid = gtk_entry_get_text(GTK_ENTRY(entry_config_srid));
	const char *entry_encoding = gtk_entry_get_text(GTK_ENTRY(entry_options_encoding));

	gboolean preservecase = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_preservecase));
	gboolean forceint = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_forceint));
	gboolean createindex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_autoindex));
	gboolean dbfonly = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_dbfonly));
	gboolean dumpformat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_dumpformat));
	gboolean geography = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_geography));

	char *c;

	/* Make the geocolumn field consistent with the load type by setting to the 
	   default geography field name if the load type is geography. */
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_geography)) )
	{
		if( ! strcmp( gtk_entry_get_text(GTK_ENTRY(entry_config_geocolumn)), GEOMETRY_DEFAULT ) )
		{
			gtk_entry_set_text(GTK_ENTRY(entry_config_geocolumn), GEOGRAPHY_DEFAULT );
		}
	}
	else
	{
		if( ! strcmp( gtk_entry_get_text(GTK_ENTRY(entry_config_geocolumn)), GEOGRAPHY_DEFAULT ) )
		{
			gtk_entry_set_text(GTK_ENTRY(entry_config_geocolumn), GEOMETRY_DEFAULT );
		}
	}

	/* Set the destination schema, table and column parameters */
	if (config->table)
		free(config->table);

	config->table = strdup(pg_table);

	if (config->schema)
		free(config->schema);

	if (strlen(pg_schema) == 0)
		config->schema = strdup("public");
	else
		config->schema = strdup(pg_schema);

	if (strlen(pg_geom) == 0)
		config->geom = strdup(GEOMETRY_DEFAULT);
	else
		config->geom = strdup(pg_geom);

	if ( geography )
		config->geography = 1;

	/* Set the destination filename: note the shp2pgsql core engine simply wants the file
	   without the .shp extension */
	if (config->shp_file)
		free(config->shp_file);

	/* Handle empty selection */
	if (source_file == NULL)
		config->shp_file = strdup("");
	else
		config->shp_file = strdup(source_file);

	for (c = config->shp_file + strlen(config->shp_file); c >= config->shp_file; c--)
	{
		if (*c == '.')
		{
			*c = '\0';
			break;
		}
	}

	/* Encoding */
	if( entry_encoding && strlen(entry_encoding) > 0 ) 
	{
		if (config->encoding)
			free(config->encoding);

		config->encoding = strdup(entry_encoding);
	}
	
	/* SRID */
	if ( ! ( config->sr_id = atoi(entry_srid) ) ) 
	{
		config->sr_id = -1;
	}

	/* Preserve case */
	if ( preservecase )
		config->quoteidentifiers = 1;
	else
		config->quoteidentifiers = 0;

	/* No long integers in table */
	if ( forceint )
		config->forceint4 = 1;
	else
		config->forceint4 = 0;
	
	/* Create spatial index after load */
	if ( createindex )
		config->createindex = 1;
	else
		config->createindex = 0;
	
	/* Read the .shp file, don't ignore it */
	if ( dbfonly )
		config->readshape = 0;
	else
		config->readshape = 1;

	/* Use COPY rather than INSERT format */
	if ( dumpformat )
		config->dump_format = 1;
	else
		config->dump_format = 0;

	return;
}

/* Validate the configuration, returning true or false */
static int
pgui_validate_config()
{
	/* Validate table parameters */
	if ( ! config->table || strlen(config->table) == 0 )
	{
		pgui_seterr("Fill in the destination table.");
		return 0;
	}

	if ( ! config->schema || strlen(config->schema) == 0 )
	{
		pgui_seterr("Fill in the destination schema.");
		return 0;
	}

	if ( ! config->geom || strlen(config->geom) == 0 )
	{
		pgui_seterr("Fill in the destination column.");
		return 0;
	}

	if ( ! config->shp_file || strlen(config->shp_file) == 0 )
	{
		pgui_seterr("Select a shape file to import.");
		return 0;
	}

	return 1;
}

/*
** Run a SQL command against the current connection.
*/
static int
pgui_exec(const char *sql)
{
	PGresult *res = NULL;
	ExecStatusType status;
	char sql_trunc[256];

	/* We need a connection to do anything. */
	if ( ! pg_connection ) return 0;
	if ( ! sql ) return 0;

	res = PQexec(pg_connection, sql);
	status = PQresultStatus(res);
	PQclear(res);

	/* Did something unexpected happen? */
	if ( ! ( status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK ) )
	{
		/* Log notices and return success. */
		if ( status == PGRES_NONFATAL_ERROR )
		{
			pgui_logf("%s", PQerrorMessage(pg_connection));
			return 1;
		}

		/* Log errors and return failure. */
		snprintf(sql_trunc, 255, "%s", sql);
		pgui_logf("Failed SQL begins: \"%s\"", sql_trunc);
		pgui_logf("Failed in pgui_exec(): %s", PQerrorMessage(pg_connection));
		return 0;
	}

	return 1;
}

/*
** Start the COPY process.
*/
static int
pgui_copy_start(const char *sql)
{
	PGresult *res = NULL;
	ExecStatusType status;
	char sql_trunc[256];

	/* We need a connection to do anything. */
	if ( ! pg_connection ) return 0;
	if ( ! sql ) return 0;

	res = PQexec(pg_connection, sql);
	status = PQresultStatus(res);
	PQclear(res);

	/* Did something unexpected happen? */
	if ( status != PGRES_COPY_IN )
	{
		/* Log errors and return failure. */
		snprintf(sql_trunc, 255, "%s", sql);
		pgui_logf("Failed SQL begins: \"%s\"", sql_trunc);
		pgui_logf("Failed in pgui_copy_start(): %s", PQerrorMessage(pg_connection));
		return 0;
	}

	return 1;
}

/*
** Send a line (row) of data into the COPY procedure.
*/
static int
pgui_copy_write(const char *line)
{
	char line_trunc[256];

	/* We need a connection to do anything. */
	if ( ! pg_connection ) return 0;
	if ( ! line ) return 0;

	/* Did something unexpected happen? */
	if ( PQputCopyData(pg_connection, line, strlen(line)) < 0 )
	{
		/* Log errors and return failure. */
		snprintf(line_trunc, 255, "%s", line);
		pgui_logf("Failed row begins: \"%s\"", line_trunc);
		pgui_logf("Failed in pgui_copy_write(): %s", PQerrorMessage(pg_connection));
		return 0;
	}

	/* Send linefeed to signify end of line */
	PQputCopyData(pg_connection, "\n", 1);

	return 1;
}

/*
** Finish the COPY process.
*/
static int
pgui_copy_end(const int rollback)
{
	char *errmsg = NULL;

	/* We need a connection to do anything. */
	if ( ! pg_connection ) return 0;

	if ( rollback ) errmsg = "Roll back the copy.";

	/* Did something unexpected happen? */
	if ( PQputCopyEnd(pg_connection, errmsg) < 0 )
	{
		/* Log errors and return failure. */
		pgui_logf("Failed in pgui_copy_end(): %s", PQerrorMessage(pg_connection));
		return 0;
	}

	return 1;
}

static char *
pgui_read_connection(void)
{
	const char *pg_host = gtk_entry_get_text(GTK_ENTRY(entry_pg_host));
	const char *pg_port = gtk_entry_get_text(GTK_ENTRY(entry_pg_port));
	const char *pg_user = gtk_entry_get_text(GTK_ENTRY(entry_pg_user));
	const char *pg_pass = gtk_entry_get_text(GTK_ENTRY(entry_pg_pass));
	const char *pg_db = gtk_entry_get_text(GTK_ENTRY(entry_pg_db));
	char *connection_string = NULL;

	if ( ! pg_host || strlen(pg_host) == 0 )
	{
		pgui_seterr("Fill in the server host.");
		return NULL;
	}
	if ( ! pg_port || strlen(pg_port) == 0 )
	{
		pgui_seterr("Fill in the server port.");
		return NULL;
	}
	if ( ! pg_user || strlen(pg_user) == 0 )
	{
		pgui_seterr("Fill in the user name.");
		return NULL;
	}
	if ( ! pg_db || strlen(pg_db) == 0 )
	{
		pgui_seterr("Fill in the database name.");
		return NULL;
	}
	if ( ! atoi(pg_port) )
	{
		pgui_seterr("Server port must be a number.");
		return NULL;
	}
	if ( ! lw_asprintf(&connection_string, "user=%s password=%s port=%s host=%s dbname=%s", pg_user, pg_pass, pg_port, pg_host, pg_db) )
	{
		return NULL;
	}
	if ( connection_string )
	{
		return connection_string;
	}
	return NULL;
}

static void
pgui_action_cancel(GtkWidget *widget, gpointer data)
{
	if (!import_running)
		pgui_quit(widget, data); /* quit if we're not running */
	else
		import_running = FALSE;
}

static void
pgui_sanitize_connection_string(char *connection_string)
{
	char *ptr = strstr(connection_string, "password");
	if ( ptr ) 
	{
		ptr += 9;
		while( *ptr != ' ' && *ptr != '\0' )
		{
			*ptr = '*';
			ptr++;
		}
	}
	return;
}

static void
pgui_action_connection_test(GtkWidget *widget, gpointer data)
{
	char *connection_string = NULL;
	char *connection_sanitized = NULL;

	if ( ! (connection_string = pgui_read_connection()) )
	{
		pgui_raise_error_dialogue();
		return;
	}
	
	connection_sanitized = strdup(connection_string);
	pgui_sanitize_connection_string(connection_string);
	pgui_logf("Connecting: %s", connection_sanitized);
	free(connection_sanitized);

	if ( pg_connection )
		PQfinish(pg_connection);

	pg_connection = PQconnectdb(connection_string);
	if (PQstatus(pg_connection) == CONNECTION_BAD)
	{
		pgui_logf( "Connection failed: %s", PQerrorMessage(pg_connection));
		gtk_label_set_text(GTK_LABEL(label_pg_connection_test), "Connection failed.");
		free(connection_string);
		PQfinish(pg_connection);
		pg_connection = NULL;
		return;
	}
	gtk_label_set_text(GTK_LABEL(label_pg_connection_test), "Connection succeeded.");
	pgui_logf( "Connection succeeded." );
	gtk_widget_show(label_pg_connection_test);
	PQfinish(pg_connection);
	pg_connection = NULL;
	free(connection_string);

	return;
}

static void
pgui_action_options_open(GtkWidget *widget, gpointer data)
{
	pgui_create_options_dialogue();
	return;
}

static void
pgui_action_options_close(GtkWidget *widget, gpointer data)
{
	pgui_set_config_from_ui();
	gtk_widget_destroy(widget);
	return;
}

static void
pgui_action_shape_file_set(GtkWidget *widget, gpointer data)
{
	char *shp_file;
	int shp_file_len;
	char *table_start;
	char *table_end;
	char *table;
	const char *gtk_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
	
	if( gtk_filename ) 
	{
		shp_file = strdup(gtk_filename);
		shp_file_len = strlen(shp_file);
	}
	else 
	{
		return;
	}

	/* Roll back from end to first slash character. */
	table_start = shp_file + shp_file_len;
	while ( *table_start != '/' && *table_start != '\\' && table_start > shp_file) {
		table_start--;
	}
	table_start++; /* Forward one to start of actual characters. */

	/* Roll back from end to first . character. */
	table_end = shp_file + shp_file_len;
	while ( *table_end != '.' && table_end > shp_file && table_end > table_start ) {
		table_end--;
	}
	
	/* Copy the table name into a fresh memory slot. */
	table = malloc(table_end - table_start + 1);
	memcpy(table, table_start, table_end - table_start);
	table[table_end - table_start] = '\0';

	/* Set the table name into the configuration */
	config->table = table;

	gtk_entry_set_text(GTK_ENTRY(entry_config_table), table);
	
	free(shp_file);
}

static void
pgui_action_import(GtkWidget *widget, gpointer data)
{
	char *connection_string = NULL;
	char *connection_sanitized = NULL;
	char *dest_string = NULL;
	int ret, i = 0;
	char *header, *footer, *record;	
	PGresult *result;


	if ( ! (connection_string = pgui_read_connection() ) )
	{
		pgui_raise_error_dialogue();
		return;
	}

	/* 
	** Set the configuration from the UI and validate
	*/
	pgui_set_config_from_ui();
	if (! pgui_validate_config() )
	{
		pgui_raise_error_dialogue();
		free(connection_string);

		return;
	}

	/* Log what we know so far */
	connection_sanitized = strdup(connection_string);
	pgui_sanitize_connection_string(connection_sanitized);
	pgui_logf("Connection: %s", connection_sanitized);
	pgui_logf("Destination: %s.%s", config->schema, config->table);
	pgui_logf("Source File: %s", config->shp_file);
	free(connection_sanitized);

	/* Connect to the database. */
	if ( pg_connection ) PQfinish(pg_connection);
	pg_connection = PQconnectdb(connection_string);

	if (PQstatus(pg_connection) == CONNECTION_BAD)
	{
		pgui_logf( "Connection failed: %s", PQerrorMessage(pg_connection));
		gtk_label_set_text(GTK_LABEL(label_pg_connection_test), "Connection failed.");
		free(connection_string);
		free(dest_string);
		PQfinish(pg_connection);
		pg_connection = NULL;
		return;
	}

	/*
	 * Loop through the items in the shapefile
	 */

	/* Disable the button to prevent multiple imports running at the same time */
	gtk_widget_set_sensitive(widget, FALSE); 

	/* Allow GTK events to get a look in */
	while (gtk_events_pending())
		gtk_main_iteration();

	/* Create the shapefile state object */
	state = ShpLoaderCreate(config);

	/* Open the shapefile */
	ret = ShpLoaderOpenShape(state);
	if (ret != SHPLOADEROK)
	{
		pgui_logf("%s", state->message);

		if (ret == SHPLOADERERR)
			goto import_cleanup;
	}

	/* If reading the whole shapefile, display its type */
	if (state->config->readshape)
	{
		pgui_logf("Shapefile type: %s", SHPTypeName(state->shpfiletype));
		pgui_logf("Postgis type: %s[%d]", state->pgtype, state->pgdims);
	}

	/* Get the header */
	ret = ShpLoaderGetSQLHeader(state, &header);
	if (ret != SHPLOADEROK)
	{
		pgui_logf("%s", state->message);

		if (ret == SHPLOADERERR)
			goto import_cleanup;
	}

	/* Send the header to the remote server: if we are in COPY mode then the last
	   statement will be a COPY and so will change connection mode */
	ret = pgui_exec(header);
	free(header);

	if (!ret)
		goto import_cleanup;

	/* If we are in COPY (dump format) mode, output the COPY statement and enter COPY mode */
	if (state->config->dump_format)
	{
		ret = ShpLoaderGetSQLCopyStatement(state, &header);

		if (ret != SHPLOADEROK)
		{
			pgui_logf("%s", state->message);
	
			if (ret == SHPLOADERERR)
				goto import_cleanup;
		}

		/* Send the result to the remote server: this should put us in COPY mode */
		ret = pgui_copy_start(header);
		free(header);

		if (!ret)
			goto import_cleanup;
	}

	/* Main loop: iterate through all of the records and send them to stdout */
	pgui_logf("Importing shapefile (%d records)...", ShpLoaderGetRecordCount(state));

	import_running = TRUE;
	for (i = 0; i < ShpLoaderGetRecordCount(state) && import_running; i++)
	{
		ret = ShpLoaderGenerateSQLRowStatement(state, i, &record);

		switch(ret)
		{
			case SHPLOADEROK:
				/* Simply send the statement */
				if (state->config->dump_format)
					ret = pgui_copy_write(record);
				else
					ret = pgui_exec(record);

				/* Display a record number if we failed */
				if (!ret)
					pgui_logf("Failed record number #%d", i);

				free(record);
				break;

			case SHPLOADERERR:
				/* Display the error message then stop */
				pgui_logf("%s\n", state->message);
				goto import_cleanup;
				break;

			case SHPLOADERWARN:
				/* Display the warning, but continue */
				pgui_logf("%s\n", state->message);

				if (state->config->dump_format)
					ret = pgui_copy_write(record);
				else
					ret = pgui_exec(record);

				/* Display a record number if we failed */
				if (!ret)
					pgui_logf("Failed record number #%d", i);

				free(record);
				break;

			case SHPLOADERRECDELETED:
				/* Record is marked as deleted - ignore */
				break;

			case SHPLOADERRECISNULL:
				/* Record is NULL and should be ignored according to NULL policy */
				break;
		}

		/* Update the progress bar */
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), (float)i / ShpLoaderGetRecordCount(state));

		/* Allow GTK events to get a look in */
		while (gtk_events_pending())
			gtk_main_iteration();
	}

	/* If we are in COPY (dump format) mode, leave COPY mode */
	if (state->config->dump_format)
	{
		if (! pgui_copy_end(0) )
			goto import_cleanup;

		result = PQgetResult(pg_connection);
		if (PQresultStatus(result) != PGRES_COMMAND_OK)
		{
			pgui_logf("COPY failed with the following error: %s", PQerrorMessage(pg_connection));
			ret = SHPLOADERERR;
			goto import_cleanup;
		}
	}

	/* Only continue if we didn't abort part way through */
	if (import_running)
	{
		/* Get the footer */
		ret = ShpLoaderGetSQLFooter(state, &footer);
		if (ret != SHPLOADEROK)
		{
			pgui_logf("%s\n", state->message);
	
			if (ret == SHPLOADERERR)
				goto import_cleanup;
		}

		if( state->config->createindex )
		{
			pgui_logf("Creating spatial index...\n");
		}
	
		/* Send the footer to the server */
		ret = pgui_exec(footer);
		free(footer);
	
		if (!ret)
			goto import_cleanup;
	}


import_cleanup:
	/* If we didn't finish inserting all of the items, an error occurred */
	if (i != ShpLoaderGetRecordCount(state) || !ret)
		pgui_logf("Shapefile import failed.");
	else
		pgui_logf("Shapefile import completed.");

	/* Free the state object */
	ShpLoaderDestroy(state);

	/* Import has definitely finished */
	import_running = FALSE;

	/* Enable the button once again */
	gtk_widget_set_sensitive(widget, TRUE);

	/* Silly GTK bug means we have to hide and show the button for it to work again! */
	gtk_widget_hide(widget);
	gtk_widget_show(widget);

	/* Reset the progress bar */
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);

	/* Allow GTK events to get a look in */
	while (gtk_events_pending())
		gtk_main_iteration();

	/* Tidy up */
	free(connection_string);
	free(dest_string);
	connection_string = dest_string = NULL;

	/* Disconnect from the database */
	PQfinish(pg_connection);
	pg_connection = NULL;

	return;
}

static void
pgui_create_options_dialogue_add_label(GtkWidget *table, const char *str, gfloat alignment, int row)
{
	GtkWidget *align = gtk_alignment_new( alignment, 0.5, 0.0, 1.0 );
	GtkWidget *label = gtk_label_new( str );
	gtk_table_attach_defaults(GTK_TABLE(table), align, 1, 3, row, row+1 );
	gtk_container_add (GTK_CONTAINER (align), label);
}

static void
pgui_create_options_dialogue()
{
	GtkWidget *table_options;
	GtkWidget *align_options_center;
	GtkWidget *dialog_options;
	static int text_width = 12;
	char *str;

	dialog_options = gtk_dialog_new_with_buttons ("Import Options", GTK_WINDOW(window_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_NONE, NULL);
	
	gtk_window_set_modal (GTK_WINDOW(dialog_options), TRUE);
	gtk_window_set_keep_above (GTK_WINDOW(dialog_options), TRUE);
	gtk_window_set_default_size (GTK_WINDOW(dialog_options), 180, 200);

	table_options = gtk_table_new(7, 3, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (table_options), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table_options), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table_options), 10);

	pgui_create_options_dialogue_add_label(table_options, "DBF file character encoding", 0.0, 0);
	entry_options_encoding = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_options_encoding), text_width);
	gtk_entry_set_text(GTK_ENTRY(entry_options_encoding), config->encoding);
	gtk_table_attach_defaults(GTK_TABLE(table_options), entry_options_encoding, 0, 1, 0, 1 );
	
	pgui_create_options_dialogue_add_label(table_options, "Preserve case of column names", 0.0, 1);
	checkbutton_options_preservecase = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_preservecase), config->quoteidentifiers ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 1, 2 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_preservecase);
	
	pgui_create_options_dialogue_add_label(table_options, "Do not create 'bigint' columns", 0.0, 2);
	checkbutton_options_forceint = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_forceint), config->forceint4 ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 2, 3 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_forceint);

	pgui_create_options_dialogue_add_label(table_options, "Create spatial index automatically after load", 0.0, 3);
	checkbutton_options_autoindex = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_autoindex), config->createindex ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 3, 4 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_autoindex);

	pgui_create_options_dialogue_add_label(table_options, "Load only attribute (dbf) data", 0.0, 4);
	checkbutton_options_dbfonly = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON	(checkbutton_options_dbfonly), config->readshape ? FALSE : TRUE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 4, 5 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_dbfonly);

	pgui_create_options_dialogue_add_label(table_options, "Load data using COPY rather than INSERT", 0.0, 5);
	checkbutton_options_dumpformat = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON	(checkbutton_options_dumpformat), config->dump_format ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 0.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 5, 6 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_dumpformat);

	pgui_create_options_dialogue_add_label(table_options, "Load into GEOGRAPHY column", 0.0, 6);
	checkbutton_options_geography = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_geography), config->geography ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 6, 7 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_geography);
	
	pgui_create_options_dialogue_add_label(table_options, "Policy for records with empty (null) shapes", 0.0, 7);
	entry_options_nullpolicy = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_options_nullpolicy), text_width);
	str = g_strdup_printf ("%d", config->null_policy);
	gtk_entry_set_text(GTK_ENTRY(entry_options_nullpolicy), str);
	g_free(str);
	gtk_table_attach_defaults(GTK_TABLE(table_options), entry_options_nullpolicy, 0, 1, 7, 8 );
	
	g_signal_connect(dialog_options, "response", G_CALLBACK(pgui_action_options_close), dialog_options);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog_options)->vbox), table_options, FALSE, FALSE, 0);

	gtk_widget_show_all (dialog_options);
}

static void
pgui_create_main_window(const SHPCONNECTIONCONFIG *conn)
{
	static int text_width = 12;
	/* Reusable label handle */
	GtkWidget *label;
	/* Main widgets */
	GtkWidget *vbox_main;
	/* Shape file section */
	GtkWidget *file_chooser_dialog_shape;
	GtkFileFilter *file_filter_shape;
	/* PgSQL section */
	GtkWidget *frame_pg, *frame_shape, *frame_log, *frame_config;
	GtkWidget *table_pg, *table_config;
	GtkWidget *button_pg_test;
	/* Button section */
	GtkWidget *hbox_buttons, *button_options, *button_import, *button_cancel;
	/* Log section */
	GtkWidget *scrolledwindow_log;

	/* create the main, top level, window */
	window_main = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	/* give the window a 10px wide border */
	gtk_container_set_border_width (GTK_CONTAINER (window_main), 10);

	/* give it the title */
	gtk_window_set_title (GTK_WINDOW (window_main), "Shape File to PostGIS Importer");

	/* open it a bit wider so that both the label and title show up */
	gtk_window_set_default_size (GTK_WINDOW (window_main), 180, 500);

	/* Connect the destroy event of the window with our pgui_quit function
	*  When the window is about to be destroyed we get a notificaiton and
	*  stop the main GTK loop
	*/
	g_signal_connect (G_OBJECT (window_main), "destroy", G_CALLBACK (pgui_quit), NULL);

	/*
	** Shape file selector 
	*/
	frame_shape = gtk_frame_new("Shape File");
	gtk_container_set_border_width (GTK_CONTAINER (frame_shape), 0);
	file_chooser_dialog_shape = gtk_file_chooser_dialog_new( "Select a Shape File", GTK_WINDOW (window_main), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	file_chooser_button_shape = gtk_file_chooser_button_new_with_dialog( file_chooser_dialog_shape );
	gtk_container_set_border_width (GTK_CONTAINER (file_chooser_button_shape), 8);
	file_filter_shape = gtk_file_filter_new();
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(file_filter_shape), "*.shp");
	gtk_file_filter_set_name(GTK_FILE_FILTER(file_filter_shape), "Shapefiles (*.shp)");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser_button_shape), file_filter_shape);
	gtk_container_add (GTK_CONTAINER (frame_shape), file_chooser_button_shape);
	g_signal_connect (G_OBJECT (file_chooser_button_shape), "file-set", G_CALLBACK (pgui_action_shape_file_set), NULL);


	/*
	** PostGIS info in a table
	*/
	frame_pg = gtk_frame_new("PostGIS Connection");
	table_pg = gtk_table_new(5, 3, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (table_pg), 8);
	gtk_table_set_col_spacings(GTK_TABLE(table_pg), 7);
	gtk_table_set_row_spacings(GTK_TABLE(table_pg), 3);
	/* User name row */
	label = gtk_label_new("Username:");
	entry_pg_user = gtk_entry_new();
	if( conn->username )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_user), conn->username);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 0, 1 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_user, 1, 3, 0, 1 );
	/* Password row */
	label = gtk_label_new("Password:");
	entry_pg_pass = gtk_entry_new();
	if( conn->password )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_pass), conn->password);
	gtk_entry_set_visibility( GTK_ENTRY(entry_pg_pass), FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 1, 2 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_pass, 1, 3, 1, 2 );
	/* Host and port row */
	label = gtk_label_new("Server Host:");
	entry_pg_host = gtk_entry_new();
	if( conn->host )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_host), conn->host);
	else
		gtk_entry_set_text(GTK_ENTRY(entry_pg_host), "localhost");
	gtk_entry_set_width_chars(GTK_ENTRY(entry_pg_host), text_width);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 2, 3 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_host, 1, 2, 2, 3 );
	entry_pg_port = gtk_entry_new();
	if( conn->port )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_port), conn->port);
	else
		gtk_entry_set_text(GTK_ENTRY(entry_pg_port), "5432");
	gtk_entry_set_width_chars(GTK_ENTRY(entry_pg_port), 8);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_port, 2, 3, 2, 3 );
	/* Database row */
	label = gtk_label_new("Database:");
	entry_pg_db   = gtk_entry_new();
	if( conn->database )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_db), conn->database);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 3, 4 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_db, 1, 3, 3, 4 );
	/* Test button row */
	button_pg_test = gtk_button_new_with_label("Test Connection...");
	gtk_table_attach_defaults(GTK_TABLE(table_pg), button_pg_test, 1, 2, 4, 5 );
	g_signal_connect (G_OBJECT (button_pg_test), "clicked", G_CALLBACK (pgui_action_connection_test), NULL);
	label_pg_connection_test = gtk_label_new("");
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label_pg_connection_test, 2, 3, 4, 5 );
	/* Add table into containing frame */
	gtk_container_add (GTK_CONTAINER (frame_pg), table_pg);

	/*
	** Configuration in a table
	*/
	frame_config = gtk_frame_new("Configuration");
	table_config = gtk_table_new(2, 4, TRUE);
	gtk_table_set_col_spacings(GTK_TABLE(table_config), 7);
	gtk_table_set_row_spacings(GTK_TABLE(table_config), 3);
	gtk_container_set_border_width (GTK_CONTAINER (table_config), 8);
	/* Destination schemea row */
	label = gtk_label_new("Destination Schema:");
	entry_config_schema = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_config_schema), "public");
	gtk_entry_set_width_chars(GTK_ENTRY(entry_config_schema), text_width);
	gtk_table_attach_defaults(GTK_TABLE(table_config), label, 0, 1, 0, 1 );
	gtk_table_attach_defaults(GTK_TABLE(table_config), entry_config_schema, 1, 2, 0, 1 );
	/* Destination table row */
	label = gtk_label_new("Destination Table:");
	entry_config_table = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_config_table), text_width);
	gtk_table_attach_defaults(GTK_TABLE(table_config), label, 0, 1, 1, 2 );
	gtk_table_attach_defaults(GTK_TABLE(table_config), entry_config_table, 1, 2, 1, 2 );
	/* SRID row */
	label = gtk_label_new("SRID:");
	entry_config_srid = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_config_srid), text_width);
	gtk_entry_set_text(GTK_ENTRY(entry_config_srid), "-1");
	gtk_table_attach_defaults(GTK_TABLE(table_config), label, 2, 3, 0, 1 );
	gtk_table_attach_defaults(GTK_TABLE(table_config), entry_config_srid, 3, 4, 0, 1 );
	/* Geom column row */
	label = gtk_label_new("Geometry Column:");
	entry_config_geocolumn = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_config_geocolumn), text_width);
	gtk_entry_set_text(GTK_ENTRY(entry_config_geocolumn), "the_geom");
	gtk_table_attach_defaults(GTK_TABLE(table_config), label, 2, 3, 1, 2 );
	gtk_table_attach_defaults(GTK_TABLE(table_config), entry_config_geocolumn, 3, 4, 1, 2 );

	/* Add table into containing frame */
	gtk_container_add (GTK_CONTAINER (frame_config), table_config);

	/* Progress bar for the import */
	progress = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(progress), GTK_PROGRESS_LEFT_TO_RIGHT);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);

	/*
	** Row of action buttons
	*/
	hbox_buttons = gtk_hbox_new(TRUE, 15);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_buttons), 0);
	/* Create the buttons themselves */
	button_options = gtk_button_new_with_label("Options...");
	button_import = gtk_button_new_with_label("Import");
	button_cancel = gtk_button_new_with_label("Cancel");
	/* Add actions to the buttons */
	g_signal_connect (G_OBJECT (button_import), "clicked", G_CALLBACK (pgui_action_import), NULL);
	g_signal_connect (G_OBJECT (button_options), "clicked", G_CALLBACK (pgui_action_options_open), NULL);
	g_signal_connect (G_OBJECT (button_cancel), "clicked", G_CALLBACK (pgui_action_cancel), NULL);
	/* And insert the buttons into the hbox */
	gtk_box_pack_start(GTK_BOX(hbox_buttons), button_options, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_cancel, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_import, TRUE, TRUE, 0);

	/*
	** Log window
	*/
	frame_log = gtk_frame_new("Import Log");
	gtk_container_set_border_width (GTK_CONTAINER (frame_log), 0);
	textview_log = gtk_text_view_new();
	textbuffer_log = gtk_text_buffer_new(NULL);
	scrolledwindow_log = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrolledwindow_log), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(textview_log), textbuffer_log);
	gtk_container_set_border_width (GTK_CONTAINER (textview_log), 5);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textview_log), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview_log), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview_log), GTK_WRAP_WORD);
	gtk_container_add (GTK_CONTAINER (scrolledwindow_log), textview_log);
	gtk_container_add (GTK_CONTAINER (frame_log), scrolledwindow_log);

	/*
	** Main window 
	*/
	vbox_main = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width (GTK_CONTAINER (vbox_main), 0);
	/* Add the frames into the main vbox */
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_shape, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_pg, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_config, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_buttons, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), progress, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_log, TRUE, TRUE, 0);
	/* and insert the vbox into the main window  */
	gtk_container_add (GTK_CONTAINER (window_main), vbox_main);
	/* make sure that everything, window and label, are visible */
	gtk_widget_show_all (window_main);

	return;
}

static void
usage()
{
	printf("RCSID: %s RELEASE: %s\n", RCSID, POSTGIS_VERSION);
	printf("USAGE: shp2pgsql-gui [options]\n");
	printf("OPTIONS:\n");
	printf("  -U <username>\n");
	printf("  -W <password>\n");
	printf("  -h <host>\n");
	printf("  -p <port>\n");
	printf("  -d <database>\n");
	printf("  -? Display this help screen\n");
}

int
main(int argc, char *argv[])
{
	char c;

	/* Parse command line options and set configuration */
	config = malloc(sizeof(SHPLOADERCONFIG));
	set_config_defaults(config);

	/* Here we override any defaults for the GUI */
	config->createindex = 1;
	
	conn = malloc(sizeof(SHPCONNECTIONCONFIG));
	memset(conn, 0, sizeof(SHPCONNECTIONCONFIG));

	while ((c = getopt(argc, argv, "U:p:W:d:h:")) != -1)
	{
		switch (c)
		{
			case 'U':
				conn->username = optarg;
				break;
			case 'p':
				conn->port = optarg;
				break;
			case 'W':
				conn->password = optarg;
				break;
			case 'd':
				conn->database = optarg;
				break;
			case 'h':
				conn->host = optarg;
				break;
			default:
				usage();
				free(conn);
				free(config);
				exit(0);
		}
	}

	/* initialize the GTK stack */
	gtk_init(&argc, &argv);

	/* set up the user interface */
	pgui_create_main_window(conn);
	
	/* start the main loop */
	gtk_main();

	/* Free the configuration */
	free(conn);
	free(config);

	return 0;
}
