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
static GtkWidget *window_main;
static GtkWidget *entry_pg_user;
static GtkWidget *entry_pg_pass;
static GtkWidget *entry_pg_host;
static GtkWidget *entry_pg_port;
static GtkWidget *entry_pg_db;
static GtkWidget *entry_config_table;
static GtkWidget *entry_config_schema;
static GtkWidget *entry_config_srid;
static GtkWidget *entry_config_geocolumn;
static GtkWidget *label_pg_connection_test;
static GtkWidget *textview_log;
static GtkWidget *file_chooser_button_shape;
static GtkTextBuffer *textbuffer_log;

/* Options window */
static GtkWidget *window_options;
static GtkWidget *entry_options_encoding;	
static GtkWidget *entry_options_nullpolicy;	
static GtkWidget *checkbutton_options_preservecase;
static GtkWidget *checkbutton_options_forceint;
static GtkWidget *checkbutton_options_autoindex;
static GtkWidget *checkbutton_options_dbfonly;

/* Other */
static char *pgui_errmsg = NULL;
static PGconn *pg_connection;


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

/*
** Write a message to the Import Log text area.
*/
void
pgui_log_va(const char *fmt, va_list ap)
{
	char *msg;

	if (!lw_vasprintf (&msg, fmt, ap)) return;

	gtk_text_buffer_insert_at_cursor(textbuffer_log, msg, -1);
	gtk_text_buffer_insert_at_cursor(textbuffer_log, "\n", -1);
	gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(textview_log), gtk_text_buffer_get_insert(textbuffer_log) );

	free(msg);
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



/*
** Run a SQL command against the current connection.
*/
int
pgui_exec(const char *sql)
{
	PGresult *res = NULL;
	ExecStatusType status;

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
		pgui_logf("Failed record number #%d", cur_entity);
		pgui_logf("Failed SQL was: %s", sql);
		pgui_logf("Failed in pgui_exec(): %s", PQerrorMessage(pg_connection));
		return 0;
	}

	return 1;
}

/*
** Start the COPY process.
*/
int
pgui_copy_start(const char *sql)
{
	PGresult *res = NULL;
	ExecStatusType status;

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
		pgui_logf("Failed SQL was: %s", sql);
		pgui_logf("Failed in pgui_copy_start(): %s", PQerrorMessage(pg_connection));
		return 0;
	}

	return 1;
}

/*
** Send a line (row) of data into the COPY procedure.
*/
int
pgui_copy_write(const char *line)
{

	/* We need a connection to do anything. */
	if ( ! pg_connection ) return 0;
	if ( ! line ) return 0;

	/* Did something unexpected happen? */
	if ( PQputCopyData(pg_connection, line, strlen(line)) < 0 )
	{
		/* Log errors and return failure. */
		pgui_logf("Failed record number #%d", cur_entity);
		pgui_logf("Failed row was: %s", line);
		pgui_logf("Failed in pgui_copy_write(): %s", PQerrorMessage(pg_connection));
		return 0;
	}

	return 1;

}

/*
** Finish the COPY process.
*/
int
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

static gboolean
check_translation_stage (gpointer data)
{
	int rv = 0;
	if ( translation_stage == TRANSLATION_IDLE ) return FALSE;
	if ( translation_stage == TRANSLATION_DONE )
	{
		pgui_logf("Import complete.");
		return FALSE;
	}
	if ( translation_stage == TRANSLATION_CREATE )
	{
		rv = translation_start();
		if ( ! rv )
		{
			pgui_logf("Import failed.");
			translation_stage = TRANSLATION_IDLE;
		}
		return TRUE;
	}
	if ( translation_stage == TRANSLATION_LOAD )
	{
		rv = translation_middle();
		if ( ! rv )
		{
			pgui_logf("Import failed.");
			translation_stage = TRANSLATION_IDLE;
		}
		return TRUE;
	}
	if ( translation_stage == TRANSLATION_CLEANUP )
	{
		rv = translation_end();
		if ( ! rv )
		{
			pgui_logf("Import failed.");
			translation_stage = TRANSLATION_IDLE;
		}
		return TRUE;
	}
	return FALSE;
}

/* Terminate the main loop and exit the application. */
static void
pgui_quit (GtkWidget *widget, gpointer data)
{
	if ( pg_connection) PQfinish(pg_connection);
	pg_connection = NULL;
	gtk_main_quit ();
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

static char *
pgui_read_destination(void)
{
	const char *pg_table = gtk_entry_get_text(GTK_ENTRY(entry_config_table));
	const char *pg_schema = gtk_entry_get_text(GTK_ENTRY(entry_config_schema));
	const char *pg_geom = gtk_entry_get_text(GTK_ENTRY(entry_config_geocolumn));
	
	char *dest_string = NULL;

	if ( ! pg_table || strlen(pg_table) == 0 )
	{
		pgui_seterr("Fill in the destination table.");
		return NULL;
	}
	if ( ! pg_schema || strlen(pg_schema) == 0 )
	{
		pg_schema = "public";
	}
	if ( ! pg_geom || strlen(pg_geom) == 0 )
	{
		pg_geom = "the_geom";
	}

	if ( ! asprintf(&dest_string, "%s.%s", pg_schema, pg_table) )
	{
		return NULL;
	}

	if ( dest_string )
	{
		/* Set the schema and table into the globals. */
		/* TODO change the core code to use dest_string instead */
		/* and move the global set into the import function. */
		table = strdup(pg_table);
		schema = strdup(pg_schema);
		geom = strdup(pg_geom);
		return dest_string;
	}
	return NULL;
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

static void
pgui_action_import(GtkWidget *widget, gpointer data)
{
	char *connection_string = NULL;
	char *dest_string = NULL;
	char *source_file = NULL;
	
	const char *entry_srid = gtk_entry_get_text(GTK_ENTRY(entry_config_srid));
	const char *entry_encoding = gtk_entry_get_text(GTK_ENTRY(entry_options_encoding));

	/* Do nothing if we're busy */
	if ( translation_stage > TRANSLATION_IDLE && translation_stage < TRANSLATION_DONE )
	{
		return;
	}

	if ( ! (connection_string = pgui_read_connection() ) )
	{
		pgui_raise_error_dialogue();
		return;
	}

	if ( ! (dest_string = pgui_read_destination() ) )
	{
		pgui_raise_error_dialogue();
		return;
	}

	if ( ! (source_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser_button_shape))) )
	{
		pgui_seterr("Select a shape file to import.");
		pgui_raise_error_dialogue();
		return;
	}

	/* Log what we know so far */
	pgui_logf("Connection: %s", connection_string);
	pgui_logf("Destination: %s", dest_string);
	pgui_logf("Source File: %s", source_file);

	/* Set the shape file into the global. */
	shp_file = strdup(source_file);
	g_free(source_file);

	/* Set the mode to "create" in the global. */
	opt = 'c';

	/* Set the output mode to inserts. */
	dump_format = 0;

	/* 
	** Read the options from the options dialogue... 
	*/
	
	/* Encoding */
	if( entry_encoding && strlen(entry_encoding) > 0 ) 
	{
		encoding = strdup(entry_encoding);
	}
	
	/* SRID */
	if ( ! ( sr_id = atoi(entry_srid) ) ) 
	{
		sr_id = -1;
	}

	/* Preserve case */
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_preservecase)) )
		quoteidentifiers = 1;
	else
		quoteidentifiers = 0;

	/* No long integers in table */
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_forceint)) )
		forceint4 = 1;
	else
		forceint4 = 0;
	
	/* Create spatial index after load */
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_autoindex)) )
		createindex = 1;
	else
		createindex = 0;
	
	/* Read the .shp file, don't ignore it */
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_dbfonly)) )
		readshape = 0;
	else
		readshape = 1;

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

	/* add the idle action */
	cur_entity = -1;
	translation_stage = TRANSLATION_CREATE;
	g_idle_add(check_translation_stage, NULL);

	free(connection_string);
	free(dest_string);

	return;

}

static void
pgui_action_options(GtkWidget *widget, gpointer data)
{
	/* Do nothing if we're busy */
	if ( translation_stage > TRANSLATION_IDLE && translation_stage < TRANSLATION_DONE )
	{
		return;
	}
	/* TODO Open the options dialog window here... */
	pgui_logf("Open the options dialog...");
	gtk_widget_show_all (window_options);
	return;
}

static void
pgui_action_cancel(GtkWidget *widget, gpointer data)
{
	if ( translation_stage > TRANSLATION_IDLE && translation_stage < TRANSLATION_DONE )
	{
		pgui_logf("Import stopped.");

		translation_stage = TRANSLATION_IDLE; /* return to idle if we are running */
	}
	else
	{
		pgui_quit(widget, data); /* quit if we're not running */
	}
	return;
}

static void
pgui_action_connection_test(GtkWidget *widget, gpointer data)
{
	char *connection_string = NULL;

	/* Do nothing if we're busy */
	if ( translation_stage > TRANSLATION_IDLE && translation_stage < TRANSLATION_DONE )
	{
		return;
	}


	if ( ! (connection_string = pgui_read_connection()) )
	{
		pgui_raise_error_dialogue();
		return;
	}
	pgui_logf("Connecting: %s", connection_string);

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
pgui_action_close_options(GtkWidget *widget, gpointer data)
{
	gtk_widget_hide_all (window_options);
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
	while( *table_start != '/' && *table_start != '\\' && table_start > shp_file) {
		table_start--;
	}
	table_start++; /* Forward one to start of actual characters. */

	/* Roll back from end to first . character. */
	table_end = shp_file + shp_file_len;
	while( *table_end != '.' && table_end > shp_file && table_end > table_start ) {
		table_end--;
	}
	
	/* Copy the table name into a fresh memory slot. */
	table = lwalloc(table_end - table_start + 1);
	memcpy(table, table_start, table_end - table_start);
	table[table_end - table_start + 1] = '\0';

	/* Set the table name into the entry. */
	gtk_entry_set_text(GTK_ENTRY(entry_config_table), table);
	
	lwfree(shp_file);
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
	GtkWidget *button_options_ok;
	GtkWidget *vbox_options;
	GtkWidget *align_options_center;
	static int text_width = 12;
	
	window_options = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal (GTK_WINDOW(window_options), TRUE);
	gtk_window_set_keep_above (GTK_WINDOW(window_options), TRUE);
	gtk_window_set_title (GTK_WINDOW(window_options), "Import Options");
	gtk_window_set_default_size (GTK_WINDOW(window_options), 180, 200);

	table_options = gtk_table_new(7, 3, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (table_options), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table_options), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table_options), 10);

	pgui_create_options_dialogue_add_label(table_options, "DBF file character encoding", 0.0, 0);
	entry_options_encoding = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_options_encoding), text_width);
	gtk_entry_set_text(GTK_ENTRY(entry_options_encoding), "LATIN1");
	gtk_table_attach_defaults(GTK_TABLE(table_options), entry_options_encoding, 0, 1, 0, 1 );
	
	pgui_create_options_dialogue_add_label(table_options, "Preserve case of column names", 0.0, 1);
	checkbutton_options_preservecase = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_preservecase), FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 1, 2 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_preservecase);
	
	pgui_create_options_dialogue_add_label(table_options, "Do not create 'bigint' columns", 0.0, 2);
	checkbutton_options_forceint = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_forceint), TRUE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 2, 3 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_forceint);

	pgui_create_options_dialogue_add_label(table_options, "Create spatial index automatically after load", 0.0, 3);
	checkbutton_options_autoindex = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_autoindex), TRUE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 3, 4 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_autoindex);

	pgui_create_options_dialogue_add_label(table_options, "Load only attribute (dbf) data", 0.0, 4);
	checkbutton_options_dbfonly = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON	(checkbutton_options_dbfonly), FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 0.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 4, 5 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_dbfonly);

	pgui_create_options_dialogue_add_label(table_options, "Policy for records with empty (null) shapes", 0.0, 5);
	entry_options_nullpolicy = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_options_nullpolicy), text_width);
	gtk_entry_set_text(GTK_ENTRY(entry_options_nullpolicy), "0");
	gtk_table_attach_defaults(GTK_TABLE(table_options), entry_options_nullpolicy, 0, 1, 5, 6 );
	
	button_options_ok = gtk_button_new_with_label("OK");
	g_signal_connect (G_OBJECT (button_options_ok), "clicked", G_CALLBACK (pgui_action_close_options), NULL);
	gtk_table_attach_defaults(GTK_TABLE(table_options), button_options_ok, 1, 2, 6, 7 );

	vbox_options = gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(vbox_options), table_options, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window_options), vbox_options);


}

static void
pgui_create_main_window(void)
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
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 0, 1 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_user, 1, 3, 0, 1 );
	/* Password row */
	label = gtk_label_new("Password:");
	entry_pg_pass = gtk_entry_new();
	gtk_entry_set_visibility( GTK_ENTRY(entry_pg_pass), FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 1, 2 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_pass, 1, 3, 1, 2 );
	/* Host and port row */
	label = gtk_label_new("Server Host:");
	entry_pg_host = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_pg_host), "localhost");
	gtk_entry_set_width_chars(GTK_ENTRY(entry_pg_host), text_width);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 2, 3 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_host, 1, 2, 2, 3 );
	entry_pg_port = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry_pg_port), "5432");
	gtk_entry_set_width_chars(GTK_ENTRY(entry_pg_port), 8);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_port, 2, 3, 2, 3 );
	/* Database row */
	label = gtk_label_new("Database:");
	entry_pg_db   = gtk_entry_new();
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
	g_signal_connect (G_OBJECT (button_options), "clicked", G_CALLBACK (pgui_action_options), NULL);
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
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrolledwindow_log), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
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
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_log, TRUE, TRUE, 0);
	/* and insert the vbox into the main window  */
	gtk_container_add (GTK_CONTAINER (window_main), vbox_main);
	/* make sure that everything, window and label, are visible */
	gtk_widget_show_all (window_main);

	return;
}

int
main(int argc, char *argv[])
{

	/* initialize the GTK stack */
	gtk_init(&argc, &argv);

	/* set up the user interface */
	pgui_create_main_window();
	pgui_create_options_dialogue();
	
	/* set up and global variables we want before running */
	gui_mode = 1;

	/* start the main loop */
	gtk_main();

	return 0;
}

/**********************************************************************
 * $Log$
 *
 **********************************************************************/
