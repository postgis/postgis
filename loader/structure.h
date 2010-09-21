/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 LISAsoft Pty Ltd <mark.leslie@lisasoft.com>
 *
 * This is free softwark; you can redistribute and/or modify it under 
 * the terms of the GNU General Public Licence.  See the COPYING file.
 *
 * This file contains struct and method definitions used to manage the
 * basic elements stored withing the shp2pgsql-gui.c list item.  These
 * were separated out for ease of testing.
 *
 *********************************************************************/


#ifndef __PGIS_STRUCTURE_H__
#define __PGIS_STRUCTURE_H__

#include <gtk/gtktreemodel.h>

typedef struct file_node
{
	char *filename;
	char *schema;
	char *table;
	char *geom_column;
	char *srid;
	char mode;
	GtkTreeIter *tree_iterator;
	struct file_node *next;
	struct file_node *prev;
} FILENODE;

void init_file_list(void);
void destroy_file_list(void);
FILENODE *append_file_node(void);
FILENODE *append_file(char *filename, char *schema, char *table, 
		    char *geom_column, char *srid, char mode, 
			GtkTreeIter *tree_iterator);
FILENODE *find_file_by_iter(GtkTreeIter *tree_iterator);
FILENODE *find_file_by_index(int index);
void remove_file(FILENODE *remove_node);
void print_file_list_delegate(void (*printer)(const char *fmta, va_list apa), const char *fmt, ...);

/*
 * This is a debugging function, it shouldn't really be used for 
 * general status output.  It takes a function pointer of the same
 * signature as vprintf (hint).
 */
void print_file_list(void (*printer)(const char *fmt, va_list ap));

/*
 * This will return the next node after the current node.
 * If the current is null, it will return the first node.  Returns NULL
 * if the list is uninitialised, empty or if the current node is the last node.
 */
FILENODE *get_next_node(FILENODE *current);

#endif /* __PGIS_STRUCTURE_H__ */
