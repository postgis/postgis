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
#include "structure.h"

#define GUI_RCSID "shp2pgsql-gui $Revision$"
#define SHAPEFIELDMAXWIDTH 60
#define SHAPEFIELDMINWIDTH 30

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
static GtkWidget *label_pg_connection_test = NULL;
static GtkWidget *textview_log = NULL;
static GtkWidget *add_file_button = NULL;
static GtkWidget *progress = NULL;
static GtkTextBuffer *textbuffer_log = NULL;
static int current_list_index = 0;
static int valid_connection = 0;

/* Tree View Stuffs */

enum
{
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
	COMBO_COLUMNS
};

enum
{
	CREATE_MODE,
	APPEND_MODE,
	DELETE_MODE,
	PREPARE_MODE
};


static void pgui_logf(const char *fmt, ...);
static void pgui_log_va(const char *fmt, va_list ap);


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

/* Options window */
static GtkWidget *entry_options_encoding = NULL;
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
static void pgui_create_file_table(GtkWidget *frame_shape);
static void pgui_action_shape_file_set(const char *gtk_filename);
static void pgui_action_handle_file_drop(GtkWidget *widget,
        GdkDragContext *dc,
        gint x, gint y,
        GtkSelectionData *selection_data,
        guint info, guint t, gpointer data);
static int validate_string(char *string);
static int validate_shape_file(FILENODE *filenode);
static int validate_shape_filename(const char *filename);
static void pgui_set_config_from_options_ui(void);
static void pgui_set_config_from_ui(FILENODE *file);
static void pgui_sanitize_connection_string(char *connection_string);
static char *pgui_read_connection(void);

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
 * Ensures that the field width is within the stated bounds, and
 * 'appropriately' sized, for some definition of 'appropriately'.
 */
static void
set_filename_field_width(void)
{
	FILENODE *node;
	int needed_width = -1;
	int i;

	node = get_next_node(NULL);
	while (node)
	{
		i = strlen(node->filename);
		if (i > needed_width)
		{
			needed_width = i;
		}
		node = get_next_node(node);
	}

	if (needed_width < SHAPEFIELDMINWIDTH)
	{
		g_object_set(filename_renderer, "width-chars", SHAPEFIELDMINWIDTH, NULL);
	}
	else if (needed_width > SHAPEFIELDMAXWIDTH)
	{
		g_object_set(filename_renderer, "width-chars", SHAPEFIELDMAXWIDTH, NULL);
	}
	else
	{
		g_object_set(filename_renderer, "width-chars", -1, NULL);
	}

}

/*
 * Signal handler for the remove box.  Performs no user interaction, simply
 * removes the FILENODE from the list and the row from the table.
 */
static void
pgui_action_handle_tree_remove(GtkCellRendererToggle *renderer,
                               gchar *path,
                               gpointer user_data)
{
	GtkTreeIter iter;
	FILENODE *file_node;
	int index;

	/*
	 * First item of business, find me a GtkTreeIter.
	 * Second item, find the correct FILENODE
	 */
	if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(list_store), &iter,
	        path))
	{
		pgui_logf(_("Problem retrieving the edited row."));
		return;
	}

	index = atoi(path);
	if (index >= current_list_index)
		return;

	file_node = find_file_by_index(index);
	if (file_node == NULL)
	{
		/*
		 * If we can't find the struct, we shouldn't update the ui.
		 * That would just be misleading.
		 */
		pgui_logf(_("Problem finding the correct file."));
		return;
	}

	/* Remove the row from the list */
	if (!gtk_list_store_remove(list_store, &iter))
	{
		pgui_logf(_("Unable to remove row."));
		return;
	}

	current_list_index--;
	remove_file(file_node);

	set_filename_field_width();
}

/*
 * Ensures that the given file has a .shp extension.
 * This function will return a new string, come hell or high water, so free it.
 */
static char*
ensure_shapefile(char *filein)
{
	char *fileout;
	char *p;

	p = filein;
	while (*p)
		p++;
	p--;
	while (g_ascii_isspace(*p))
		p--;
	while (*p && p > filein && *p != '.')
		p--;

	if (strcmp(p, ".shp") == 0
	        || strcmp(p, ".SHP") == 0
	        || strcmp(p, ".Shp") == 0)
		return strdup(filein);

	/* if there is no extension, let's add one. */
	if (p == filein)
	{
		fileout = malloc(strlen(filein) + 5);
		strcpy(fileout, filein);
		strcat(fileout, ".shp");
		return fileout;
	}

	/* p is on the '.', so we need to remember not to copy it. */
	pgui_logf("mallocing %d, from %p %p", p - filein + 5, p, filein);
	fileout = malloc(p - filein + 5);
	strncpy(fileout, filein, p - filein);
	fileout[p - filein] = '\0';
	pgui_logf("b: %s", fileout);
	strcat(fileout, ".shp");

	pgui_logf("Rewritten as %s", fileout);
	return fileout;
}

/*
 * Creates a single file row in the list table given the URI of a file.
 */
static void
process_single_uri(char *uri)
{
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

	pgui_action_shape_file_set(filename);

	g_free(filename);
	g_free(hostname);

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
 * It's fairly small-minded, but is where the char's representing the various
 * modes in the FILENODE are hardcoded.
 */
static void
pgui_action_handle_tree_combo(GtkCellRendererCombo *combo,
                              gchar *path_string,
                              GtkTreeIter *new_iter,
                              gpointer user_data)
{
	GtkTreeIter iter;
	FILENODE *file_node;
	int index;

	/*
	 * First item of business, find me a GtkTreeIter.
	 * Second item, find the correct FILENODE
	 */
	if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(list_store), &iter,
	        path_string))
	{
		pgui_logf(_("Problem retrieving the edited row."));
		return;
	}

	index = atoi(path_string);
	file_node = find_file_by_index(index);
	if (file_node == NULL)
	{
		/*
		 * If we can't find the struct, we shouldn't update the ui.
		 * That would just be misleading.
		 */
		pgui_logf(_("Problem finding the correct file."));
		return;
	}

	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(combo_list), new_iter);
	const char *str_path = gtk_tree_path_to_string(path);
	index = atoi(str_path);
	gtk_tree_path_free(path);

	if (index == APPEND_MODE)
	{
		file_node->mode = 'a';
		gtk_list_store_set(list_store, &iter,
		                   MODE_COLUMN, _("Append"),
		                   -1);
	}
	else if (index == DELETE_MODE)
	{
		file_node->mode = 'd';
		gtk_list_store_set(list_store, &iter,
		                   MODE_COLUMN, _("Delete"),
		                   -1);
	}
	else if (index == PREPARE_MODE)
	{
		file_node->mode = 'p';
		gtk_list_store_set(list_store, &iter,
		                   MODE_COLUMN, _("Prepare"),
		                   -1);
	}
	else
	{

		file_node->mode = 'c';
		gtk_list_store_set(list_store, &iter,
		                   MODE_COLUMN, _("Create"),
		                   -1);
	}
	validate_shape_file(file_node);

}

/*
 * This method will generate a file row in the list table given an edit
 * of a single column.  Most fields will contain defaults, but a filename
 * generally can't be created from the ether, so faking that up is still
 * a bit weak.
 */
static void
generate_file_bits(GtkCellRendererText *renderer, char *new_text)
{
	GtkTreeIter iter;
	FILENODE *file_node;
	char *filename;
	char *schema;
	char *table;
	char *geom_column;
	char *srid;

	if (renderer == GTK_CELL_RENDERER_TEXT(filename_renderer))
	{
		/* If we've been given a filename, we can use the existing method. */
		pgui_logf(_("Setting filename to %s"), new_text);
		pgui_action_shape_file_set(ensure_shapefile(new_text));
		return;
	}
	else if (renderer == GTK_CELL_RENDERER_TEXT(table_renderer))
	{
		pgui_logf(_("Setting table to %s"), new_text);
		table = strdup(new_text);
		filename = malloc(strlen(new_text) + 5);
		sprintf(filename, "%s.shp", new_text);
	}
	else
	{
		pgui_logf(_("Default filename / table."));
		filename = "";
		table = "new_table";
	}
	if (renderer == GTK_CELL_RENDERER_TEXT(schema_renderer))
	{
		pgui_logf(_("Setting schema to %s"), new_text);
		schema = strdup(new_text);
	}
	else
	{
		pgui_logf(_("Default schema."));
		schema = "public";
	}
	if (renderer == GTK_CELL_RENDERER_TEXT(geom_column_renderer))
	{
		pgui_logf(_("Setting geometry column to %s"), new_text);
		geom_column = strdup(new_text);
	}
	else
	{
		pgui_logf(_("Default geom_column"));
		if (config->geography)
			geom_column = GEOGRAPHY_DEFAULT;
		else
			geom_column = GEOMETRY_DEFAULT;

	}
	if (renderer == GTK_CELL_RENDERER_TEXT(srid_renderer))
	{
		pgui_logf(_("Setting srid to %s"), new_text);
		srid = strdup(new_text);
	}
	else
	{
		pgui_logf(_("Default SRID."));
		srid = "-1";
	}

	file_node = append_file(filename, schema, table, geom_column, srid, 'c', &iter);

	validate_shape_file(file_node);
	
	gtk_list_store_insert_with_values(
	    list_store, &iter, current_list_index++,
	    FILENAME_COLUMN, filename,
	    SCHEMA_COLUMN, schema,
	    TABLE_COLUMN, table,
	    GEOMETRY_COLUMN, geom_column,
	    SRID_COLUMN, srid,
	    MODE_COLUMN, "Create",
	    -1);
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
                             gpointer user_data)
{
	GtkTreeIter iter;
	FILENODE *file_node;
	int index;

	/* Empty doesn't fly */
	if (strlen(new_text) == 0)
		return;

	/*
	 * First item of business, find me a GtkTreeIter.
	 * Second item, find the correct FILENODE
	 */
	if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(list_store), &iter,
	        path))
	{
		pgui_logf(_("Problem retrieving the edited row."));
		return;
	}
	index = atoi(path);
	file_node = find_file_by_index(index);
	if (file_node == NULL)
	{
		/*
		 * If there is no file, it may be a new addition.
		 * Check the path against our current index to see if they're
		 * editing the empty row.
		 */
		int index = atoi(path);
		if (index == current_list_index)
		{
			generate_file_bits(renderer, new_text);
			return;
		}

		/*
		 * If we can't find (or create) the struct, we shouldn't update the ui.
		 * That would just be misleading.
		 */
		pgui_logf(_("Problem finding the correct file."));
		return;
	}

	if (renderer == GTK_CELL_RENDERER_TEXT(filename_renderer))
	{
		file_node->filename = ensure_shapefile(new_text);
		set_filename_field_width();
	}
	else if (renderer == GTK_CELL_RENDERER_TEXT(schema_renderer))
	{
		file_node->schema = strdup(new_text);
	}
	else if (renderer == GTK_CELL_RENDERER_TEXT(table_renderer))
	{
		file_node->table = strdup(new_text);
	}
	else if (renderer == GTK_CELL_RENDERER_TEXT(geom_column_renderer))
	{
		file_node->geom_column = strdup(new_text);
	}
	else if (renderer == GTK_CELL_RENDERER_TEXT(srid_renderer))
	{
		file_node->srid = strdup(new_text);
	}

	validate_shape_file(file_node);

	gtk_list_store_set(list_store, &iter,
	                   FILENAME_COLUMN, file_node->filename,
	                   SCHEMA_COLUMN, file_node->schema,
	                   TABLE_COLUMN, file_node->table,
	                   GEOMETRY_COLUMN, file_node->geom_column,
	                   SRID_COLUMN, file_node->srid,
	                   -1);
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
	destroy_file_list();
	gtk_main_quit ();
}

/* Set the global config variables controlled by the options dialogue */
static void
pgui_set_config_from_options_ui()
{
	FILENODE *current_node;
	const char *entry_encoding = gtk_entry_get_text(GTK_ENTRY(entry_options_encoding));
	gboolean preservecase = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_preservecase));
	gboolean forceint = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_forceint));
	gboolean createindex = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_autoindex));
	gboolean dbfonly = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_dbfonly));
	gboolean dumpformat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_dumpformat));
	gboolean geography = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_options_geography));

	if ( geography )
	{
		config->geography = 1;
		/* Flip the geometry column name to match the load type */
		current_node = get_next_node(NULL);
		while (current_node != NULL)
		{
			if (!strcmp(current_node->geom_column, GEOMETRY_DEFAULT))
			{
				free(current_node->geom_column);
				current_node->geom_column = strdup(GEOGRAPHY_DEFAULT);
				gtk_list_store_set(GTK_LIST_STORE(list_store),
				                   current_node->tree_iterator,
				                   GEOMETRY_COLUMN,
				                   GEOGRAPHY_DEFAULT, -1);
				free(config->geo_col);
				config->geo_col = strdup(GEOGRAPHY_DEFAULT);
			}
			current_node = get_next_node(current_node);
		}
	}
	else
	{
		config->geography = 0;
		/* Flip the geometry column name to match the load type */
		current_node = get_next_node(NULL);
		while (current_node != NULL)
		{
			if (!strcmp(current_node->geom_column, GEOGRAPHY_DEFAULT))
			{
				free(current_node->geom_column);
				current_node->geom_column = strdup(GEOMETRY_DEFAULT);
				gtk_list_store_set(GTK_LIST_STORE(list_store),
				                   current_node->tree_iterator,
				                   GEOMETRY_COLUMN,
				                   GEOMETRY_DEFAULT, -1);
				free(config->geo_col);
				config->geo_col = strdup(GEOMETRY_DEFAULT);
			}
			current_node = get_next_node(current_node);
		}
	}

	/* Encoding */
	if ( entry_encoding && strlen(entry_encoding) > 0 )
	{
		if (config->encoding)
			free(config->encoding);

		config->encoding = strdup(entry_encoding);
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
	{
		config->readshape = 0;
		/* There will be no spatial column so don't create a spatial index */
		config->createindex = 0; 
	}
	else
		config->readshape = 1;

	/* Use COPY rather than INSERT format */
	if ( dumpformat )
		config->dump_format = 1;
	else
		config->dump_format = 0;

	return;
}

/* Set the global configuration based upon the current UI */
static void
pgui_set_config_from_ui(FILENODE *file_node)
{
	const char *srid = strdup(file_node->srid);
	char *c;

	/* Set the destination schema, table and column parameters */
	if (config->table)
		free(config->table);

	config->table = strdup(file_node->table);

	if (config->schema)
		free(config->schema);

	if (strlen(file_node->schema) == 0)
		config->schema = strdup("public");
	else
		config->schema = strdup(file_node->schema);

	if (strlen(file_node->geom_column) == 0)
		config->geo_col = strdup(GEOMETRY_DEFAULT);
	else
		config->geo_col = strdup(file_node->geom_column);

	/* Set the destination filename: note the shp2pgsql core engine simply wants the file
	   without the .shp extension */
	if (config->shp_file)
		free(config->shp_file);

	/* Handle empty selection */
	if (file_node->filename == NULL)
		config->shp_file = strdup("");
	else
		config->shp_file = strdup(file_node->filename);

	/*  NULL-terminate the file name before the .shp extension */
	for (c = config->shp_file + strlen(config->shp_file); c >= config->shp_file; c--)
	{
		if (*c == '.')
		{
			*c = '\0';
			break;
		}
	}

	/* SRID */
	if ( srid == NULL || ! ( config->sr_id = atoi(srid) ) )
	{
		config->sr_id = -1;
	}

	/* Mode */
	if (file_node->mode == '\0')
		config->opt = 'c';
	else
		config->opt = file_node->mode;

	return;
}

/*
 * Performs rudimentary validation on the given string.  This takes the form
 * of checking for nullage and zero length, the trimming whitespace and
 * checking again for zero length.
 *
 * Returns 1 for valid, 0 for not.
 */
static int
validate_string(char *string)
{
	char *p, *q;
	if (string == NULL || strlen(string) == 0)
		return 0;

	p = string;
	while (g_ascii_isspace(*p))
		p++;
	q = p;
	while (*q)
		q++;
	q--;
	while (g_ascii_isspace(*q) && q > p)
		q--;
	if (p >= q)
		return 0;

	return 1;
}

/*
 * This compares two columns indicated state/result structs.  They are
 * assumed to already have had their names compared, so this will only
 * compare the types.  Precision remains TBD.
 *
 * 0 if the comparison fails, 1 if it's happy.
 */
static int
compare_columns(SHPLOADERSTATE *state, int dbf_index,
                PGresult *result, int db_index)
{
	char *string;
	int value = 1;
	int i = dbf_index;
	int j  = db_index;

	int dbTypeColumn = PQfnumber(result, "type");
	/* It would be nice to go into this level of detail, but not today. */
	/*
	int dbSizeColumn = PQfnumber(result, "length");
	int dbPrecisionColumn = PQfnumber(result, "precision");
	*/

	string = PQgetvalue(result, j, dbTypeColumn);
	switch (state->types[i])
	{
	case FTString:
		if (strcmp(string, "varchar") != 0)
		{
			pgui_logf(_("  DBF field %s is a varchar, while the table attribute is of type %s"), state->field_names[i], string);
			value = 0;
		}
		break;
	case FTDate:
		if (strcmp(string, "date") != 0)
		{
			pgui_logf(_("  DBF field %s is a date, while the table attribute is of type %s"), state->field_names[i], string);
			value = 0;
		}
		break;
	case FTInteger:
		if (state->widths[i] < 5 && !state->config->forceint4 && strcmp(string, "int2") != 0)
		{
			pgui_logf(_("  DBF field %s is an int2, while the table attribute is of type %s"), state->field_names[i], string);
			value = 0;
		}
		else if ((state->widths[i] < 10 || state->config->forceint4) && strcmp(string, "int4") != 0)
		{
			pgui_logf(_("  DBF field %s is an int4, while the table attribute is of type %s"), state->field_names[i], string);
			value = 0;
		}
		else if (strcmp(string, "numeric") != 0)
		{
			pgui_logf(_("  DBF field %s is a numeric, while the table attribute is of type %s"), state->field_names[i], string);
			value = 0;
		}

		break;
	case FTDouble:
		if (state->widths[i] > 18 && strcmp(string, "numeric") != 0)
		{
			pgui_logf(_("  DBF field %s is a numeric, while the table attribute is of type %s"), state->field_names[i], string);
			value = 0;
		}
		else if (state->widths[i] <= 18 && strcmp(string, "float8") != 0)
		{
			pgui_logf(_("  DBF field %s is a float8, while the table attribute is of type %s"), state->field_names[i], string);
			value = 0;
		}
		break;
	case FTLogical:
		if (strcmp(string, "boolean") != 0)
		{
			pgui_logf(_("  DBF field %s is a boolean, while the table attribue is of type %s"), state->field_names[i], string);
			value = 0;
		}
		break;
	/* 
	 * It should be safe to assume that we aren't going to 
	 * match an invalid column 
	 */
	case FTInvalid:
		value = 0;
		break;
	}
	return value;
}

/*
 * This will loop through each field defined in the DBF and find an attribute
 * in the table (provided by the PGresult) with the same name.  It then
 * delegates a comparison to the compare_columns function.
 *
 * 0 if all fields in the DBF cannot be matched to one in the table results,
 * 1 if they all can.
 */
static int
compare_column_lists(SHPLOADERSTATE *state, PGresult *result)
{
	int dbCount= PQntuples(result);
	int dbNameColumn = PQfnumber(result, "field");
	char **db_columns;
	int i, j, colgood;
	int value = 1;

	int shpCount = state->num_fields;

	db_columns = malloc(dbCount * sizeof(char*));
	for (j = 0; j < dbCount; j++)
	{
		db_columns[j] = strdup(PQgetvalue(result, j, dbNameColumn));
	}

	for (i = 0; i < shpCount; i++)
	{
		colgood = 0;
		for (j = 0; j < dbCount; j++)
		{
			if (strcmp(state->field_names[i], db_columns[j]) == 0)
			{
				value = value & compare_columns(state, i, result, j);
				colgood = 1;
				break;
			}
		}
		if (colgood == 0)
		{
			pgui_logf(_("   DBF field %s (%d) could not be matched to a table attribute."), state->field_names[i], i);
			value = 0;
		}
	}

	return value;
}

/*
 * Checks the file node for general completeness.
 * First all fields are checked to ensure they contain values.
 * Next the filename is checked to ensure it is stat'able (ie can be parsed
 * and is visible to the application).
 * Finally the database is checked.  What is done here depends on the load
 * mode as follows:
 *   Delete: nothing is checked.
 *   Create: check if the table already exists.
 *   Prepare: check if the table already exists.
 *   Append: check if the table is absent or if columns are missing or of the
 *           wrong type as defined in the DBF.
 *
 * returns 0 for error, 1 for warning, 2 for good
 */
static int
validate_shape_file(FILENODE *filenode)
{
	PGresult *result = NULL;
	int nresult;
	ExecStatusType status;
	char *connection_string;
	char *query;

	if (validate_string(filenode->filename) == 0
	        || validate_string(filenode->schema) == 0
	        || validate_string(filenode->table) == 0
	        || validate_string(filenode->srid) == 0)
	{
		pgui_logf(_("Incomplete, please fill in all fields."));
		return 0;
	}

	if (!validate_shape_filename(filenode->filename))
	{
		pgui_logf(_("Warning: Cannot find file %s"), filenode->filename);
		return 1;
	}

	if (filenode->mode != 'd')
	{
		int ret;
		SHPLOADERSTATE *state;

		pgui_set_config_from_options_ui();
		pgui_set_config_from_ui(filenode);

		state = ShpLoaderCreate(config);
		ret = ShpLoaderOpenShape(state);
		if (ret != SHPLOADEROK)
		{
			pgui_logf(_("Warning: Could not load shapefile %s"), filenode->filename);
			ShpLoaderDestroy(state);
			return 1;
		}

		if (valid_connection == 1)
		{
			const char *sql_form = "SELECT a.attnum, a.attname AS field, t.typname AS type, a.attlen AS length, a.atttypmod AS precision FROM pg_class c, pg_attribute a, pg_type t, pg_namespace n WHERE c.relname = '%s' AND n.nspname = '%s' AND a.attnum > 0 AND a.attrelid = c.oid AND a.atttypid = t.oid AND c.relnamespace = n.oid ORDER BY a.attnum";

			query = malloc(strlen(sql_form) + strlen(filenode->schema) + strlen(filenode->table) + 1);
			sprintf(query, sql_form, filenode->table, filenode->schema);

			if ( ! (connection_string = pgui_read_connection()) )
			{
				pgui_raise_error_dialogue();
				ShpLoaderDestroy(state);
				valid_connection = 0;
				return 0;
			}

			/* This has been moved into the earlier validation functions. */
			/*
			connection_sanitized = strdup(connection_string);
			pgui_sanitize_connection_string(connection_sanitized);
			pgui_logf("Connection: %s", connection_sanitized);
			free(connection_sanitized);
			*/

			if ( pg_connection ) PQfinish(pg_connection);
			pg_connection = PQconnectdb(connection_string);

			if (PQstatus(pg_connection) == CONNECTION_BAD)
			{
				pgui_logf(_("Warning: Database connection failed: %s"), PQerrorMessage(pg_connection));
				free(connection_string);
				free(query);
				PQfinish(pg_connection);
				pg_connection = NULL;
				ShpLoaderDestroy(state);
				return 1;
			}

			/*
			 * TBD: There is a horrible amount of switching/cleanup code
			 * here.  I would love to decompose this a bit better.
			 */

			result = PQexec(pg_connection, query);
			status = PQresultStatus(result);
			if (filenode->mode == 'a' && status != PGRES_TUPLES_OK)
			{
				pgui_logf(_("Warning: Append mode selected but no existing table found: %s"), filenode->table);
				PQclear(result);
				free(connection_string);
				free(query);
				PQfinish(pg_connection);
				pg_connection = NULL;
				ShpLoaderDestroy(state);
				return 1;
			}

			if (status == PGRES_TUPLES_OK)
			{
				nresult = PQntuples(result);
				if ((filenode->mode == 'c' || filenode->mode == 'p')
				        && nresult > 0)
				{
					if (filenode->mode == 'c')
						pgui_logf(_("Warning: Create mode selected for existing table name: %s"), filenode->table);
					else
						pgui_logf(_("Warning: Prepare mode selected for existing table name: %s"), filenode->table);

					PQclear(result);
					free(connection_string);
					free(query);
					PQfinish(pg_connection);
					pg_connection = NULL;
					ShpLoaderDestroy(state);
					return 1;
				}
				if (filenode->mode == 'a')
				{
					if (nresult == 0)
					{
						pgui_logf(_("Warning: Destination table (%s.%s) could not be found for appending."), filenode->schema, filenode->table);
						PQclear(result);
						free(connection_string);
						free(query);
						PQfinish(pg_connection);
						pg_connection = NULL;
						ShpLoaderDestroy(state);
						return 1;

					}
					int generated_columns = 2;
					if (config->readshape == 0)
						generated_columns = 1;
					pgui_logf(_("Validating schema of %s.%s against the shapefile %s."),
					          filenode->schema, filenode->table,
					          filenode->filename);
					if (compare_column_lists(state, result) == 0)
						ret = 1;
					else
						ret = 2;

					PQclear(result);
					free(connection_string);
					free(query);
					PQfinish(pg_connection);
					pg_connection = NULL;
					ShpLoaderDestroy(state);
					return ret;
				}
			}
			PQclear(result);
			free(connection_string);
			free(query);
			PQfinish(pg_connection);
			pg_connection = NULL;
			ShpLoaderDestroy(state);
		}
	}
	return 2;
}

/*
 * This checks the shapefile to ensure it exists.
 *
 * returns 0 for invalid, 1 for happy goodness
 */
static int
validate_shape_filename(const char *filename)
{
	struct stat buf;

	if (stat(filename, &buf) != 0)
	{
		return 0;
	}
	return 1;
}

/* Validate the configuration, returning true or false */
static int
pgui_validate_config()
{
	char *p;
	/* Validate table parameters */
	if ( ! config->table || strlen(config->table) == 0 )
	{
		pgui_seterr(_("Fill in the destination table."));
		return 0;
	}

	if ( ! config->schema || strlen(config->schema) == 0 )
	{
		pgui_seterr(_("Fill in the destination schema."));
		return 0;
	}

	if ( ! config->geo_col || strlen(config->geo_col) == 0 )
	{
		pgui_seterr(_("Fill in the destination column."));
		return 0;
	}

	if ( ! config->shp_file || strlen(config->shp_file) == 0 )
	{
		pgui_seterr(_("Select a shape file to import."));
		return 0;
	}

	p = malloc(strlen(config->shp_file) + 5);
	sprintf(p, "%s.shp", config->shp_file);
	if (validate_shape_filename(p) == 0)
	{
		const char *format = _("Unable to stat file: %s");
		char *s = malloc(strlen(p) + strlen(format) + 1);
		sprintf(s, format, p);
		pgui_seterr(s);
		free(s);
		free(p);
		return 0;
	}
	free(p);

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
	char *escape_pg_pass = NULL;

	if ( ! pg_host || strlen(pg_host) == 0 )
	{
		pgui_seterr(_("Fill in the server host."));
		return NULL;
	}
	if ( ! pg_port || strlen(pg_port) == 0 )
	{
		pgui_seterr(_("Fill in the server port."));
		return NULL;
	}
	if ( ! pg_user || strlen(pg_user) == 0 )
	{
		pgui_seterr(_("Fill in the user name."));
		return NULL;
	}
	if ( ! pg_db || strlen(pg_db) == 0 )
	{
		pgui_seterr(_("Fill in the database name."));
		return NULL;
	}
	if ( ! atoi(pg_port) )
	{
		pgui_seterr(_("Server port must be a number."));
		return NULL;
	}

	/* Escape the password in case it contains any special characters */
	escape_pg_pass = escape_connection_string((char *)pg_pass);

	if ( ! lw_asprintf(&connection_string, "user=%s password='%s' port=%s host=%s dbname=%s", pg_user, escape_pg_pass, pg_port, pg_host, pg_db) )
	{
		return NULL;
	}

	/* Free the escaped version */
	if (escape_pg_pass != pg_pass)
		free(escape_pg_pass);

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
 * This will create a connection to the database, just to see if it can.
 * It cleans up after itself like a good little function and maintains
 * the status of the valid_connection parameter.
 */
static int
connection_test(void)
{
	char *connection_string = NULL;
	char *connection_sanitized = NULL;

	if ( ! (connection_string = pgui_read_connection()) )
	{
		pgui_raise_error_dialogue();
		valid_connection = 0;
		return 0;
	}

	connection_sanitized = strdup(connection_string);
	pgui_sanitize_connection_string(connection_sanitized);
	pgui_logf("Connecting: %s", connection_sanitized);
	free(connection_sanitized);

	if ( pg_connection )
		PQfinish(pg_connection);

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

/*
 * This is a signal handler delegate used for validating connection
 * parameters as the user is changing them, prior to testing for a database
 * connection.
 */
static void
pgui_action_auto_connection_test()
{
	/*
	 * Since this isn't explicitly triggered, I don't want to error
	 * if we don't have enough information.
	 */
	if (validate_string((char*)gtk_entry_get_text(GTK_ENTRY(entry_pg_host))) == 0
	        || validate_string((char*)gtk_entry_get_text(GTK_ENTRY(entry_pg_port))) == 0
	        || validate_string((char*)gtk_entry_get_text(GTK_ENTRY(entry_pg_user))) == 0
	        || validate_string((char*)gtk_entry_get_text(GTK_ENTRY(entry_pg_pass))) == 0
	        || validate_string((char*)gtk_entry_get_text(GTK_ENTRY(entry_pg_db))) == 0)
		return;
	if (connection_test() == 1)
		pgui_logf(_("Database connection succeeded."));
	else
		pgui_logf(_("Database connection failed."));
}

/*
 * Signal handler for the connection parameter entry fields on activation.
 */
static void
pgui_action_auto_connection_test_activate(GtkWidget *entry, gpointer user_data)
{
	pgui_action_auto_connection_test();
}

/*
 * Signal handler for the connection parameter entry fields on loss of focus.
 *
 * Note that this must always return FALSE to allow subsequent event handlers
 * to be called.
 */
static gboolean
pgui_action_auto_connection_test_focus(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
	pgui_action_auto_connection_test();
	return FALSE;
}

/*
 * We retain the ability to explicitly request a test of the connection
 * parameters.  This is the button signal handler to do so.
 */
static void
pgui_action_connection_test(GtkWidget *widget, gpointer data)
{
	if (!connection_test())
	{
		gtk_label_set_text(GTK_LABEL(label_pg_connection_test), _("Connection failed."));
		pgui_logf( _("Connection failed.") );
		gtk_widget_show(label_pg_connection_test);

	}
	else
	{
		gtk_label_set_text(
	    	GTK_LABEL(label_pg_connection_test),
	    	_("Connection succeeded."));
		pgui_logf( _("Connection succeeded.") );
		gtk_widget_show(label_pg_connection_test);
	}
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
	pgui_set_config_from_options_ui();
	gtk_widget_destroy(widget);
	return;
}

static void
pgui_action_null(GtkWidget *widget, gpointer data)
{
	;
}

static void
pgui_action_open_file_dialog(GtkWidget *widget, gpointer data)
{
	GtkFileFilter *file_filter_shape;

	GtkWidget *file_chooser_dialog_shape = gtk_file_chooser_dialog_new( _("Select a Shape File"), GTK_WINDOW (window_main), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	pgui_logf("pgui_action_open_file_dialog called.");


	file_filter_shape = gtk_file_filter_new();
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(file_filter_shape), "*.shp");
	gtk_file_filter_set_name(GTK_FILE_FILTER(file_filter_shape), _("Shapefiles (*.shp)"));

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser_dialog_shape), file_filter_shape);

	if (gtk_dialog_run(GTK_DIALOG(file_chooser_dialog_shape))
	        == GTK_RESPONSE_ACCEPT)
	{
		pgui_action_shape_file_set(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser_dialog_shape)));
		gtk_widget_destroy(file_chooser_dialog_shape);
	}

}

/*
 * Given a filename, this function generates the default load parameters,
 * creates a new FILENODE in the file list and adds a row to the list table.
 */
static void
pgui_action_shape_file_set(const char *gtk_filename)
{
	GtkTreeIter iter;
	FILENODE *file;
	char *shp_file;
	int shp_file_len;
	int i;
	char *table_start;
	char *table_end;
	char *table;

	if ( gtk_filename )
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
	while ( *table_start != '/' && *table_start != '\\' && table_start > shp_file)
	{
		table_start--;
	}
	table_start++; /* Forward one to start of actual characters. */

	/* Roll back from end to first . character. */
	table_end = shp_file + shp_file_len;
	while ( *table_end != '.' && table_end > shp_file && table_end > table_start )
	{
		table_end--;
	}

	/* Copy the table name into a fresh memory slot. */
	table = malloc(table_end - table_start + 1);
	memcpy(table, table_start, table_end - table_start);
	table[table_end - table_start] = '\0';

	/* Force the table name to lower case */
	for(i = 0; i < table_end - table_start; i++)
	{
		if(isupper(table[i]) != 0)
		{
			table[i] = tolower(table[i]);
		}
	}

	/* Set the table name into the configuration */
	config->table = table;


	/*
	gtk_entry_set_text(GTK_ENTRY(entry_config_table), table);
	*/

	file = append_file(shp_file, "public", table, GEOMETRY_DEFAULT, "-1", 'c', &iter);

	set_filename_field_width();

	validate_shape_file(file);

	gtk_list_store_insert(list_store, &iter, current_list_index++);
	gtk_list_store_set(list_store, &iter,
	                   FILENAME_COLUMN, shp_file,
	                   SCHEMA_COLUMN, "public",
	                   TABLE_COLUMN, table,
	                   GEOMETRY_COLUMN, GEOMETRY_DEFAULT,
	                   SRID_COLUMN, "-1",
	                   MODE_COLUMN, _("Create"),
	                   -1);

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
	FILENODE *current_file;


	if ( ! (connection_string = pgui_read_connection() ) )
	{
		pgui_raise_error_dialogue();
		return;
	}

	current_file = get_next_node(NULL);
	if (current_file == NULL)
	{
		pgui_logf(_("File list is empty or uninitialised."));
		return;
	}

	while (current_file != NULL)
	{

		/*
		** Set the configuration from the UI and validate
		*/
		pgui_set_config_from_ui(current_file);
		if (! pgui_validate_config() )
		{
			pgui_raise_error_dialogue();

			current_file = get_next_node(current_file);
			continue;
		}

		pgui_logf("\n==============================");
		pgui_logf("Importing with configuration: %s, %s, %s, %s, mode=%c, dump=%d, simple=%d, geography=%d, index=%d, shape=%d, srid=%d", config->table, config->schema, config->geo_col, config->shp_file, config->opt, config->dump_format, config->simple_geometries, config->geography, config->createindex, config->readshape, config->sr_id);

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
			pgui_logf( _("Database connection failed: %s"), PQerrorMessage(pg_connection));
			gtk_label_set_text(GTK_LABEL(label_pg_connection_test), _("Connection failed."));
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

		import_running = TRUE;

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
			pgui_logf(_("Importing shapefile (%d records)..."), ShpLoaderGetRecordCount(state));

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

			if ( state->config->createindex )
			{
				pgui_logf(_("Creating spatial index...\n"));
			}

			/* Send the footer to the server */
			ret = pgui_exec(footer);
			free(footer);

			if (!ret)
				goto import_cleanup;
		}


import_cleanup:
		/* If we didn't finish inserting all of the items (and we expected to), an error occurred */
		if ((state->config->opt != 'p' && i != ShpLoaderGetRecordCount(state)) || !ret)
			pgui_logf(_("Shapefile import failed."));
		else
			pgui_logf(_("Shapefile import completed."));

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

		/* Disconnect from the database */
		PQfinish(pg_connection);
		pg_connection = NULL;

		current_file = get_next_node(current_file);
	}

	/* Tidy up */
	free(connection_string);
	free(dest_string);
	connection_string = dest_string = NULL;

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
pgui_action_about_open()
{
	GtkWidget *dlg;
	const char *authors[] =
	{
		"Paul Ramsey <pramsey@opengeo.org>",
		"Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>",
		"Mark Leslie <mark.leslie@lisasoft.com>",
		NULL
	};

	dlg = gtk_about_dialog_new ();
	gtk_about_dialog_set_name (GTK_ABOUT_DIALOG(dlg), _("Shape to PostGIS"));
	gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG(dlg), GUI_RCSID);
	/*	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(dlg), GUI_RCSID); */
	gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(dlg), "http://postgis.org/");
	gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(dlg), authors);
	g_signal_connect_swapped(dlg, "response", G_CALLBACK(gtk_widget_destroy), dlg);
	gtk_widget_show (dlg);
}

static void
pgui_create_options_dialogue()
{
	GtkWidget *table_options;
	GtkWidget *align_options_center;
	GtkWidget *dialog_options;
	static int text_width = 12;

	dialog_options = gtk_dialog_new_with_buttons (_("Import Options"), GTK_WINDOW(window_main), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_NONE, NULL);

	gtk_window_set_modal (GTK_WINDOW(dialog_options), TRUE);
	gtk_window_set_keep_above (GTK_WINDOW(dialog_options), TRUE);
	gtk_window_set_default_size (GTK_WINDOW(dialog_options), 180, 200);

	table_options = gtk_table_new(7, 3, TRUE);
	gtk_container_set_border_width (GTK_CONTAINER (table_options), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table_options), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table_options), 10);

	pgui_create_options_dialogue_add_label(table_options, _("DBF file character encoding"), 0.0, 0);
	entry_options_encoding = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(entry_options_encoding), text_width);
	gtk_entry_set_text(GTK_ENTRY(entry_options_encoding), config->encoding);
	gtk_table_attach_defaults(GTK_TABLE(table_options), entry_options_encoding, 0, 1, 0, 1 );

	pgui_create_options_dialogue_add_label(table_options, _("Preserve case of column names"), 0.0, 1);
	checkbutton_options_preservecase = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_preservecase), config->quoteidentifiers ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 1, 2 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_preservecase);

	pgui_create_options_dialogue_add_label(table_options, _("Do not create 'bigint' columns"), 0.0, 2);
	checkbutton_options_forceint = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_forceint), config->forceint4 ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 2, 3 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_forceint);

	pgui_create_options_dialogue_add_label(table_options, _("Create spatial index automatically after load"), 0.0, 3);
	checkbutton_options_autoindex = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_autoindex), config->createindex ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 3, 4 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_autoindex);

	pgui_create_options_dialogue_add_label(table_options, _("Load only attribute (dbf) data"), 0.0, 4);
	checkbutton_options_dbfonly = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON	(checkbutton_options_dbfonly), config->readshape ? FALSE : TRUE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 4, 5 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_dbfonly);

	pgui_create_options_dialogue_add_label(table_options, _("Load data using COPY rather than INSERT"), 0.0, 5);
	checkbutton_options_dumpformat = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON	(checkbutton_options_dumpformat), config->dump_format ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 0.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 5, 6 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_dumpformat);

	pgui_create_options_dialogue_add_label(table_options, _("Load into GEOGRAPHY column"), 0.0, 6);
	checkbutton_options_geography = gtk_check_button_new();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_options_geography), config->geography ? TRUE : FALSE);
	align_options_center = gtk_alignment_new( 0.5, 0.5, 0.0, 1.0 );
	gtk_table_attach_defaults(GTK_TABLE(table_options), align_options_center, 0, 1, 6, 7 );
	gtk_container_add (GTK_CONTAINER (align_options_center), checkbutton_options_geography);

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
	/* PgSQL section */
	GtkWidget *frame_pg, *frame_shape, *frame_log;
	GtkWidget *table_pg;
	GtkWidget *button_pg_test;
	/* Button section */
	GtkWidget *hbox_buttons, *button_options, *button_import, *button_cancel, *button_about;
	/* Log section */
	GtkWidget *scrolledwindow_log;

	/* create the main, top level, window */
	window_main = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	/* give the window a 10px wide border */
	gtk_container_set_border_width (GTK_CONTAINER (window_main), 10);

	/* give it the title */
	gtk_window_set_title (GTK_WINDOW (window_main), _("Shape File to PostGIS Importer"));

	/* open it a bit wider so that both the label and title show up */
	gtk_window_set_default_size (GTK_WINDOW (window_main), 180, 500);

	/* Connect the destroy event of the window with our pgui_quit function
	*  When the window is about to be destroyed we get a notificaiton and
	*  stop the main GTK loop
	*/
	g_signal_connect (G_OBJECT (window_main), "destroy", G_CALLBACK (pgui_quit), NULL);

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
	if ( conn->username )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_user), conn->username);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 0, 1 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_user, 1, 3, 0, 1 );
	g_signal_connect(G_OBJECT(entry_pg_user), "activate",
	                 G_CALLBACK(pgui_action_auto_connection_test), NULL);
	g_signal_connect(G_OBJECT(entry_pg_user), "focus-out-event",
	                 G_CALLBACK(pgui_action_auto_connection_test_focus), NULL);
	/* Password row */
	label = gtk_label_new(_("Password:"));
	entry_pg_pass = gtk_entry_new();
	if ( conn->password )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_pass), conn->password);
	gtk_entry_set_visibility( GTK_ENTRY(entry_pg_pass), FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 1, 2 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_pass, 1, 3, 1, 2 );
	g_signal_connect(G_OBJECT(entry_pg_pass), "activate",
	                 G_CALLBACK(pgui_action_auto_connection_test_activate), NULL);
	g_signal_connect(G_OBJECT(entry_pg_pass), "focus-out-event",
	                 G_CALLBACK(pgui_action_auto_connection_test_focus), NULL);
	/* Host and port row */
	label = gtk_label_new(_("Server Host:"));
	entry_pg_host = gtk_entry_new();
	if ( conn->host )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_host), conn->host);
	else
		gtk_entry_set_text(GTK_ENTRY(entry_pg_host), "localhost");
	gtk_entry_set_width_chars(GTK_ENTRY(entry_pg_host), text_width);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 2, 3 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_host, 1, 2, 2, 3 );
	g_signal_connect(G_OBJECT(entry_pg_host), "activate",
	                 G_CALLBACK(pgui_action_auto_connection_test_activate), NULL);
	g_signal_connect(G_OBJECT(entry_pg_host), "focus-out-event",
	                 G_CALLBACK(pgui_action_auto_connection_test_focus), NULL);
	entry_pg_port = gtk_entry_new();
	if ( conn->port )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_port), conn->port);
	else
		gtk_entry_set_text(GTK_ENTRY(entry_pg_port), "5432");
	gtk_entry_set_width_chars(GTK_ENTRY(entry_pg_port), 8);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_port, 2, 3, 2, 3 );
	g_signal_connect(G_OBJECT(entry_pg_port), "activate",
	                 G_CALLBACK(pgui_action_auto_connection_test_activate), NULL);
	g_signal_connect(G_OBJECT(entry_pg_port), "focus-out-event",
	                 G_CALLBACK(pgui_action_auto_connection_test_focus), NULL);
	/* Database row */
	label = gtk_label_new(_("Database:"));
	entry_pg_db   = gtk_entry_new();
	if ( conn->database )
		gtk_entry_set_text(GTK_ENTRY(entry_pg_db), conn->database);
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label, 0, 1, 3, 4 );
	gtk_table_attach_defaults(GTK_TABLE(table_pg), entry_pg_db, 1, 3, 3, 4 );
	g_signal_connect(G_OBJECT(entry_pg_db), "activate",
	                 G_CALLBACK(pgui_action_auto_connection_test_activate), NULL);
	g_signal_connect(G_OBJECT(entry_pg_db), "focus-out-event",
	                 G_CALLBACK(pgui_action_auto_connection_test_focus), NULL);
	/* Test button row */
	button_pg_test = gtk_button_new_with_label(_("Test Connection..."));
	gtk_table_attach_defaults(GTK_TABLE(table_pg), button_pg_test, 1, 2, 4, 5 );
	g_signal_connect (G_OBJECT (button_pg_test), "clicked", G_CALLBACK (pgui_action_connection_test), NULL);
	label_pg_connection_test = gtk_label_new("");
	gtk_table_attach_defaults(GTK_TABLE(table_pg), label_pg_connection_test, 2, 3, 4, 5 );
	/* Add table into containing frame */
	gtk_container_add (GTK_CONTAINER (frame_pg), table_pg);

	/*
	** Shape file selector
	*/
	frame_shape = gtk_frame_new(_("Shape File"));
	pgui_create_file_table(frame_shape);

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
	frame_log = gtk_frame_new(_("Import Log"));
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
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_pg, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_shape, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_buttons, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), progress, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_log, TRUE, TRUE, 0);
	/* and insert the vbox into the main window  */
	gtk_container_add (GTK_CONTAINER (window_main), vbox_main);
	/* make sure that everything, window and label, are visible */
	gtk_widget_show_all (window_main);

	return;
}

/*
 * This function creates the UI artefacts for the file list table and hooks
 * up all the pretty signals.
 */
static void
pgui_create_file_table(GtkWidget *frame_shape)
{
	GtkWidget *vbox_tree;

	gtk_container_set_border_width (GTK_CONTAINER (frame_shape), 0);

	vbox_tree = gtk_vbox_new(FALSE, 15);
	gtk_container_set_border_width(GTK_CONTAINER(vbox_tree), 5);
	gtk_container_add(GTK_CONTAINER(frame_shape), vbox_tree);

	add_file_button = gtk_button_new_with_label(_("Add File"));

	gtk_container_add (GTK_CONTAINER (vbox_tree), add_file_button);

	/* Setup a model */
	list_store = gtk_list_store_new (N_COLUMNS,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_STRING,
	                                 G_TYPE_BOOLEAN);
	/* Create file details list */
	init_file_list();
	/* Create the view and such */
	tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
	/* Make the tree view */
	gtk_box_pack_start(GTK_BOX(vbox_tree), tree, TRUE, TRUE, 0);

	/* Filename Field */
	filename_renderer = gtk_cell_renderer_text_new();
	set_filename_field_width();
	g_object_set(filename_renderer, "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(filename_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), NULL);
	filename_column = gtk_tree_view_column_new_with_attributes(_("Shapefile"),
	                  filename_renderer,
	                  "text",
	                  FILENAME_COLUMN,
	                  NULL);
	g_object_set(filename_column, "resizable", TRUE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), filename_column);

	/* Schema Field */
	schema_renderer = gtk_cell_renderer_text_new();
	g_object_set(schema_renderer, "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(schema_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), NULL);
	schema_column = gtk_tree_view_column_new_with_attributes(_("Schema"),
	                schema_renderer,
	                "text",
	                SCHEMA_COLUMN,
	                "background",
	                "white",
	                NULL);
	g_object_set(schema_column, "resizable", TRUE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), schema_column);

	/* Table Field */
	table_renderer = gtk_cell_renderer_text_new();
	g_object_set(table_renderer, "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(table_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), NULL);
	table_column = gtk_tree_view_column_new_with_attributes("Table",
	               table_renderer,
	               "text",
	               TABLE_COLUMN,
	               "background",
	               "white",
	               NULL);
	g_object_set(schema_column, "resizable", TRUE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), table_column);

	geom_column_renderer = gtk_cell_renderer_text_new();
	g_object_set(geom_column_renderer, "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(geom_column_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), NULL);
	geom_column = gtk_tree_view_column_new_with_attributes(_("Geometry Column"),
	              geom_column_renderer,
	              "text",
	              GEOMETRY_COLUMN,
	              "background",
	              "white",
	              NULL);
	g_object_set(geom_column, "resizable", TRUE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), geom_column);

	/* SRID Field */
	srid_renderer = gtk_cell_renderer_text_new();
	g_object_set(srid_renderer, "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(srid_renderer), "edited", G_CALLBACK(pgui_action_handle_tree_edit), NULL);
	srid_column = gtk_tree_view_column_new_with_attributes("SRID",
	              srid_renderer,
	              "text",
	              SRID_COLUMN,
	              "background",
	              "white",
	              NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), srid_column);

	/* Mode Combo */
	combo_list = gtk_list_store_new(COMBO_COLUMNS,
	                                G_TYPE_STRING);
	GtkTreeIter iter;
	gtk_list_store_insert(combo_list, &iter, CREATE_MODE);
	gtk_list_store_set(combo_list, &iter,
	                   COMBO_TEXT, _("Create"), -1);
	gtk_list_store_insert(combo_list, &iter, APPEND_MODE);
	gtk_list_store_set(combo_list, &iter,
	                   COMBO_TEXT, _("Append"), -1);
	gtk_list_store_insert(combo_list, &iter, DELETE_MODE);
	gtk_list_store_set(combo_list, &iter,
	                   COMBO_TEXT, _("Delete"), -1);
	gtk_list_store_insert(combo_list, &iter, PREPARE_MODE);
	gtk_list_store_set(combo_list, &iter,
	                   COMBO_TEXT, _("Prepare"), -1);
	mode_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(combo_list));
	mode_renderer = gtk_cell_renderer_combo_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mode_combo),
	                           mode_renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(mode_combo),
	                              mode_renderer, "text", 0);
	g_object_set(mode_renderer, "width-chars", 8, NULL);
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



	remove_renderer = gtk_cell_renderer_toggle_new();
	g_object_set(remove_renderer, "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(remove_renderer), "toggled", G_CALLBACK (pgui_action_handle_tree_remove), NULL);
	remove_column = gtk_tree_view_column_new_with_attributes("Rm",
	                remove_renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), remove_column);


	g_signal_connect (G_OBJECT (add_file_button), "clicked", G_CALLBACK (pgui_action_open_file_dialog), NULL);


	//GtkTreeIter iter;
	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter,
	                   FILENAME_COLUMN, "",
	                   SCHEMA_COLUMN, "",
	                   TABLE_COLUMN, "",
	                   GEOMETRY_COLUMN, "",
	                   SRID_COLUMN, "",
	                   -1);

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

#ifdef USE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
#endif

	/* Parse command line options and set configuration */
	config = malloc(sizeof(SHPLOADERCONFIG));
	set_config_defaults(config);

	/* Here we override any defaults for the GUI */
	config->createindex = 1;

	conn = malloc(sizeof(SHPCONNECTIONCONFIG));
	memset(conn, 0, sizeof(SHPCONNECTIONCONFIG));

	while ((c = pgis_getopt(argc, argv, "U:p:W:d:h:")) != -1)
	{
		switch (c)
		{
		case 'U':
			conn->username = pgis_optarg;
			break;
		case 'p':
			conn->port = pgis_optarg;
			break;
		case 'W':
			conn->password = pgis_optarg;
			break;
		case 'd':
			conn->database = pgis_optarg;
			break;
		case 'h':
			conn->host = pgis_optarg;
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
