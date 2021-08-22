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
 * Copyright (c) 2019, Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/

#ifndef _LWGEOM_ACCUM_H
#define _LWGEOM_ACCUM_H 1

/**
** To pass the internal state of our collection between the
** transfn and finalfn we need to wrap it into a custom type first,
** the CollectionBuildState type in our case.  The extra "data" member
** can optionally be used to pass additional constant
** arguments to a finalizer function.
*/
#define CollectionBuildStateDataSize 2
typedef struct CollectionBuildState
{
	List *geoms;  /* collected geometries */
	Datum data[CollectionBuildStateDataSize];
	Oid geomOid;
	float8 gridSize;
} CollectionBuildState;


#endif /* _LWGEOM_ACCUM_H */
