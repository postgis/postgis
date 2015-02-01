/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _LWUNIONFIND
#define _LWUNIONFIND 1

#include "liblwgeom.h"

typedef struct
{
	int* clusters;
	int* cluster_sizes;
	size_t num_clusters;
	size_t N;
} UNIONFIND;

UNIONFIND* UF_create(size_t N);
void UF_destroy(UNIONFIND* uf);
size_t UF_find(UNIONFIND* uf, size_t i);
void UF_union(UNIONFIND* uf, size_t i, size_t j);
int* UF_ordered_by_cluster(UNIONFIND* uf);

#endif
