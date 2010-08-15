/**********************************************************************
 * $Id: cu_list.c 5674 2010-06-03 02:04:15Z mleslie $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2010 LISAsoft Pty Ltd
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_list.h"
#include "cu_tester.h"
#include "../structure.h"

/*
typedef struct _GtkTreeIter GtkTreeIter;

struct _GtkTreeIter
{
	gint stamp;
	gpointer user_data;
	gpointer user_data2;
	gpointer user_data3;
}
*/

/* Test functions */
void test_append_file(void);
void test_find_file(void);
void test_traversal(void);
void test_remove_first(void);
void test_remove_middle(void);
void test_remove_last(void);
void test_find_index(void);

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_list_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("GUI Shapefile Loader File List Test", init_list_suite, clean_list_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_append_file()", test_append_file))
	    ||
	    (NULL == CU_add_test(pSuite, "test_find_file()", test_find_file))
	    ||
	    (NULL == CU_add_test(pSuite, "test_traversal()", test_traversal))
	    ||
	    (NULL == CU_add_test(pSuite, "test_remove_first()", test_remove_first))
	    ||
	    (NULL == CU_add_test(pSuite, "test_remove_last()", test_remove_first))
	    ||
	    (NULL == CU_add_test(pSuite, "test_remove_middle()", test_remove_middle))
	    ||
	    (NULL == CU_add_test(pSuite, "test_find_index()", test_find_index))
	)
	{
		CU_cleanup_registry();
		return NULL;
	}
	return pSuite;
}

/*
** The suite initialization function.
** Create any re-used objects.
*/
int init_list_suite(void)
{
	return 0;

}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_list_suite(void)
{
	return 0;
}

void test_append_file(void)
{
	FILENODE *node;
	GtkTreeIter iter;
	init_file_list();

	node = append_file("file1", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);

	node = append_file("file2", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);

	destroy_file_list();
}

void test_find_file(void)
{
	FILENODE *node;
	FILENODE *keeper_node;
	GtkTreeIter iter;
	GtkTreeIter keeper_iter;
	init_file_list();

	node = append_file("file1", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);

	node = append_file("file2", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);
	node = append_file("file3", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);

	node = append_file("file4", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);
	keeper_node = append_file("file5", "schema", "table", "geom_column", "-1", 'c', &keeper_iter);
	CU_ASSERT_PTR_NOT_NULL(node);

	node = append_file("file6", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);
	node = append_file("file7", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);

	node = append_file("file8", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node);

	node = find_file_by_iter(&keeper_iter);
	CU_ASSERT_PTR_NOT_NULL(node);
	CU_ASSERT_PTR_EQUAL(node, keeper_node);

	destroy_file_list();
}

void test_traversal(void)
{
	FILENODE *node[5];
	FILENODE *current_node;
	GtkTreeIter iter;
	int i = 0;
	init_file_list();

	node[0] = append_file("file1", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[0]);
	node[1] = append_file("file2", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[1]);
	node[2] = append_file("file3", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[2]);
	node[3] = append_file("file4", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[3]);
	node[4] = append_file("file5", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[4]);

	current_node = get_next_node(NULL);
	CU_ASSERT_PTR_NOT_NULL(current_node);
	while (current_node != NULL)
	{
		CU_ASSERT_NOT_EQUAL(i, 5);
		CU_ASSERT_PTR_EQUAL(current_node, node[i]);
		current_node = get_next_node(current_node);
		i++;
	}
}

void test_remove_first(void)
{
	FILENODE *node[5];
	FILENODE *current_node;
	GtkTreeIter iter;
	int i = 0;
	init_file_list();

	node[0] = append_file("file1", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[0]);
	node[1] = append_file("file2", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[1]);
	node[2] = append_file("file3", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[2]);
	node[3] = append_file("file4", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[3]);
	node[4] = append_file("file5", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[4]);

	current_node = get_next_node(NULL);
	CU_ASSERT_PTR_NOT_NULL(current_node);
	while (current_node != NULL)
	{
		CU_ASSERT_NOT_EQUAL(i, 5);
		CU_ASSERT_PTR_EQUAL(current_node, node[i]);
		current_node = get_next_node(current_node);
		i++;
	}

	remove_file(node[0]);
	i = 1;
	current_node = get_next_node(NULL);
	CU_ASSERT_PTR_NOT_NULL(current_node);
	while (current_node != NULL)
	{
		CU_ASSERT_NOT_EQUAL(i, 5);
		CU_ASSERT_PTR_EQUAL(current_node, node[i]);
		current_node = get_next_node(current_node);
		i++;
	}
	CU_ASSERT_EQUAL(i, 5);

}

void test_remove_last(void)
{
	FILENODE *node[5];
	FILENODE *current_node;
	GtkTreeIter iter;
	int i = 0;
	init_file_list();

	node[0] = append_file("file1", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[0]);
	node[1] = append_file("file2", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[1]);
	node[2] = append_file("file3", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[2]);
	node[3] = append_file("file4", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[3]);
	node[4] = append_file("file5", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[4]);

	current_node = get_next_node(NULL);
	CU_ASSERT_PTR_NOT_NULL(current_node);
	while (current_node != NULL)
	{
		CU_ASSERT_NOT_EQUAL(i, 5);
		CU_ASSERT_PTR_EQUAL(current_node, node[i]);
		current_node = get_next_node(current_node);
		i++;
	}

	remove_file(node[4]);
	i = 0;
	current_node = get_next_node(NULL);
	CU_ASSERT_PTR_NOT_NULL(current_node);
	while (current_node != NULL)
	{
		CU_ASSERT_NOT_EQUAL(i, 4);
		CU_ASSERT_PTR_EQUAL(current_node, node[i]);
		current_node = get_next_node(current_node);
		i++;
	}
	CU_ASSERT_EQUAL(i, 4);

}

void test_remove_middle(void)
{
	FILENODE *node[5];
	FILENODE *current_node;
	GtkTreeIter iter;
	int i = 0;
	init_file_list();

	node[0] = append_file("file1", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[0]);
	node[1] = append_file("file2", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[1]);
	node[2] = append_file("file3", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[2]);
	node[3] = append_file("file4", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[3]);
	node[4] = append_file("file5", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[4]);

	current_node = get_next_node(NULL);
	CU_ASSERT_PTR_NOT_NULL(current_node);
	while (current_node != NULL)
	{
		CU_ASSERT_NOT_EQUAL(i, 5);
		CU_ASSERT_PTR_EQUAL(current_node, node[i]);
		current_node = get_next_node(current_node);
		i++;
	}

	remove_file(node[3]);
	i = 0;
	current_node = get_next_node(NULL);
	CU_ASSERT_PTR_NOT_NULL(current_node);
	while (current_node != NULL)
	{
		CU_ASSERT_NOT_EQUAL(i, 5);
		CU_ASSERT_PTR_EQUAL(current_node, node[i]);
		current_node = get_next_node(current_node);
		i++;
		if (i == 3)
			i++;
	}
	CU_ASSERT_EQUAL(i, 5);

	destroy_file_list();

}

void test_find_index(void)
{
	FILENODE *node[5];
	FILENODE *current_node;
	GtkTreeIter iter;
	int index[11] = {4, 3, 2, 1, 0, 3, 2, 4, 0, 1, 1};
	int i = 0;
	init_file_list();

	node[0] = append_file("file1", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[0]);
	node[1] = append_file("file2", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[1]);
	node[2] = append_file("file3", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[2]);
	node[3] = append_file("file4", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[3]);
	node[4] = append_file("file5", "schema", "table", "geom_column", "-1", 'c', &iter);
	CU_ASSERT_PTR_NOT_NULL(node[4]);

	for (i = 0; i < 11; i++)
	{
		current_node = find_file_by_index(index[i]);
		CU_ASSERT_PTR_EQUAL(node[index[i]], current_node);
	}

	destroy_file_list();

}
