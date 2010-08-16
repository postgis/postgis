/**********************************************************************
 * $ID$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 LISAsoft Pty Ltd <mark.leslie@lisasoft.com>
 *
 * This is free softwark; you can redistribute and/or modify it under 
 * the terms of the GNU General Public Licence.  See the COPYING file.
 *
 * This file contains functions used to manage the basic elements 
 * stored withing the shp2pgsql-gui.c list item.  These were separated
 * out for ease of testing.
 *
 *********************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "structure.h"

FILENODE *file_list_head = NULL;
FILENODE *file_list_tail = NULL;

void
init_file_list(void)
{
	file_list_head = malloc(sizeof(FILENODE));
	file_list_tail = malloc(sizeof(FILENODE));

	file_list_head->filename = NULL;
	file_list_head->schema = NULL;
	file_list_head->table = NULL;
	file_list_head->geom_column = NULL;
	file_list_head->srid = NULL;
	file_list_head->mode = '\0';
	file_list_head->prev = NULL;
	file_list_head->next = file_list_tail;

	file_list_tail->filename = NULL;
	file_list_tail->schema = NULL;
	file_list_tail->table = NULL;
	file_list_tail->geom_column = NULL;
	file_list_tail->srid = NULL;
	file_list_tail->mode = '\0';
	file_list_tail->prev = file_list_head;
	file_list_tail->next = NULL;
}

void
destroy_file_list(void)
{
	FILENODE *node = get_next_node(NULL);
	while (node != NULL)
	{
		remove_file(node);
		node = get_next_node(NULL);
	}

	if (file_list_head != NULL)
	{
		file_list_head->next = NULL;
		file_list_head->prev = NULL;
		free(file_list_head);
	}

	if (file_list_tail != NULL)
	{
		file_list_tail->next = NULL;
		file_list_tail->prev = NULL;
		free(file_list_tail);
	}
}


FILENODE*
append_file_node(void)
{
	if (file_list_head == NULL || file_list_tail == NULL)
	{
		init_file_list();
	}
	FILENODE *new_node = malloc(sizeof(FILENODE));

	new_node->filename = NULL;
	new_node->schema = NULL;
	new_node->table = NULL;
	new_node->geom_column = NULL;
	new_node->srid = NULL;
	new_node->mode = '\0';
	new_node->tree_iterator = NULL;
	new_node->next = file_list_tail;
	new_node->prev = file_list_tail->prev;
	file_list_tail->prev->next = new_node;
	file_list_tail->prev = new_node;

	return new_node;
}

FILENODE*
append_file(char *filename, char *schema, char *table,
            char *geom_column, char *srid, char mode,
            GtkTreeIter *tree_iterator)
{
	FILENODE *new_file = append_file_node();

	/*
	 * I have no reason to believe that freeing char*'s in the struct will
	 * not harm the ui and vice versa, so I'm avoiding the problem by dup'ing.
	 */
	new_file->filename = strdup(filename);
	new_file->schema = strdup(schema);
	new_file->table = strdup(table);
	new_file->geom_column = strdup(geom_column);
	new_file->srid = strdup(srid);
	new_file->mode = mode;
	new_file->tree_iterator = tree_iterator;

	return new_file;
}

FILENODE*
find_file_by_iter(GtkTreeIter *tree_iterator)
{

	FILENODE *current_node;

	if (file_list_head == NULL)
	{
		return NULL;
	}
	current_node = file_list_head->next;
	while (current_node != NULL && current_node != file_list_tail)
	{
		if (current_node->tree_iterator == tree_iterator)
		{
			return current_node;
		}
		current_node = current_node->next;
	}
	return NULL;
}

FILENODE*
find_file_by_index(int index)
{
	FILENODE *current_node;
	int i = 0;

	if (file_list_head == NULL)
	{
		return NULL;
	}
	current_node = file_list_head->next;
	while (current_node != NULL && current_node != file_list_tail)
	{
		if (i == index)
		{
			return current_node;
		}

		current_node = current_node->next;
		i++;
	}
	return NULL;
}

void
remove_file(FILENODE *remove_node)
{
	if (remove_node == NULL
	        || remove_node->next == NULL
	        || remove_node->prev == NULL)
	{
		return;
	}
	remove_node->next->prev = remove_node->prev;
	remove_node->prev->next = remove_node->next;
	remove_node->next = NULL;
	remove_node->prev = NULL;
	if (remove_node->filename == NULL)
	{
		free(remove_node->filename);
		remove_node->filename = NULL;
	}
	if (remove_node->schema == NULL)
	{
		free(remove_node->schema);
		remove_node->schema = NULL;
	}
	if (remove_node->table == NULL)
	{
		free(remove_node->table);
		remove_node->table = NULL;
	}
	if (remove_node->geom_column == NULL)
	{
		free(remove_node->geom_column);
		remove_node->geom_column = NULL;
	}
	if (remove_node->srid == NULL)
	{
		free(remove_node->srid);
		remove_node->srid = NULL;
	}
	free(remove_node);
}

FILENODE*
get_next_node(FILENODE *current)
{
	if (file_list_head == NULL)
		return NULL;
	if (current == NULL)
		current = file_list_head;
	return (current->next != file_list_tail) ? current->next : NULL;

}

void
print_file_list_delegate(void (*printer)(const char *fmta, va_list apa), const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	(*printer)(fmt, ap);

	va_end(ap);
	return;

}

void
print_file_list(void (*printer)(const char *fmt, va_list ap))
{
	FILENODE *current_node;
	int i = 0;

	print_file_list_delegate(printer, "File List:\n");
	print_file_list_delegate(printer, "Head %p <-- %p --> %p\n", file_list_head->prev, file_list_head, file_list_head->next);

	current_node = get_next_node(NULL);
	while (current_node != NULL)
	{
		print_file_list_delegate(printer, "  Node %d: %s\n", i++, current_node->filename);
		print_file_list_delegate(printer, "    %p <-- %p --> %p\n", current_node->prev, current_node, current_node->next);
		current_node = get_next_node(current_node);
	}

	print_file_list_delegate(printer, "Tail %p <-- %p --> %p\n", file_list_tail->prev, file_list_tail, file_list_tail->next);
}
