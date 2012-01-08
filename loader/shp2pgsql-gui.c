/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2008 OpenGeo.org
 * Copyright 2010 LISAsoft
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * Maintainer: Paul Ramsey <pramsey@opengeo.org>
 *             Mark Leslie <mark.leslie@lisasoft.com>
 *
 **********************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <sys/stat.h>
#include "libpq-fe.h"
#include "shp2pgsql-core.h"
#include "pgsql2shp-core.h"

#include "../liblwgeom/liblwgeom.h" /* for lw_vasprintf */

#define GUI_RCSID "shp2pgsql-gui $Revision$"
#define SHAPEFIELDMAXWIDTH 60

static void pgui_log_va(const char *fmt, va_list ap);
static void pgui_seterr_va(const char *fmt, va_list ap);

/*
** Global variables for GUI only
*/

/* Main window */
static GtkWidget *window_main = NULL;

static GtkWidget *textview_log = NULL;
static GtkWidget *add_file_button = NULL;
static GtkTextBuffer *textbuffer_log = NULL;

/* Main window (listview) */
GtkListStore *list_store;
GtkWidget *tree;
GtkCellRenderer *filename_renderer;
GtkCellRenderer *schema_renderer;
GtkCellRenderer *table_renderer;
GtkCellRenderer *geom_column_renderer;
GtkCellRenderer *srid_renderer;
GtkCellRenderer *mode_renderer;
GtkCellRenderer *remove_renderer;

GtkTreeViewColumn *filename_column;
GtkTreeViewColumn *schema_column;
GtkTreeViewColumn *table_column;
GtkTreeViewColumn *geom_column;
GtkTreeViewColumn *srid_column;
GtkTreeViewColumn *mode_column;
GtkTreeViewColumn *remove_column;

GtkWidget *mode_combo = NULL;
GtkListStore *combo_list;

/* PostgreSQL database connection window */
static GtkWidget *window_conn = NULL;

static GtkWidget *entry_pg_user = NULL;
static GtkWidget *entry_pg_pass = NULL;
static GtkWidget *entry_pg_host = NULL;
static GtkWidget *entry_pg_port = NULL;
static GtkWidget *entry_pg_db = NULL;

/* Options window */
static GtkWidget *dialog_options = NULL;
static GtkWidget *entry_options_encoding = NULL;
static GtkWidget *checkbutton_options_preservecase = NULL;
static GtkWidget *checkbutton_options_forceint = NULL;
static GtkWidget *checkbutton_options_autoindex = NULL;
static GtkWidget *checkbutton_options_dbfonly = NULL;
static GtkWidget *checkbutton_options_dumpformat = NULL;
static GtkWidget *checkbutton_options_geography = NULL;

/* About dialog */
static GtkWidget *dialog_about = NULL;

/* File chooser */
static GtkWidget *dialog_filechooser = NULL;

/* Progress dialog */
static GtkWidget *dialog_progress = NULL;
static GtkWidget *progress = NULL;
static GtkWidget *label_progress = NULL;

/* Other items */
static int valid_connection = 0;

/* Constants for the list view etc. */
enum
{
	POINTER_COLUMN,
	FILENAME_COLUMN,
	SCHEMA_COLUMN,
	TABLE_COLUMN,
	GEOMETRY_COLUMN,
	SRID_COLUMN,
	MODE_COLUMN,
	REMOVE_COLUMN,
	N_COLUMNS
};

enum
{
	COMBO_TEXT,
	COMBO_OPTION_CHAR,
	COMBO_COLUMNS
};

enum
{
	CREATE_MODE,
	APPEND_MODE,
	DELETE_MODE,
	PREPARE_MODE
};

/* Other */
static char *pgui_errmsg = NULL;
static PGconn *pg_connection = NULL;
static SHPLOADERSTATE *state = NULL;
static SHPCONNECTIONCONFIG *conn = NULL;
static SHPLOADERCONFIG *global_loader_config = NULL;

static volatile int import_running = FALSE;

/* Local prototypes */
static void pgui_sanitize_connection_string(char *connection_string);


/*
** Write a message to the Import Log text area.
*/
void
pgui_log_va(const char *fmt, va_list ap)
{
	char *msg;
	GtkTextIter iter;

	if (!lw_vasprintf (&msg, fmt, ap)) return;

	/* Append text to the end of the text area, scrolling if required to make it visible */
	gtk_text_buffer_get_end_iter(textbuffer_log, &iter);
	gtk_text_buffer_insert(textbuffer_log, &iter, msg, -1);
	gtk_text_buffer_insert(textbuffer_log, &iter, "\n", -1);

	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(textview_log), &iter, 0.0, TRUE, 0.0, 1.0);

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

/* Write an error message */
void
pgui_seterr_va(const char *fmt, va_list ap)
{
	/* Free any existing message */
	if (pgui_errmsg)
		free(pgui_errmsg);
	
	if (!lw_vasprintf (&pgui_errmsg, fmt, ap)) return;
}

static void
pgui_seterr(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	pgui_seterr_va(fmt, ap);
	
	va_end(ap);
	return;
}

static void
pgui_raise_error_dialogue(void)
{
	GtkWidget *dialog, *label;
	gint result;

	label = gtk_label_new(pgui_errmsg);
	dialog = gtk_dialog_new_with_buttons(_("Error"), GTK_WINDOW(window_main),
	                                     GTK_DIALOG_MODAL & GTK_DIALOG_NO_SEPARATOR & GTK_DIALOG_DESTROY_WITH_PARENT,
	                                     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE );
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_container_set_border_width(GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), 15);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return;
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

/*
 * Ensures that the filename field width is within the stated bounds, and
 * 'appropriately' sized, for some definition of 'appropriately'.
 */
static void
update_filename_field_width(void)
{
	GtkTreeIter iter;
	gboolean is_valid;
	gchar *filename;
	int max_width;
	
	/* Loop through the list store to find the maximum length of an entry */
	max_width = 0;
	is_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	while (is_valid)
	{
		/* Grab the length of the filename entry in characters */
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, FILENAME_COLUMN, &filename, -1);
		if (strlen(filename) > max_width)
			max_width = strlen(filename);
		
		/* Get next entry */
		is_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}
	
	/* Note the layout manager will handle the minimum size for us; we just need to be concerned with
	   making sure we don't exceed a maximum limit */
	if (max_width > SHAPEFIELDMAXWIDTH)
		g_object_set(filename_renderer, "width-chars", SHAPEFIELDMAXWIDTH, NULL);
	else
		g_object_set(filename_renderer, "width-chars", -1, NULL);
	
	return;
}

/*
 * This will create a connection to the database, just to see if it can.
 * It cleans up after itself like a good little function and maintains
 * the status of the valid_connection parameter.
 */
static int
connection_test(void)
{
	char *connection_string = NULL;
	char *connection_sanitized = NULL;

	if (!(connection_string = ShpDumperGetConnectionStringFromConn(conn)))
	{
		pgui_raise_error_dialogue();
		valid_connection = 0;
		return 0;
	}

	connection_sanitized = strdup(connection_string);
	pgui_sanitize_connection_string(connection_sanitized);
	pgui_logf("Connecting: %s", connection_sanitized);
	free(connection_sanitized);

	pg_connection = PQconnectdb(connection_string);
	if (PQstatus(pg_connection) == CONNECTION_BAD)
	{
		pgui_logf( _("Database connection failed: %s"), PQerrorMessage(pg_connection));
		free(connection_string);
		PQfinish(pg_connection);
		pg_connection = NULL;
		valid_connection = 0;
		return 0;
	}
	PQfinish(pg_connection);
	pg_connection = NULL;
	free(connection_string);

	valid_connection = 1;
	return 1;
}


/* === Generic window functions === */

/* Delete event handler for popups that simply returns TRUE to prevent GTK from
   destroying the window and then hides it manually */
static gint
pgui_event_popup_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(widget));
	return TRUE;
}

/* === Progress window functions === */

static void
pgui_action_progress_cancel(GtkDialog *dialog, gint response_id, gpointer user_data) 
{
	/* Stop the current import */
	import_running = FALSE;

	return;
}

static gint
pgui_action_progress_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	/* Stop the current import */
	import_running = FALSE;

	return TRUE;
}


/* === Option Window functions === */

/* Update the specified SHPLOADERCONFIG with the global settings from the Options dialog */
static void
update_loader_config_globals_from_options_ui(SHPLOADERCONFIG *config)
{
	const char *entry_encoding = gtk_entry_get_text(GTK_ENTRY(entry_options_encoding));
	gboolean preservecase = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_preservecase));
	gboolean forceint = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_forceint));
	gboolean createindex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_autoindex));
	gboolean dbfonly = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_dbfonly));
	gboolean dumpformat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_dumpformat));
	gboolean geography = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_geography));

	if (geography)
	{
		config->geography = 1;
		
		if (config->geo_col)
			free(config->geo_col);
		
		config->geo_col = strdup(GEOGRAPHY_DEFAULT);
	}
	else
	{
		config->geography = 0;

		if (config->geo_col)
			free(config->geo_col);
		
		config->geo_col = strdup(GEOMETRY_DEFAULT);
	}

	/* Encoding */
	if (entry_encoding && strlen(entry_encoding) > 0)
	{
		if (config->encoding)
			free(config->encoding);

		config->encoding = strdup(entry_encoding);
	}

	/* Preserve case */
	if (preservecase)
		config->quoteidentifiers = 1;
	else
		config->quoteidentifiers = 0;

	/* No long integers in table */
	if (forceint)
		config->forceint4 = 1;
	else
		config->forceint4 = 0;

	/* Create spatial index after load */
	if (createindex)
		config->createindex = 1;
	else
		config->createindex = 0;

	/* Read the .shp file, don't ignore it */
	if (dbfonly)
	{
		config->readshape = 0;
		
		/* There will be no spatial column so don't create a spatial index */
		config->createindex = 0; 
	}
	else
		config->readshape = 1;

	/* Use COPY rather than INSERT format */
	if (dumpformat)
		config->dump_format = 1;
	else
		config->dump_format = 0;
	
	return;
}

/* Update the options dialog with the current values from the global config */
static void
update_options_ui_from_loader_config_globals(void)
{
	gtk_entry_set_text(GTK_ENTRY(entry_options_encoding), global_loader_config->encoding);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_preservecase), global_loader_config->quoteidentifiers ? TRUE : FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_forceint), global_loader_config->forceint4 ? TRUE : FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_autoindex), global_loader_config->createindex ? TRUE : FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_dbfonly), global_loader_config->readshape ? FALSE : TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_dumpformat), global_loader_config->dump_format ? TRUE : FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_geography), global_loader_config->geography ? TRUE : FALSE);
	
	return;
}
	
/* Set the global config variables controlled by the options dialogue */
static void
pgui_set_loader_configs_from_options_ui()
{
	GtkTreeIter iter;
	gboolean is_valid;
	gpointer gptr;
	SHPLOADERCONFIG *loader_file_config;
	
	/* First update the global (template) configuration */
	update_loader_config_globals_from_options_ui(global_loader_config);

	/* Now also update the same settings for any existing files already added. We
	   do this by looping through all entries and updating their config too. */
	is_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	while (is_valid)
	{
		/* Get the SHPLOADERCONFIG for this file entry */
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, POINTER_COLUMN, &gptr, -1);
		loader_file_config = (SHPLOADERCONFIG *)gptr;
		
		/* Update it */
		update_loader_config_globals_from_options_ui(loader_file_config);
		
		/* Get next entry */
		is_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}
	
	return;
}


/* === Main window functions === */

/* Given a filename, generate a new configuration and add it to the listview */
static SHPLOADERCONFIG *
create_new_file_config(const char *filename)
{
	SHPLOADERCONFIG *loader_file_config;
	char *table_start, *table_end;
	int i;
	
	/* Generate a new configuration by copying the global options first and then
	   adding in the specific values for this file */
	loader_file_config = malloc(sizeof(SHPLOADERCONFIG));
	memcpy(loader_file_config, global_loader_config, sizeof(SHPLOADERCONFIG));
	
	/* Note: we must copy the encoding here since it is the only pass-by-reference
	   type set in set_loader_config_defaults() and each config needs its own copy
	   of any referenced items */
	loader_file_config->encoding = strdup(global_loader_config->encoding);
	
	/* Copy the filename (we'll remove the .shp extension in a sec) */
	loader_file_config->shp_file = strdup(filename);
	
	/* Generate the default table name from the filename */
	table_start = loader_file_config->shp_file + strlen(loader_file_config->shp_file);
	while (*table_start != '/' && *table_start != '\\' && table_start > loader_file_config->shp_file)
		table_start--;
	
	/* Forward one to start of actual characters */
	table_start++;

	/* Roll back from end to first . character. */
	table_end = loader_file_config->shp_file + strlen(loader_file_config->shp_file);
	while (*table_end != '.' && table_end > loader_file_config->shp_file && table_end > table_start )
		table_end--;
	
	/* Sneakily remove .shp from shp_file */
	*table_end = '\0';

	/* Copy the table name */
	loader_file_config->table = malloc(table_end - table_start + 1);
	memcpy(loader_file_config->table, table_start, table_end - table_start);
	loader_file_config->table[table_end - table_start] = '\0';

	/* Force the table name to lower case */
	for (i = 0; i < table_end - table_start; i++)
	{
		if (isupper(loader_file_config->table[i]) != 0)
			loader_file_config->table[i] = tolower(loader_file_config->table[i]);
	}

	/* Set the default schema to public */
	loader_file_config->schema = strdup("public");
	
	/* Set the default geo column name */
	if (global_loader_config->geography)
		loader_file_config->geo_col = strdup(GEOGRAPHY_DEFAULT);
	else
		loader_file_config->geo_col = strdup(GEOMETRY_DEFAULT);
	
	return loader_file_config;
}

/* Given the loader configuration, add a new row representing this file to the listview */
static void
add_loader_file_config_to_list(SHPLOADERCONFIG *loader_file_config)
{
	GtkTreeIter iter;
	char *srid;
	
	/* Convert SRID into string */
	lw_asprintf(&srid, "%d", loader_file_config->sr_id);
	
	gtk_list_store_insert_before(list_store, &iter, NULL);
	gtk_list_store_set(list_store, &iter,
			   POINTER_COLUMN, loader_file_config,
	                   FILENAME_COLUMN, loader_file_config->shp_file,
	                   SCHEMA_COLUMN, loader_file_config->schema,
	                   TABLE_COLUMN, loader_file_config->table,
	                   GEOMETRY_COLUMN, loader_file_config->geo_col,
	                   SRID_COLUMN, srid,
	                   MODE_COLUMN, _("Create"),
	                   -1);	
		   
	/* Update the filename field width */
	update_filename_field_width();
	
	return;
}

/* Free up the specified SHPLOADERCONFIG */
static void
free_loader_config(SHPLOADERCONFIG *config)
{

	if (config->table)
		free(config->table);

	if (config->schema)
		free(config->schema);
	
	if (config->geo_col)
		free(config->geo_col);
	
	if (config->shp_file)
		free(config->shp_file);

	if (config->encoding)
		free(config->encoding);

	if (config->tablespace)
		free(config->tablespace);
	
	if (config->idxtablespace)
		free(config->idxtablespace);
	
	/* Free the config itself */
	free(config);
}

/* Validate a single DBF column type against a PostgreSQL type: return either TRUE or FALSE depending
   upon whether or not the type is (broadly) compatible */
static int
validate_shape_column_against_pg_column(int dbf_fieldtype, char *pg_fieldtype)
{
	switch (dbf_fieldtype)
	{
		case FTString:
			/* Only varchar */
			if (!strcmp(pg_fieldtype, "varchar"))
				return -1;
			break;
			
		case FTDate:
			/* Only date */
			if (!strcmp(pg_fieldtype, "date"))
				return -1;
			break;
			
		case FTInteger:
			/* Tentatively allow int2, int4 and numeric */
			if (!strcmp(pg_fieldtype, "int2") || !strcmp(pg_fieldtype, "int4") || !strcmp(pg_fieldtype, "numeric"))
				return -1;
			break;
			
		case FTDouble:
			/* Only float8/numeric */
			if (!strcmp(pg_fieldtype, "float8") || !strcmp(pg_fieldtype, "numeric"))
				return -1;
			break;
			
		case FTLogical:
			/* Only boolean */
			if (!strcmp(pg_fieldtype, "boolean"))
				return -1;
			break;
	}
	
	/* Otherwise we can't guarantee this (but this is just a warning anyway) */
	return 0;
}

/* Validate column compatibility for the given loader configuration against the table/column
   list returned in result */
static int
validate_remote_loader_columns(SHPLOADERCONFIG *config, PGresult *result)
{
	ExecStatusType status;
	int ntuples;
	char *pg_fieldname, *pg_fieldtype;
	int ret, i, j, found, response = SHPLOADEROK;
	
	/* Check the status of the result set */
	status = PQresultStatus(result);
	if (status == PGRES_TUPLES_OK)
	{
		ntuples = PQntuples(result);
	
		switch (config->opt)
		{
			case 'c':
				/* If we have a row matching the table given in the config, then it already exists */
				if (ntuples > 0)
				{
					pgui_seterr(_("ERROR: Create mode selected for existing table: %s.%s"), config->schema, config->table);
					response = SHPLOADERERR;
				}	
				break;
			
			case 'p':
				/* If we have a row matching the table given in the config, then it already exists */
				if (ntuples > 0)
				{
					pgui_seterr(_("ERROR: Prepare mode selected for existing table: %s.%s"), config->schema, config->table);
					response = SHPLOADERERR;
				}
				break;	

			case 'a':
				/* If we are trying to append to a table but it doesn't exist, emit a warning */
				if (ntuples == 0)
				{
					pgui_seterr(_("ERROR: Destination table %s.%s could not be found for appending"), config->schema, config->table);
					response = SHPLOADERERR;
				}
				else
				{
					/* If we have a row then lets do some simple column validation... */
					state = ShpLoaderCreate(config);
					ret = ShpLoaderOpenShape(state);
					if (ret != SHPLOADEROK)
					{
						pgui_logf(_("Warning: Could not load shapefile %s"), config->shp_file);
						ShpLoaderDestroy(state);
					}
					
					/* Find each column based upon its name and then validate type separately... */
					for (i = 0; i < state->num_fields; i++)
					{
						/* Make sure we find a column */
						found = 0;
						for (j = 0; j < ntuples; j++)
						{
							pg_fieldname = PQgetvalue(result, j, PQfnumber(result, "field"));
							pg_fieldtype = PQgetvalue(result, j, PQfnumber(result, "type"));
						
							if (!strcmp(state->field_names[i], pg_fieldname))
							{
								found = -1;
								
								ret = validate_shape_column_against_pg_column(state->types[i], pg_fieldtype);
								if (!ret)
								{
									pgui_logf(_("Warning: DBF Field '%s' is not compatible with PostgreSQL column '%s' in %s.%s"), state->field_names[i], pg_fieldname, config->schema, config->table);
									response = SHPLOADERWARN;
								}
							}
						}
						
						/* Flag a warning if we can't find a match */
						if (!found)
						{
							pgui_logf(_("Warning: DBF Field '%s' within file %s could not be matched to a column within table %s.%s"), 
								  state->field_names[i], config->shp_file, config->schema, config->table);
							response = SHPLOADERWARN;
						}
					}
					
					ShpLoaderDestroy(state);
				}
				
				break;
		}
	}
	else
	{
		pgui_seterr(_("ERROR: unable to process validation response from remote server"));
		response = SHPLOADERERR;
	}
	
	return response;	
}

/* Terminate the main loop and exit the application. */
static void
pgui_quit (GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

static void
pgui_action_about_open()
{
	/* Display the dialog and hide it again upon exit */
	gtk_dialog_run(GTK_DIALOG(dialog_about));
	gtk_widget_hide(dialog_about);
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
pgui_action_options_open(GtkWidget *widget, gpointer data)
{
	update_options_ui_from_loader_config_globals();
	gtk_widget_show_all(dialog_options);
	return;
}

static void
pgui_action_options_close(GtkWidget *widget, gint response, gpointer data)
{
	/* Only update the configuration if the user hit OK */
	if (response == GTK_RESPONSE_OK)
		pgui_set_loader_configs_from_options_ui();
	
	/* Hide the dialog */
	gtk_widget_hide(dialog_options);
	
	return;
}

static void
pgui_action_open_file_dialog(GtkWidget *widget, gpointer data)
{
	SHPLOADERCONFIG *loader_file_config;
	
	/* Run the dialog */
	if (gtk_dialog_run(GTK_DIALOG(dialog_filechooser)) == GTK_RESPONSE_ACCEPT)
	{
		/* Create the new file configuration based upon the filename and add it to the listview */
		loader_file_config = create_new_file_config(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog_filechooser)));
		add_loader_file_config_to_list(loader_file_config);
	}
	
	gtk_widget_hide(dialog_filechooser);
}

static void
pgui_action_import(GtkWidget *widget, gpointer data)
{	
	SHPLOADERCONFIG *loader_file_config;
	gint is_valid;
	gpointer gptr;
	GtkTreeIter iter;
	char *sql_form, *query, *progress_text = NULL, *progress_shapefile = NULL;
	PGresult *result;
	char *connection_string = NULL;
	int ret, success, i = 0;
	char *header, *footer, *record;
	
	/* Firstly make sure that we can connect to the database - if we can't then there isn't much
	   point doing anything else... */
	if (!connection_test())
	{
		pgui_seterr(_("Unable to connect to the database - please check your connection settings"));
		pgui_raise_error_dialogue();
	
		return;
	}

	/* Validation: we loop through each of the files in order to validate them as a separate pass */
	is_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	
	/* Let's open a single connection to the remote DB for the duration of the validation pass;
	   note that we already know the connection string works, otherwise we would have bailed
	   out earlier in the function */
	connection_string = ShpDumperGetConnectionStringFromConn(conn);
	pg_connection = PQconnectdb(connection_string);
	
	/* Setup the table/column type discovery query */
	sql_form = "SELECT a.attnum, a.attname AS field, t.typname AS type, a.attlen AS length, a.atttypmod AS precision FROM pg_class c, pg_attribute a, pg_type t, pg_namespace n WHERE c.relname = '%s' AND n.nspname = '%s' AND a.attnum > 0 AND a.attrelid = c.oid AND a.atttypid = t.oid AND c.relnamespace = n.oid ORDER BY a.attnum";
	
	while (is_valid)
	{
		/* Grab the SHPLOADERCONFIG for this row */
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, POINTER_COLUMN, &gptr, -1);
		loader_file_config = (SHPLOADERCONFIG *)gptr;
		
		/* For each entry, we execute a remote query in order to determine the column names
		   and types for the remote table if they actually exist */
		query = malloc(strlen(sql_form) + strlen(loader_file_config->schema) + strlen(loader_file_config->table) + 1);
		sprintf(query, sql_form, loader_file_config->table, loader_file_config->schema);
		result = PQexec(pg_connection, query);
		
		/* Call the validation function with the SHPLOADERCONFIG and the result set */
		ret = validate_remote_loader_columns(loader_file_config, result);
		if (ret == SHPLOADERERR)
		{
			pgui_raise_error_dialogue();
			
			PQclear(result);
			free(query);
			
			return;
		}	
		
		/* Free the SQL query */
		PQclear(result);
		free(query);

		/* Get next entry */
		is_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}
	
	/* Close our database connection */
	PQfinish(pg_connection);	

	
	/* Once we've done the validation pass, now let's load the shapefile */
	pg_connection = PQconnectdb(connection_string);
	
	is_valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	while (is_valid)
	{
		/* Grab the SHPLOADERCONFIG for this row */
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, POINTER_COLUMN, &gptr, -1);
		loader_file_config = (SHPLOADERCONFIG *)gptr;
	
		pgui_logf("\n==============================");
		pgui_logf("Importing with configuration: %s, %s, %s, %s, mode=%c, dump=%d, simple=%d, geography=%d, index=%d, shape=%d, srid=%d", loader_file_config->table, loader_file_config->schema, loader_file_config->geo_col, loader_file_config->shp_file, loader_file_config->opt, loader_file_config->dump_format, loader_file_config->simple_geometries, loader_file_config->geography, loader_file_config->createindex, loader_file_config->readshape, loader_file_config->sr_id);
		
		/*
		 * Loop through the items in the shapefile
		 */
		import_running = TRUE;
		success = FALSE;
		
		/* Disable the button to prevent multiple imports running at the same time */
		gtk_widget_set_sensitive(widget, FALSE);

		/* Allow GTK events to get a look in */
		while (gtk_events_pending())
			gtk_main_iteration();
				
		/* Create the shapefile state object */
		state = ShpLoaderCreate(loader_file_config);

		/* Open the shapefile */
		ret = ShpLoaderOpenShape(state);
		if (ret != SHPLOADEROK)
		{
			pgui_logf("%s", state->message);

			if (ret == SHPLOADERERR)
				goto import_cleanup;
		}

		/* For progress display, only show the "core" filename */
		for (i = strlen(loader_file_config->shp_file); i >= 0 
			&& loader_file_config->shp_file[i - 1] != '\\' && loader_file_config->shp_file[i - 1] != '/'; i--);

		progress_shapefile = malloc(strlen(loader_file_config->shp_file));
		strcpy(progress_shapefile, &loader_file_config->shp_file[i]);
		
		/* Display the progress dialog */
		lw_asprintf(&progress_text, _("Importing shapefile %s (%d records)..."), progress_shapefile, ShpLoaderGetRecordCount(state));
		gtk_label_set_text(GTK_LABEL(label_progress), progress_text);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
		gtk_widget_show_all(dialog_progress);

		/* If reading the whole shapefile, display its type */
		if (state->config->readshape)
		{
			pgui_logf("Shapefile type: %s", SHPTypeName(state->shpfiletype));
			pgui_logf("PostGIS type: %s[%d]", state->pgtype, state->pgdims);
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

		/* If we are in prepare mode, we need to skip the actual load. */
		if (state->config->opt != 'p')
		{
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
			for (i = 0; i < ShpLoaderGetRecordCount(state) && import_running; i++)
			{
				ret = ShpLoaderGenerateSQLRowStatement(state, i, &record);

				switch (ret)
				{
				case SHPLOADEROK:
					/* Simply send the statement */
					if (state->config->dump_format)
						ret = pgui_copy_write(record);
					else
						ret = pgui_exec(record);

					/* Display a record number if we failed */
					if (!ret)
						pgui_logf(_("Import failed on record number %d"), i);

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
						pgui_logf(_("Import failed on record number %d"), i);

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
					pgui_logf(_("COPY failed with the following error: %s"), PQerrorMessage(pg_connection));
					ret = SHPLOADERERR;
					goto import_cleanup;
				}
			}
		} /* if (state->config->opt != 'p') */

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

			if (state->config->createindex)
			{
				pgui_logf(_("Creating spatial index...\n"));
			}

			/* Send the footer to the server */
			ret = pgui_exec(footer);
			free(footer);

			if (!ret)
				goto import_cleanup;
		}
		
		/* Indicate success */
		success = TRUE;

import_cleanup:
		/* Import has definitely stopped running */
		import_running = FALSE;

		/* Make sure we abort any existing transaction */
		if (!success)
			pgui_exec("ABORT");
		
		/* If we didn't finish inserting all of the items (and we expected to), an error occurred */
		if ((state->config->opt != 'p' && i != ShpLoaderGetRecordCount(state)) || !ret)
			pgui_logf(_("Shapefile import failed."));
		else
			pgui_logf(_("Shapefile import completed."));

		/* Free the state object */
		ShpLoaderDestroy(state);

		/* Tidy up */
		if (progress_text)
			free(progress_text);
		
		if (progress_shapefile)
			free(progress_shapefile);
		
		/* Get next entry */
		is_valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}
	
	/* Import has definitely finished */
	import_running = FALSE;

	/* Enable the button once again */
	gtk_widget_set_sensitive(widget, TRUE);

	/* Silly GTK bug means we have to hide and show the button for it to work again! */
	gtk_widget_hide(widget);
	gtk_widget_show(widget);

	/* Hide the progress dialog */
	gtk_widget_hide(dialog_progress);
		
	/* Allow GTK events to get a look in */
	while (gtk_events_pending())
		gtk_main_iteration();

	/* Disconnect from the database */
	PQfinish(pg_connection);
	pg_connection = NULL;

	/* Tidy up */
	free(connection_string);

	return;
}


/* === ListView functions and signal handlers === */

/* Creates a single file row in the list table given the URI of a file */
static void
process_single_uri(char *uri)
{
	SHPLOADERCONFIG *loader_file_config;
	char *filename = NULL;
	char *hostname;
	GError *error = NULL;

	if (uri == NULL)
	{
		pgui_logf(_("Unable to process drag URI."));
		return;
	}

	filename = g_filename_from_uri(uri, &hostname, &error);
	g_free(uri);

	if (filename == NULL)
	{
		pgui_logf(_("Unable to process filename: %s\n"), error->message);
		g_error_free(error);
		return;
	}

	/* Create a new row in the listview */
	loader_file_config = create_new_file_config(filename);
	add_loader_file_config_to_list(loader_file_config);	

	g_free(filename);
	g_free(hostname);

}

/* Update the SHPLOADERCONFIG to the values currently contained within the iter  */
static void
update_loader_file_config_from_listview_iter(GtkTreeIter *iter, SHPLOADERCONFIG *loader_file_config)
{
	gchar *schema, *table, *geo_col, *srid;
	
	/* Grab the main values for this file */
	gtk_tree_model_get(GTK_TREE_MODEL(list_store), iter,
		SCHEMA_COLUMN, &schema,
		TABLE_COLUMN, &table,
		GEOMETRY_COLUMN, &geo_col,
		SRID_COLUMN, &srid,
		-1);
	
	/* Update the schema */
	if (loader_file_config->schema)
		free(loader_file_config->schema);
	
	loader_file_config->schema = strdup(schema);

	/* Update the table */
	if (loader_file_config->table)
		free(loader_file_config->table);
		
	loader_file_config->table = strdup(table);

	/* Update the geo column */
	if (loader_file_config->geo_col)
		free(loader_file_config->geo_col);
		
	loader_file_config->geo_col = strdup(geo_col);
	
	/* Update the SRID */
	loader_file_config->sr_id = atoi(srid);

	/* Free the values */
	return;
}


/*
 * Here lives the magic of the drag-n-drop of the app.  We really don't care
 * about much of the provided tidbits.  We only actually user selection_data
 * and extract a list of filenames from it.
 */
static void
pgui_action_handle_file_drop(GtkWidget *widget,
                             GdkDragContext *dc,
                             gint x, gint y,
                             GtkSelectionData *selection_data,
                             guint info, guint t, gpointer data)
{
	const gchar *p, *q;

	if (selection_data->data == NULL)
	{
		pgui_logf(_("Unable to process drag data."));
		return;
	}

	p = (char*)selection_data->data;
	while (p)
	{
		/* Only process non-comments */
		if (*p != '#')
		{
			/* Trim leading whitespace */
			while (g_ascii_isspace(*p))
				p++;
			q = p;
			/* Scan to the end of the string (null or newline) */
			while (*q && (*q != '\n') && (*q != '\r'))
				q++;
			if (q > p)
			{
				/* Ignore terminating character */
				q--;
				/* Trim trailing whitespace */
				while (q > p && g_ascii_isspace(*q))
					q--;
				if (q > p)
				{
					process_single_uri(g_strndup(p, q - p + 1));
				}
			}
		}
		/* Skip to the next entry */
		p = strchr(p, '\n');
		if (p)
			p++;
	}
}


/*
 * This function is a signal handler for the load mode combo boxes.
 */
static void
pgui_action_handle_tree_combo(GtkCellRendererCombo *combo,
                              gchar *path_string,
                              GtkTreeIter *new_iter,
                              gpointer user_data)
{
	GtkTreeIter iter;
	SHPLOADERCONFIG *loader_file_config;
	char opt;
	gchar *combo_text;
	gpointer gptr;
	
	/* Grab the SHPLOADERCONFIG from the POINTER_COLUMN for the list store */
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(list_store), &iter, path_string);
	gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, POINTER_COLUMN, &gptr, -1);
	loader_file_config = (SHPLOADERCONFIG *)gptr;

	/* Now grab the row selected within the combo box */
	gtk_tree_model_get(GTK_TREE_MODEL(combo_list), new_iter, COMBO_OPTION_CHAR, &opt, -1);
	
	/* Update the configuration */
	
	/* Hack for index creation: we must disable it if we are appending, otherwise we
	   end up trying to generate the index again */
	loader_file_config->createindex = global_loader_config->createindex;
	
	switch (opt)
	{
		case 'a':
			loader_file_config->opt = 'a';
			
			/* Other half of index creation hack */
			loader_file_config->createindex = 0;
				
			break;
			
		case 'd':
			loader_file_config->opt = 'd';
			break;
			
		case 'p':
			loader_file_config->opt = 'p';
			break;
			
		case 'c':
			loader_file_config->opt = 'c';
			break;
	}
	
	/* Update the selection in the listview with the text from the combo */
	gtk_tree_model_get(GTK_TREE_MODEL(combo_list), new_iter, COMBO_TEXT, &combo_text, -1); 
	gtk_list_store_set(list_store, &iter, MODE_COLUMN, combo_text, -1);
	
	return;	
}
	

/*
 * This method is a signal listener for all text renderers in the file
 * list table, including the empty ones.  Edits of the empty table are
 * passed to an appropriate function, while edits of existing file rows
 * are applied and the various validations called.
 */
static void
pgui_action_handle_tree_edit(GtkCellRendererText *renderer,
                             gchar *path,
                             gchar *new_text,
                             gpointer column)
{
	GtkTreeIter iter;
	gpointer gptr;
	gint columnindex;
	SHPLOADERCONFIG *loader_file_config;
	char *srid;
	
	/* Empty doesn't fly */
	if (strlen(new_text) == 0)
		return;
	
	/* Update the model with the current edit change */
	columnindex = *(gint *)column;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(list_store), &iter, path);
	gtk_list_store_set(list_store, &iter, columnindex, new_text, -1);
	
	/* Grab the SHPLOADERCONFIG from the POINTER_COLUMN for the list store */
	gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, POINTER_COLUMN, &gptr, -1);
	loader_file_config = (SHPLOADERCONFIG *)gptr;
	
	/* Update the configuration from the current UI data */
	update_loader_file_config_from_listview_iter(&iter, loader_file_config);
	
	/* Now refresh the listview UI row with the new configuration */
	lw_asprintf(&srid, "%d", loader_file_config->sr_id);
	
	gtk_list_store_set(list_store, &iter,
	                   SCHEMA_COLUMN, loader_file_config->schema,
	                   TABLE_COLUMN, loader_file_config->table,
	                   GEOMETRY_COLUMN, loader_file_config->geo_col,
	                   SRID_COLUMN, srid,
	                   -1);

	return;
}

/*
 * Signal handler for the remove box.  Performs no user interaction, simply
 * removes the row from the table.
 */
static void
pgui_action_handle_tree_remove(GtkCellRendererToggle *renderer,
                               gchar *path,
                               gpointer user_data)
{
	GtkTreeIter iter;
	SHPLOADERCONFIG *loader_file_config;
	gpointer gptr;
	
	/* Grab the SHPLOADERCONFIG from the POINTER_COLUMN for the list store */
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(list_store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, POINTER_COLUMN, &gptr, -1);
	loader_file_config = (SHPLOADERCONFIG *)gptr;
	
	/* Free the configuration from memory */
	free_loader_config(loader_file_config);
	
	/* Remove the row from the list */
	gtk_list_store_remove(list_store, &iter);

	/* Update the filename field width */
	update_filename_field_width();
}



/* === Connection Window functions === */

/* Set the connection details UI from the current configuration */
static void 
update_conn_ui_from_conn_config(void)
{
	if (conn->username)
		gtk_entry_set_text(GTK_ENTRY(entry_pg_user), conn->username);
	
	if (conn->password)
		gtk_entry_set_text(GTK_ENTRY(entry_pg_pass), conn->password);
	
	if (conn->host)
		gtk_entry_set_text(GTK_ENTRY(entry_pg_host), conn->host);
	
	if (conn->port)
		gtk_entry_set_text(GTK_ENTRY(entry_pg_port), conn->port);
	
	if (conn->database)
		gtk_entry_set_text(GTK_ENTRY(entry_pg_db), conn->database);

	return;
}

/* Set the current connection configuration from the connection details UI */
static void
update_conn_config_from_conn_ui(void)
{
	const char *text;
	
	text = gtk_entry_get_text(GTK_ENTRY(entry_pg_user));
	if (conn->username)
		free(conn->username);
	
	if (strlen(text))
		conn->username = strdup(text);
	else
		conn->username = NULL;
	
	text = gtk_entry_get_text(GTK_ENTRY(entry_pg_pass));
	if (conn->password)
		free(conn->password);
	
	if (strlen(text))
		conn->password = strdup(text);
	else
		conn->password = NULL;
	
	text = gtk_entry_get_text(GTK_ENTRY(entry_pg_host));
	if (conn->host)
		free(conn->host);
	
	if (strlen(text))
		conn->host = strdup(text);
	else
		conn->host = NULL;

	text = gtk_entry_get_text(GTK_ENTRY(entry_pg_port));
	if (conn->port)
		free(conn->port);
	
	if (strlen(text))
		conn->port = strdup(text);
	else
		conn->port = NULL;

	text = gtk_entry_get_text(GTK_ENTRY(entry_pg_db));
	if (conn->database)
		free(conn->database);
	
	if (strlen(text))
		conn->database = strdup(text);
	else
		conn->database = NULL;

	return;
}

/*
 * Open the connection details dialog
 */
static void
pgui_action_connection_details(GtkWidget *widget, gpointer data)
{
	/* Update the UI with the current options */
	update_conn_ui_from_conn_config();
	
	gtk_widget_show_all(GTK_WIDGET(window_conn));
	return;
}

/* Validate the connection, returning true or false */
static int
pgui_validate_connection()
{
	int i;
	
	if (strlen(conn->port))
	{
		for (i = 0; i < strlen(conn->port); i++)
		{
			if (!isdigit(conn->port[i]))
			{
				pgui_seterr(_("The connection port must be numeric!"));
				return 0;
			}
		}
	}
	
	return 1;
}

static void
pgui_sanitize_connection_string(char *connection_string)
{
	char *ptr = strstr(connection_string, "password");
	if ( ptr )
	{
		ptr += 10;
		while ( *ptr != '\'' && *ptr != '\0' )
		{
			/* If we find a \, hide both it and the next character */
			if ( *ptr == '\\' )
				*ptr++ = '*';
		
			*ptr++ = '*';
		}
	}
	return;
}

/*
 * We retain the ability to explicitly request a test of the connection
 * parameters.  This is the button signal handler to do so.
 */
static void
pgui_action_connection_okay(GtkWidget *widget, gpointer data)
{
	/* Update the configuration structure from the form */
	update_conn_config_from_conn_ui();
	
	/* Make sure have a valid connection first */
	if (!pgui_validate_connection())
	{
		pgui_raise_error_dialogue();
		return;
	}
	
	if (!connection_test())
	{
		pgui_logf(_("Connection failed."));
	
		/* If the connection failed, display a warning before closing */
		pgui_seterr(_("Unable to connect to the database - please check your connection settings"));
		pgui_raise_error_dialogue();
	}
	else
	{
		pgui_logf(_("Connection succeeded."));
	}
	
			
	/* Hide the window after the test */
	gtk_widget_hide(GTK_WIDGET(window_conn));
}


/* === Window creation functions === */

static void
pgui_create_about_dialog(void)
{
	const char *authors[] =
	{
		"Paul Ramsey <pramsey@opengeo.org>",
		"Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>",
		"Mark Leslie <mark.leslie@lisasoft.com>",
		NULL
	};

	dialog_about = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(dialog_about), _("Shape to PostGIS"));
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog_about), GUI_RCSID);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog_about), "http://postgis.org/");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog_about), authors);
}

static void
pgui_create_filechooser_dialog(void)
{
	GtkFileFilter *file_filter_shape;
	
	/* Create the dialog */
	dialog_filechooser= gtk_file_chooser_dialog_new( _("Select a Shape File"), GTK_WINDOW (window_main), 
		GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	/* Filter for .shp files */
	file_filter_shape = gtk_file_filter_new();
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(file_filter_shape), "*.shp");
	gtk_file_filter_set_name(GTK_FILE_FILTER(file_filter_shape), _("Shape Files (*.shp)"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog_filechooser), file_filter_shape);

	/* Filter for .dbf files */
	file_filter_shape = gtk_file_filter_new();
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(file_filter_shape), "*.dbf");
	gtk_file_filter_set_name(GTK_FILE_FILTER(file_filter_shape), _("DBF Files (*.dbf)"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog_filechooser), file_filter_shape);

	return;
}

static void
pgui_create_progress_dialog()
{
	GtkWidget *vbox_progress, *table_progress;
	
	dialog_progress = gtk_dialog_new_with_buttons(_("Working..."), GTK_WINDOW(window_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	gtk_window_set_modal(GTK_WINDOW(dialog_progress), TRUE);
	gtk_window_set_keep_above(GTK_WINDOW(dialog_progress), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(dialog_progress), 640, -1);

	/* Use a vbox as the base container */
	vbox_progress = gtk_dialog_get_content_area(GTK_DIALOG(dialog_progress));
	gtk_box_set_spacing(GTK_BOX(vbox_progress), 15);
	
	/* Create a table within the vbox */
	table_progress = gtk_table_new(2, 1, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (table_progress), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table_progress), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table_progress), 10);
	
	/* Text for the progress bar */
	label_progress = gtk_label_new("");
	gtk_table_attach_defaults(GTK_TABLE(table_progress), label_progress, 0, 1, 0, 1);
	
	/* Progress bar for the import */
	progress = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(progress), GTK_PROGRESS_LEFT_TO_RIGHT);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0.0);
	gtk_table_attach_defaults(GTK_TABLE(table_progress), progress, 0, 1, 1, 2);
	
	/* Add the table to the vbox */
	gtk_box_pack_start(GTK_BOX(vbox_progress), table_progress, FALSE, FALSE, 0);
	
	/* Add signal for cancel button */
	g_signal_connect(dialog_progress, "response", G_CALLBACK(pgui_action_progress_cancel), dialog_progress);
	
	/* Make sure we catch a delete event too */
	gtk_signal_connect(GTK_OBJECT(dialog_progress), "delete_event", GTK_SIGNAL_FUNC(pgui_action_progress_delete), NULL);
	
	return;
}

static void
pgui_create_options_dialog_add_label(GtkWidget *table, const char *str, gfloat alignment, int row)
{
	GtkWidget *align = gtk_alignment_new(alignment, 0.5, 0.0, 1.0);
	GtkWidget *label = gtk_label_new(str);
	gtk_table_attach_defaults(GTK_TABLE(table), align, 1, 3, row, row + 1);
	gtk_container_add(GTK_CONTAINER (align), label);
}

static void
pgui_create_options_dialog()
{
	GtkWidget *table_options;
	GtkWidget *align_options_center;
	static int text_width = 12;

	dialog_options = gtk_dialog_new_with_buttons(_("Import Options"), GTK_WINDOW(window_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_window_set_modal (GTK_WINDOW(dialog_options), TRUE);
	gtk_window_set_keep_above (GTK_WINDOW(dialog_options), TRUE);
	gtk_window_set_default_size (GTK_WINDOW(dialog_options), 180, 200);

	table_options = gtk_table_new(7, 3, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (table_options), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table_options), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table_options), 10);

	pgui_create_options_dialog_add_label(table_options, _("DBF file character encoding"), 0.0, 0);
	entry_options_encoding = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_options_encoding), text_width);
	gtk_table_attach_defaults(GTK_TABLE(table_options), entry_options_encoding, 0, 1, 0, 1 );

	pgui_create_options_dialog_add_label(table_options, _("Preserve case of column names"), 0.0, 1);
	checkbutton_options_preservecase = gtk_check_button_new();
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 1, 2 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_preservecase);

	pgui_create_options_dialog_add_label(table_options, _("Do not create 'bigint' columns"), 0.0, 2);
	checkbutton_options_forceint = gtk_check_button_new();
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 2, 3 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_forceint);

	pgui_create_options_dialog_add_label(table_options, _("Create spatial index automatically after load"), 0.0, 3);
	checkbutton_options_autoindex = gtk_check_button_new();
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 3, 4 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_autoindex);

	pgui_create_options_dialog_add_label(table_options, _("Load only attribute (dbf) data"), 0.0, 4);
	checkbutton_options_dbfonly = gtk_check_button_new();
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 4, 5 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_dbfonly);

	pgui_create_options_dialog_add_label(table_options, _("Load data using COPY rather than INSERT"), 0.0, 5);
	checkbutton_options_dumpformat = gtk_check_button_new();
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 0.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 5, 6 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_dumpformat);

	pgui_create_options_dialog_add_label(table_options, _("Load into GEOGRAPHY column"), 0.0, 6);
	checkbutton_options_geography = gtk_check_button_new();
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 6, 7 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_geography);

	/* Catch the response from the dialog */
	g_signal_connect(dialog_options, "response", G_CALLBACK(pgui_action_options_close), dialog_options);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog_options)->vbox), table_options, FALSE, FALSE, 0);
	
	/* Hook the delete event so we don't destroy the dialog (just hide) if cancelled */
	gtk_signal_connect(GTK_OBJECT(dialog_options), "delete_event", GTK_SIGNAL_FUNC(pgui_event_popup_delete), NULL);
}

/*
 * This function creates the UI artefacts for the file list table and hooks
 * up all the pretty signals.
 */
static void
pgui_create_file_table(GtkWidget *frame_shape)
{
	GtkWidget *vbox_tree;
	GtkWidget *sw;
	GtkTreeIter iter;
	gint *column_indexes;
	
	gtk_container_set_border_width (GTK_CONTAINER (frame_shape), 0);

	vbox_tree = gtk_vbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(vbox_tree), 5);
	gtk_container_add(GTK_CONTAINER(frame_shape), vbox_tree);

	/* Setup a model */
	list_store = gtk_list_store_new(N_COLUMNS,
					 G_TYPE_POINTER,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_BOOLEAN);
	
	/* Create the view and such */
	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
	
	/* GTK has a slightly brain-dead API in that you can't directly find
	   the column being used by a GtkCellRenderer when using the same
	   callback to handle multiple fields; hence we manually store this
	   information here and pass a pointer to the column index into
	   the signal handler */
	column_indexes = g_malloc(sizeof(gint) * N_COLUMNS);
	
	/* Make the tree view in a scrollable window */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(sw, -1, 150);
	
	gtk_box_pack_start(GTK_BOX(vbox_tree), sw, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER (sw), tree);

	/* Place the "Add File" button below the list view */
	add_file_button = gtk_button_new_with_label(_("Add File"));
	gtk_container_add (GTK_CONTAINER (vbox_tree), add_file_button);
	
	/* Filename Field */
	filename_renderer = gtk_cell_renderer_text_new();
	g_object_set(filename_renderer, "editable", FALSE, NULL);
	column_indexes[FILENAME_COLUMN] = FILENAME_COLUMN;
	g_signal_connect(G_OBJECT(filename_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), &column_indexes[FILENAME_COLUMN]);
	filename_column = gtk_tree_view_column_new_with_attributes(_("Shapefile"),
	                  filename_renderer,
	                  "text",
	                  FILENAME_COLUMN,
	                  NULL);
	g_object_set(filename_column, "resizable", TRUE, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), filename_column);

	/* Schema Field */
	schema_renderer = gtk_cell_renderer_text_new();
	g_object_set(schema_renderer, "editable", TRUE, NULL);
	column_indexes[SCHEMA_COLUMN] = SCHEMA_COLUMN;
	g_signal_connect(G_OBJECT(schema_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), &column_indexes[SCHEMA_COLUMN]);
	schema_column = gtk_tree_view_column_new_with_attributes(_("Schema"),
	                schema_renderer,
	                "text",
	                SCHEMA_COLUMN,
	                NULL);
	g_object_set(schema_column, "resizable", TRUE, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), schema_column);

	/* Table Field */
	table_renderer = gtk_cell_renderer_text_new();
	g_object_set(table_renderer, "editable", TRUE, NULL);
	column_indexes[TABLE_COLUMN] = TABLE_COLUMN;
	g_signal_connect(G_OBJECT(table_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), &column_indexes[TABLE_COLUMN]);
	table_column = gtk_tree_view_column_new_with_attributes("Table",
	               table_renderer,
	               "text",
	               TABLE_COLUMN,
	               NULL);
	g_object_set(schema_column, "resizable", TRUE, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), table_column);

	/* Geo column field */
	geom_column_renderer = gtk_cell_renderer_text_new();
	g_object_set(geom_column_renderer, "editable", TRUE, NULL);
	column_indexes[GEOMETRY_COLUMN] = GEOMETRY_COLUMN;
	g_signal_connect(G_OBJECT(geom_column_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), &column_indexes[GEOMETRY_COLUMN]);
	geom_column = gtk_tree_view_column_new_with_attributes(_("Geometry Column"),
	              geom_column_renderer,
	              "text",
	              GEOMETRY_COLUMN,
	              NULL);
	g_object_set(geom_column, "resizable", TRUE, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), geom_column);

	/* SRID Field */
	srid_renderer = gtk_cell_renderer_text_new();
	g_object_set(srid_renderer, "editable", TRUE, NULL);
	column_indexes[SRID_COLUMN] = SRID_COLUMN;
	g_signal_connect(G_OBJECT(srid_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), &column_indexes[SRID_COLUMN]);
	srid_column = gtk_tree_view_column_new_with_attributes("SRID",
	              srid_renderer,
	              "text",
	              SRID_COLUMN,
	              NULL);
	g_object_set(srid_column, "resizable", TRUE, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);  
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), srid_column);

	/* Mode Combo Field */
	combo_list = gtk_list_store_new(COMBO_COLUMNS, 
					G_TYPE_STRING,
					G_TYPE_CHAR);
	
	gtk_list_store_insert(combo_list, &iter, CREATE_MODE);
	gtk_list_store_set(combo_list, &iter,
	                   COMBO_TEXT, _("Create"), 
			   COMBO_OPTION_CHAR, 'c',			   
			   -1);
	gtk_list_store_insert(combo_list, &iter, APPEND_MODE);
	gtk_list_store_set(combo_list, &iter,
	                   COMBO_TEXT, _("Append"), 
			   COMBO_OPTION_CHAR, 'a', 
			   -1);
	gtk_list_store_insert(combo_list, &iter, DELETE_MODE);
	gtk_list_store_set(combo_list, &iter,
	                   COMBO_TEXT, _("Delete"), 
			   COMBO_OPTION_CHAR, 'd', 
			   -1);
	gtk_list_store_insert(combo_list, &iter, PREPARE_MODE);
	gtk_list_store_set(combo_list, &iter,
	                   COMBO_TEXT, _("Prepare"), 
			   COMBO_OPTION_CHAR, 'p', 
			   -1);
	mode_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(combo_list));
	mode_renderer = gtk_cell_renderer_combo_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mode_combo),
	                           mode_renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(mode_combo),
	                              mode_renderer, "text", 0);
	g_object_set(mode_renderer,
	             "model", combo_list,
	             "editable", TRUE,
	             "has-entry", FALSE,
	             "text-column", COMBO_TEXT,
	             NULL);
	mode_column = gtk_tree_view_column_new_with_attributes(_("Mode"),
	              mode_renderer,
	              "text",
	              MODE_COLUMN,
	              NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), mode_column);
	gtk_combo_box_set_active(GTK_COMBO_BOX(mode_combo), 1);

	g_signal_connect (G_OBJECT(mode_renderer), "changed", G_CALLBACK(pgui_action_handle_tree_combo), NULL);

	/* Remove Field */
	remove_renderer = gtk_cell_renderer_toggle_new();
	g_object_set(remove_renderer, "activatable", TRUE, NULL);
	g_signal_connect(G_OBJECT(remove_renderer), "toggled", G_CALLBACK (pgui_action_handle_tree_remove), NULL);
	remove_column = gtk_tree_view_column_new_with_attributes("Rm",
	                remove_renderer, NULL);
	g_object_set(remove_column, "resizable", TRUE, "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), remove_column);

	g_signal_connect (G_OBJECT (add_file_button), "clicked", G_CALLBACK (pgui_action_open_file_dialog), NULL);

	/* Drag n Drop wiring */
	GtkTargetEntry drop_types[] =
	{
		{ "text/uri-list", 0, 0}
	};
	
	gint n_drop_types = sizeof(drop_types)/sizeof(drop_types[0]);
	gtk_drag_dest_set(GTK_WIDGET(tree),
	                  GTK_DEST_DEFAULT_ALL,
	                  drop_types, n_drop_types,
	                  GDK_ACTION_COPY);
	g_signal_connect(G_OBJECT(tree), "drag_data_received",
	                 G_CALLBACK(pgui_action_handle_file_drop), NULL);
}

static void
pgui_create_connection_window()
{
	/* Default text width */
	static int text_width = 12;
	
	/* Vbox container */
	GtkWidget *vbox;
	
	/* Reusable label handle */
	GtkWidget *label;

	/* PgSQL section */
	GtkWidget *frame_pg, *table_pg;
	
	/* OK button */
	GtkWidget *button_okay;

	/* Create the main top level window with a 10px border */
	window_conn = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window_conn), 10);
	gtk_window_set_title(GTK_WINDOW(window_conn), _("PostGIS connection"));
	gtk_window_set_position(GTK_WINDOW(window_conn), GTK_WIN_POS_CENTER);
	gtk_window_set_modal(GTK_WINDOW(window_conn), TRUE);
	
	/* Use a vbox as the base container */
	vbox = gtk_vbox_new(FALSE, 15);
	
	/*
	** PostGIS info in a table
	*/
	frame_pg = gtk_frame_new(_("PostGIS Connection"));
	table_pg = gtk_table_new(5, 3, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (table_pg), 8);
	gtk_table_set_col_spacings(GTK_TABLE(table_pg), 7);
	gtk_table_set_row_spacings(GTK_TABLE(table_pg), 3);

	/* User name row */
	label = gtk_label_new(_("Username:"));
	entry_pg_user = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 0, 1 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_user, 1, 3, 0, 1 );

	/* Password row */
	label = gtk_label_new(_("Password:"));
	entry_pg_pass = gtk_entry_new();
	gtk_entry_set_visibility( GTK_ENTRY(entry_pg_pass), FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 1, 2 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_pass, 1, 3, 1, 2 );

	/* Host and port row */
	label = gtk_label_new(_("Server Host:"));
	entry_pg_host = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_pg_host), text_width);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 2, 3 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_host, 1, 2, 2, 3 );

	entry_pg_port = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_pg_port), 8);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_port, 2, 3, 2, 3 );

	/* Database row */
	label = gtk_label_new(_("Database:"));
	entry_pg_db   = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 3, 4 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_db, 1, 3, 3, 4 );
			 
	/* Add table into containing frame */
	gtk_container_add(GTK_CONTAINER(frame_pg), table_pg);

	/* Add frame into containing vbox */
	gtk_container_add(GTK_CONTAINER(window_conn), vbox);
	
	/* Add the vbox into the window */
	gtk_container_add(GTK_CONTAINER(vbox), frame_pg);
	
	/* Create a simple "OK" button for the dialog */
	button_okay = gtk_button_new_with_label(_("OK"));
	gtk_container_add(GTK_CONTAINER(vbox), button_okay);
	g_signal_connect(G_OBJECT(button_okay), "clicked", G_CALLBACK(pgui_action_connection_okay), NULL);
	
	/* Hook the delete event so we don't destroy the dialog (only hide it) if cancelled */
	gtk_signal_connect(GTK_OBJECT(window_conn), "delete_event", GTK_SIGNAL_FUNC(pgui_event_popup_delete), NULL);
	
	return;
}	

static void
pgui_create_main_window(const SHPCONNECTIONCONFIG *conn)
{
	/* Main widgets */
	GtkWidget *vbox_main, *vbox_loader;
	
	/* PgSQL section */
	GtkWidget *frame_pg, *frame_shape, *frame_log;
	GtkWidget *button_pg_conn;
	
	/* Notebook */
	GtkWidget *notebook;
	
	/* Button section */
	GtkWidget *hbox_buttons, *button_options, *button_import, *button_cancel, *button_about;
	
	/* Log section */
	GtkWidget *scrolledwindow_log;

	/* Create the main top level window with a 10px border */
	window_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window_main), 10);
	gtk_window_set_title(GTK_WINDOW(window_main), _("Shape File to PostGIS Importer"));
	gtk_window_set_position(GTK_WINDOW(window_main), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_resizable(GTK_WINDOW(window_main), FALSE);
	
	/* Open it a bit wider so that both the label and title show up */
	gtk_window_set_default_size(GTK_WINDOW(window_main), 180, 500);
	
	/* Connect the destroy event of the window with our pgui_quit function
	*  When the window is about to be destroyed we get a notificaiton and
	*  stop the main GTK loop
	*/
	g_signal_connect(G_OBJECT(window_main), "destroy", G_CALLBACK(pgui_quit), NULL);

	/* Connection row */
	frame_pg = gtk_frame_new(_("PostGIS Connection"));
	
	/* Test button row */
	button_pg_conn = gtk_button_new_with_label(_("View connection details..."));
	g_signal_connect(G_OBJECT(button_pg_conn), "clicked", G_CALLBACK(pgui_action_connection_details), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(button_pg_conn), 10);
	gtk_container_add(GTK_CONTAINER(frame_pg), button_pg_conn);
	
	/*
	 * GTK Notebook for selecting import/export
	 */
	notebook = gtk_notebook_new();
	
	/*
	** Shape file selector
	*/
	frame_shape = gtk_frame_new(_("Import List"));
	pgui_create_file_table(frame_shape);

	/*
	** Row of action buttons
	*/
	hbox_buttons = gtk_hbox_new(TRUE, 15);
	gtk_container_set_border_width (GTK_CONTAINER (hbox_buttons), 0);
	/* Create the buttons themselves */
	button_options = gtk_button_new_with_label(_("Options..."));
	button_import = gtk_button_new_with_label(_("Import"));
	button_cancel = gtk_button_new_with_label(_("Cancel"));
	button_about = gtk_button_new_with_label(_("About"));
	/* Add actions to the buttons */
	g_signal_connect (G_OBJECT (button_import), "clicked", G_CALLBACK (pgui_action_import), NULL);
	g_signal_connect (G_OBJECT (button_options), "clicked", G_CALLBACK (pgui_action_options_open), NULL);
	g_signal_connect (G_OBJECT (button_cancel), "clicked", G_CALLBACK (pgui_action_cancel), NULL);
	g_signal_connect (G_OBJECT (button_about), "clicked", G_CALLBACK (pgui_action_about_open), NULL);
	/* And insert the buttons into the hbox */
	gtk_box_pack_start(GTK_BOX(hbox_buttons), button_options, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_cancel, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_about, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_buttons), button_import, TRUE, TRUE, 0);

	/*
	** Log window
	*/
	frame_log = gtk_frame_new(_("Log Window"));
	gtk_container_set_border_width (GTK_CONTAINER (frame_log), 0);
	gtk_widget_set_size_request(frame_log, -1, 200);
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

	/* Add the loader frames into the notebook page */
	vbox_loader = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(vbox_loader), 10);
	
	gtk_box_pack_start(GTK_BOX(vbox_loader), frame_shape, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_loader), hbox_buttons, FALSE, FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox_loader, gtk_label_new(_("Import")));
	
	/* Add the frames into the main vbox */
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_pg, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), notebook, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_log, TRUE, TRUE, 0);
	
	/* and insert the vbox into the main window  */
	gtk_container_add(GTK_CONTAINER(window_main), vbox_main);
	
	/* make sure that everything, window and label, are visible */
	gtk_widget_show_all(window_main);

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
	int c;

#ifdef USE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
#endif

	/* Parse command line options and set configuration */
	global_loader_config = malloc(sizeof(SHPLOADERCONFIG));
	set_loader_config_defaults(global_loader_config);

	/* Here we override any defaults for the GUI */
	global_loader_config->createindex = 1;
	global_loader_config->geo_col = strdup(GEOMETRY_DEFAULT);
	
	conn = malloc(sizeof(SHPCONNECTIONCONFIG));
	memset(conn, 0, sizeof(SHPCONNECTIONCONFIG));
	
	/* Here we override any defaults for the connection */
	conn->host = strdup("localhost");
	conn->port = strdup("5432");

	while ((c = pgis_getopt(argc, argv, "U:p:W:d:h:")) != -1)
	{
		switch (c)
		{
		case 'U':
			conn->username = strdup(pgis_optarg);
			break;
		case 'p':
			conn->port = strdup(pgis_optarg);
			break;
		case 'W':
			conn->password = strdup(pgis_optarg);
			break;
		case 'd':
			conn->database = strdup(pgis_optarg);
			break;
		case 'h':
			conn->host = strdup(pgis_optarg);
			break;
		default:
			usage();
			free(conn);
			free(global_loader_config);
			exit(0);
		}
	}

	/* initialize the GTK stack */
	gtk_init(&argc, &argv);
	
	/* set up the user interface */
	pgui_create_main_window(conn);
	pgui_create_connection_window();
	pgui_create_options_dialog();
	pgui_create_about_dialog();
	pgui_create_filechooser_dialog();
	pgui_create_progress_dialog();
		
	/* start the main loop */
	gtk_main();

	/* Free the configuration */
	free(conn);
	free(global_loader_config);

	return 0;
}
