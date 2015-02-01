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

#include "liblwgeom.h"
#include "lwunionfind.h"
#include <string.h>

static int cmp_int(const void *a, const void *b);
static int cmp_int_ptr(const void *a, const void *b);

UNIONFIND*
UF_create(size_t N)
{
	size_t i;
	UNIONFIND* uf = lwalloc(sizeof(UNIONFIND));
	uf->N = N;
	uf->num_clusters = N;
	uf->clusters = lwalloc(N * sizeof(int));
	uf->cluster_sizes = lwalloc(N * sizeof(int));

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

size_t
UF_find (UNIONFIND* uf, size_t i)
{
	while (uf->clusters[i] != i)
	{
		uf->clusters[i] = uf->clusters[uf->clusters[i]];
		i = uf->clusters[i];
	}
	return i;
}

void
UF_union(UNIONFIND* uf, size_t i, size_t j)
{
	size_t a = UF_find(uf, i);
	size_t b = UF_find(uf, j);

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

int*
UF_ordered_by_cluster(UNIONFIND* uf)
{
	size_t i;
	int** cluster_id_ptr_by_elem_id = lwalloc(uf->N * sizeof (int*));
	int* ordered_ids = lwalloc(uf->N * sizeof (int));

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
	qsort(cluster_id_ptr_by_elem_id, uf->N, sizeof (int*), &cmp_int_ptr);

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
static int
cmp_int(const void *a, const void *b)
{
	if (*((int*) a) > *((int*) b))
	{
		return 1;
	}
	else if (*((int*) a) < *((int*) b))
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
	int val_cmp = cmp_int(*((int**) a), *((int**) b));
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
