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
 * Copyright 2020 Raúl Marín
 *
 **********************************************************************/

#include "lwgeom_cache.h"

SHARED_GSERIALIZED *
shared_gserialized_new_nocache(Datum d)
{
	SHARED_GSERIALIZED *s = palloc(sizeof(SHARED_GSERIALIZED));
	s->count = 0;
	s->geom = (GSERIALIZED *)PG_DETOAST_DATUM(d);
	return s;
}

SHARED_GSERIALIZED *
shared_gserialized_new_cached(FunctionCallInfo fcinfo, Datum d)
{
	SHARED_GSERIALIZED *s = MemoryContextAlloc(PostgisCacheContext(fcinfo), sizeof(SHARED_GSERIALIZED));
	MemoryContext old_context = MemoryContextSwitchTo(PostgisCacheContext(fcinfo));
	s->geom = (GSERIALIZED *)PG_DETOAST_DATUM_COPY(d);
	MemoryContextSwitchTo(old_context);
	s->count = 1;
	return s;
}

SHARED_GSERIALIZED *
shared_gserialized_ref(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *ref)
{
	if (MemoryContextContains(PostgisCacheContext(fcinfo), ref))
	{
		ref->count++;
		return ref;
	}
	else
	{
		SHARED_GSERIALIZED *sg = MemoryContextAlloc(PostgisCacheContext(fcinfo), sizeof(SHARED_GSERIALIZED));
		sg->count = 1;
		sg->geom = MemoryContextAlloc(PostgisCacheContext(fcinfo), VARSIZE(ref->geom));
		memcpy(sg->geom, ref->geom, VARSIZE(ref->geom));
		return sg;
	}
}

void
shared_gserialized_unref(FunctionCallInfo fcinfo, SHARED_GSERIALIZED *ref)
{
	if (MemoryContextContains(PostgisCacheContext(fcinfo), ref))
	{
		ref->count--;
		if (!ref->count)
		{
			pfree(ref->geom);
			pfree(ref);
		}
	}
	else
	{
		pfree(ref->geom);
		pfree(ref);
	}
}

bool
shared_gserialized_equal(SHARED_GSERIALIZED *r1, SHARED_GSERIALIZED *r2)
{
	if (r1->geom == r2->geom)
		return true;
	if (VARSIZE(r1->geom) != VARSIZE(r2->geom))
		return false;
	return memcmp(r1->geom, r2->geom, VARSIZE(r1->geom)) == 0;
}

const GSERIALIZED *
shared_gserialized_get(SHARED_GSERIALIZED *s)
{
	return s->geom;
}