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
	uint32_t* clusters;
	uint32_t* cluster_sizes;
	uint32_t num_clusters;
	uint32_t N;
} UNIONFIND;

UNIONFIND* UF_create(uint32_t N);
void UF_destroy(UNIONFIND* uf);
uint32_t UF_find(UNIONFIND* uf, uint32_t i);
void UF_union(UNIONFIND* uf, uint32_t i, uint32_t j);
uint32_t* UF_ordered_by_cluster(UNIONFIND* uf);

#endif
