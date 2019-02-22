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


#include "liblwgeom.h"
#include "lwunionfind.h"
#include <string.h>
#include <stdlib.h>

static int cmp_int(const void *a, const void *b);
static int cmp_int_ptr(const void *a, const void *b);

UNIONFIND*
UF_create(uint32_t N)
{
	size_t i;
	UNIONFIND* uf = lwalloc(sizeof(UNIONFIND));
	uf->N = N;
	uf->num_clusters = N;
	uf->clusters = lwalloc(N * sizeof(uint32_t));
	uf->cluster_sizes = lwalloc(N * sizeof(uint32_t));

	for (i = 0; i < N; i++)
	{
		uf->clusters[i] = i;
		uf->cluster_sizes[i] = 1;
	}

	return uf;
}

void
UF_destroy(UNIONFIND* uf)
{
	lwfree(uf->clusters);
	lwfree(uf->cluster_sizes);
	lwfree(uf);
}

uint32_t
UF_find (UNIONFIND* uf, uint32_t i)
{
	uint32_t base = i;
	while (uf->clusters[base] != base) {
		base = uf->clusters[base];
	}

	while (i != base) {
		uint32_t next = uf->clusters[i];
		uf->clusters[i] = base;
		i = next;
	}

	return i;
}

uint32_t
UF_size (UNIONFIND* uf, uint32_t i)
{
    return uf->cluster_sizes[UF_find(uf, i)];
}

void
UF_union(UNIONFIND* uf, uint32_t i, uint32_t j)
{
	uint32_t a = UF_find(uf, i);
	uint32_t b = UF_find(uf, j);

	if (a == b)
	{
		return;
	}

	if (uf->cluster_sizes[a] < uf->cluster_sizes[b] ||
	        (uf->cluster_sizes[a] == uf->cluster_sizes[b] && a > b))
	{
		uf->clusters[a] = uf->clusters[b];
		uf->cluster_sizes[b] += uf->cluster_sizes[a];
		uf->cluster_sizes[a] = 0;
	}
	else
	{
		uf->clusters[b] = uf->clusters[a];
		uf->cluster_sizes[a] += uf->cluster_sizes[b];
		uf->cluster_sizes[b] = 0;
	}

	uf->num_clusters--;
}

uint32_t*
UF_ordered_by_cluster(UNIONFIND* uf)
{
	size_t i;
	uint32_t** cluster_id_ptr_by_elem_id = lwalloc(uf->N * sizeof (uint32_t*));
	uint32_t* ordered_ids = lwalloc(uf->N * sizeof (uint32_t));

	for (i = 0; i < uf->N; i++)
	{
		/* Make sure each value in uf->clusters is pointing to the
		 * root of the cluster.
		 * */
		UF_find(uf, i);
		cluster_id_ptr_by_elem_id[i] = &(uf->clusters[i]);
	}

	/* Sort the array of cluster id pointers, so that pointers to the
	 * same cluster id are grouped together.
	 * */
	qsort(cluster_id_ptr_by_elem_id, uf->N, sizeof (uint32_t*), &cmp_int_ptr);

	/* Recover the input element ids from the cluster id pointers, so
	 * we can return element ids grouped by cluster id.
	 * */
	for (i = 0; i < uf-> N; i++)
	{
		ordered_ids[i] = (cluster_id_ptr_by_elem_id[i] - uf->clusters);
	}

	lwfree(cluster_id_ptr_by_elem_id);
	return ordered_ids;
}

uint32_t*
UF_get_collapsed_cluster_ids(UNIONFIND* uf, const char* is_in_cluster)
{
	uint32_t* ordered_components = UF_ordered_by_cluster(uf);
	uint32_t* new_ids = lwalloc(uf->N * sizeof(uint32_t));
	uint32_t last_old_id, current_new_id, i;
	char encountered_cluster = LW_FALSE;

	current_new_id = 0; last_old_id = 0;
	for (i = 0; i < uf->N; i++)
	{
		uint32_t j = ordered_components[i];
		if (!is_in_cluster || is_in_cluster[j])
		{
			uint32_t current_old_id = UF_find(uf, j);
			if (!encountered_cluster)
			{
				encountered_cluster = LW_TRUE;
				last_old_id = current_old_id;
			}

			if (current_old_id != last_old_id)
				current_new_id++;

			new_ids[j] = current_new_id;
			last_old_id = current_old_id;
		}
	}

	lwfree(ordered_components);

	return new_ids;
}

static int
cmp_int(const void *a, const void *b)
{
	if (*((uint32_t*) a) > *((uint32_t*) b))
	{
		return 1;
	}
	else if (*((uint32_t*) a) < *((uint32_t*) b))
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

static int
cmp_int_ptr(const void *a, const void *b)
{
	int val_cmp = cmp_int(*((uint32_t**) a), *((uint32_t**) b));
	if (val_cmp != 0)
	{
		return val_cmp;
	}
	if (a > b)
	{
		return 1;
	}
	if (a < b)
	{
		return -1;
	}
	return 0;
}
