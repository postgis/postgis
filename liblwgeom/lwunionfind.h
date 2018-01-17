/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.net
 *
 * PostGIS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PostGIS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
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

/* Allocate a UNIONFIND structure of capacity N */
UNIONFIND* UF_create(uint32_t N);

/* Release memory associated with UNIONFIND structure */
void UF_destroy(UNIONFIND* uf);

/* Identify the cluster id associated with specified component id */
uint32_t UF_find(UNIONFIND* uf, uint32_t i);

/* Get the size of the cluster associated with the specified component id */
uint32_t UF_size(UNIONFIND* uf, uint32_t i);

/* Merge the clusters that contain the two specified component ids */
void UF_union(UNIONFIND* uf, uint32_t i, uint32_t j);

/* Return an array of component ids, where components that are in the
 * same cluster are contiguous in the array */
uint32_t* UF_ordered_by_cluster(UNIONFIND* uf);

/* Replace the cluster ids in a UNIONFIND with sequential ids starting at one.
 * If is_in_cluster array is provided, it will be used to skip any indexes
 * that are not in a cluster.
 * */
uint32_t* UF_get_collapsed_cluster_ids(UNIONFIND* uf, const char* is_in_cluster);

#endif
