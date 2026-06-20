/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef LOADER_ACTIONS_H
#define LOADER_ACTIONS_H 1

#include <stdbool.h>

typedef enum loader_create_mode
{
	LOADER_CREATE_NONE = 0,
	LOADER_CREATE_ALWAYS,
	LOADER_CREATE_IF_NOT_EXISTS
} LoaderCreateMode;

typedef enum loader_transaction_mode
{
	LOADER_TRANSACTION_OFF = 0,
	LOADER_TRANSACTION_ON
} LoaderTransactionMode;

typedef struct loader_plan {
	int drop_table;
	LoaderCreateMode create_table;
	int load_data;
	LoaderCreateMode create_index;
	int add_constraints;
	int vacuum;
	int analyze;
	LoaderTransactionMode transaction;
} LoaderPlan;

typedef struct loader_action_options {
	char mode;
	int mode_set;
	int if_not_exists;
	int drop_table;
	LoaderCreateMode create_table;
	int create_table_set;
	int load_data;
	int load_data_set;
	LoaderCreateMode create_index;
	int create_index_set;
	int add_constraints;
	int vacuum;
	int analyze;
} LoaderActionOptions;

static inline bool
loader_action_set_create_mode(LoaderCreateMode *current_mode, int *is_set, LoaderCreateMode new_mode)
{
	if (*is_set && *current_mode != new_mode)
		return false;

	*current_mode = new_mode;
	*is_set = true;
	return true;
}

#endif /* LOADER_ACTIONS_H */
