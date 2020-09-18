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
 * Copyright 2011-2017 Arrival 3D, Regina Obe
 *
 **********************************************************************/

/**
* @file X3D output routines.
*
**********************************************************************/

#include "lwout_x3d.h"

/*
 * VERSION X3D 3.0.2 http://www.web3d.org/specifications/x3d-3.0.dtd
 */
/* takes a GEOMETRY and returns a X3D representation */
lwvarlena_t *
lwgeom_to_x3d3(const LWGEOM *geom, int precision, int opts, const char *defid)
{
	stringbuffer_t *sb;
	int rv;

	/* Empty varlena for empties */
	if( lwgeom_is_empty(geom) )
	{
		lwvarlena_t *v = lwalloc(LWVARHDRSZ);
		LWSIZE_SET(v->size, LWVARHDRSZ);
		return v;
	}

	sb = stringbuffer_create();
	rv = lwgeom_to_x3d3_sb(geom, precision, opts, defid, sb);

	if ( rv == LW_FAILURE )
	{
		stringbuffer_destroy(sb);
		return NULL;
	}

	lwvarlena_t *v = stringbuffer_getvarlenacopy(sb);
	stringbuffer_destroy(sb);

	return v;
}
/* takes a GEOMETRY and appends to string buffer the x3d output */
static int
lwgeom_to_x3d3_sb(const LWGEOM *geom, int precision, int opts, const char *defid, stringbuffer_t *sb)
{
	int type = geom->type;

	switch (type)
	{
	case POINTTYPE:
		return asx3d3_point_sb((LWPOINT *)geom, precision, opts, defid, sb);

	case LINETYPE:
		return asx3d3_line_sb((LWLINE *)geom, precision, opts, defid, sb);

	case POLYGONTYPE:
	{
		/** We might change this later, but putting a polygon in an indexed face set
		* seems like the simplest way to go so treat just like a mulitpolygon
		*/
		LWCOLLECTION *tmp = (LWCOLLECTION*)lwgeom_as_multi(geom);
		asx3d3_multi_sb(tmp, precision, opts, defid, sb);
		lwcollection_free(tmp);
		return LW_SUCCESS;
	}

	case TRIANGLETYPE:
		return asx3d3_triangle_sb((LWTRIANGLE *)geom, precision, opts, defid, sb);

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		return asx3d3_multi_sb((LWCOLLECTION *)geom, precision, opts, defid, sb);

	case POLYHEDRALSURFACETYPE:
		return asx3d3_psurface_sb((LWPSURFACE *)geom, precision, opts, defid, sb);

	case TINTYPE:
		return asx3d3_tin_sb((LWTIN *)geom, precision, opts, defid, sb);

	case COLLECTIONTYPE:
		return asx3d3_collection_sb((LWCOLLECTION *)geom, precision, opts, defid, sb);

	default:
		lwerror("lwgeom_to_x3d3: '%s' geometry type not supported", lwtype_name(type));
		return LW_FAILURE;
	}
}

static int
asx3d3_point_sb(const LWPOINT *point,
		int precision,
		int opts,
		__attribute__((__unused__)) const char *defid,
		stringbuffer_t *sb)
{
	/** for point we just output the coordinates **/
	return ptarray_to_x3d3_sb(point->point, precision, opts, 0, sb);
}

static int
asx3d3_line_coords_sb(const LWLINE *line, int precision, int opts, stringbuffer_t *sb)
{
	return ptarray_to_x3d3_sb(line->points, precision, opts, lwline_is_closed(line), sb);
}

/* Calculate the coordIndex property of the IndexedLineSet for the multilinestring
and add to string buffer */
static int
asx3d3_mline_coordindex_sb(const LWMLINE *mgeom, stringbuffer_t *sb)
{
	LWLINE *geom;
	uint32_t i, j, k, si;
	POINTARRAY *pa;
	uint32_t np;

	j = 0;
	for (i=0; i < mgeom->ngeoms; i++)
	{
		geom = (LWLINE *) mgeom->geoms[i];
		pa = geom->points;
		np = pa->npoints;
		si = j;  /* start index of first point of linestring */
		for (k=0; k < np ; k++)
		{
			if (k)
			{
				stringbuffer_aprintf(sb, " ");
			}
			/** if the linestring is closed, we put the start point index
			*   for the last vertex to denote use first point
			*    and don't increment the index **/
			if (!lwline_is_closed(geom) || k < (np -1) )
			{
				stringbuffer_aprintf(sb, "%u", j);
				j += 1;
			}
			else
			{
				stringbuffer_aprintf(sb,"%u", si);
			}
		}
		if (i < (mgeom->ngeoms - 1) )
		{
			stringbuffer_aprintf(sb, " -1 "); /* separator for each linestring */
		}
	}
	return LW_SUCCESS;
}

/* Calculate the coordIndex property of the IndexedLineSet for a multipolygon
    This is not ideal -- would be really nice to just share this function with psurf,
    but I'm not smart enough to do that yet*/
static int
asx3d3_mpoly_coordindex_sb(const LWMPOLY *psur, stringbuffer_t *sb)
{
	LWPOLY *patch;
	uint32_t i, j, k, l;
	uint32_t np;
	j = 0;
	for (i=0; i<psur->ngeoms; i++)
	{
		patch = (LWPOLY *) psur->geoms[i];
		for (l=0; l < patch->nrings; l++)
		{
			np = patch->rings[l]->npoints - 1;
			for (k=0; k < np ; k++)
			{
				if (k)
				{
					stringbuffer_aprintf(sb,  " ");
				}
				stringbuffer_aprintf(sb,  "%d", (j + k));
			}
			j += k;
			if (l < (patch->nrings - 1) )
			{
				/** @todo TODO: Decide the best way to render holes
				*  Evidently according to my X3D expert the X3D consortium doesn't really
				*  support holes and it's an issue of argument among many that feel it should. He thinks CAD x3d extensions to spec might.
				*  What he has done and others developing X3D exports to simulate a hole is to cut around it.
				*  So if you have a donut, you would cut it into half and have 2 solid polygons.  Not really sure the best way to handle this.
				*  For now will leave it as polygons stacked on top of each other -- which is what we are doing here and perhaps an option
				*  to color differently.  It's not ideal but the alternative sounds complicated.
				**/
				stringbuffer_aprintf(sb,  " -1 "); /* separator for each inner ring. Ideally we should probably triangulate and cut around as others do */
			}
		}
		if (i < (psur->ngeoms - 1) )
		{
			stringbuffer_aprintf(sb,  " -1 "); /* separator for each subgeom */
		}
	}
	return LW_SUCCESS;
}

/** Return the linestring as an X3D LineSet */
static int
asx3d3_line_sb(const LWLINE *line,
	       int precision,
	       int opts,
	       __attribute__((__unused__)) const char *defid,
	       stringbuffer_t *sb)
{

	/* int dimension=2; */
	POINTARRAY *pa;


	/* if (FLAGS_GET_Z(line->flags)) dimension = 3; */

	pa = line->points;
	stringbuffer_aprintf(sb, "<LineSet %s vertexCount='%d'>", defid, pa->npoints);

	if ( X3D_USE_GEOCOORDS(opts) ) stringbuffer_aprintf(sb, "<GeoCoordinate geoSystem='\"GD\" \"WE\" \"%s\"' point='", ( (opts & LW_X3D_FLIP_XY) ? "latitude_first" : "longitude_first") );
	else
		stringbuffer_aprintf(sb, "<Coordinate point='");

	ptarray_to_x3d3_sb(line->points, precision, opts, lwline_is_closed((LWLINE *) line), sb);


	stringbuffer_aprintf(sb, "' />");

	return stringbuffer_aprintf(sb, "</LineSet>");
}

/** Compute the X3D coordinates of the polygon and add to string buffer **/
static int
asx3d3_poly_sb(const LWPOLY *poly,
	       int precision,
	       int opts,
	       __attribute__((__unused__)) int is_patch,
	       __attribute__((__unused__)) const char *defid,
	       stringbuffer_t *sb)
{
	uint32_t i;
	for (i=0; i<poly->nrings; i++)
	{
		if (i) stringbuffer_aprintf(sb, " "); /* inner ring points start */
		ptarray_to_x3d3_sb(poly->rings[i], precision, opts, 1, sb);
	}
	return LW_SUCCESS;
}

static int
asx3d3_triangle_sb(const LWTRIANGLE *triangle,
		   int precision,
		   int opts,
		   __attribute__((__unused__)) const char *defid,
		   stringbuffer_t *sb)
{
	return  ptarray_to_x3d3_sb(triangle->points, precision, opts, 1, sb);
}


/*
 * Don't call this with single-geoms inspected!
 */
static int
asx3d3_multi_sb(const LWCOLLECTION *col, int precision, int opts, const char *defid, stringbuffer_t *sb)
{
	char *x3dtype;
	uint32_t i;
	int dimension=2;

	if (FLAGS_GET_Z(col->flags)) dimension = 3;
	LWGEOM *subgeom;
	x3dtype="";


	switch (col->type)
	{
        case MULTIPOINTTYPE:
            x3dtype = "PointSet";
            if ( dimension == 2 ){ /** Use Polypoint2D instead **/
                x3dtype = "Polypoint2D";
                stringbuffer_aprintf(sb, "<%s %s point='", x3dtype, defid);
            }
            else {
                stringbuffer_aprintf(sb, "<%s %s>", x3dtype, defid);
            }
            break;
        case MULTILINETYPE:
            x3dtype = "IndexedLineSet";
            stringbuffer_aprintf(sb, "<%s %s coordIndex='", x3dtype, defid);
            asx3d3_mline_coordindex_sb((const LWMLINE *)col, sb);
            stringbuffer_aprintf(sb, "'>");
            break;
        case MULTIPOLYGONTYPE:
            x3dtype = "IndexedFaceSet";
            stringbuffer_aprintf(sb, "<%s %s convex='false' coordIndex='", x3dtype, defid);
            asx3d3_mpoly_coordindex_sb((const LWMPOLY *)col, sb);
            stringbuffer_aprintf(sb, "'>");
            break;
        default:
            lwerror("asx3d3_multi_buf: '%s' geometry type not supported", lwtype_name(col->type));
            return 0;
    }
    if (dimension == 3){
		if ( X3D_USE_GEOCOORDS(opts) )
			stringbuffer_aprintf(sb, "<GeoCoordinate geoSystem='\"GD\" \"WE\" \"%s\"' point='", ((opts & LW_X3D_FLIP_XY) ? "latitude_first" : "longitude_first") );
		else
        	stringbuffer_aprintf(sb, "<Coordinate point='");
    }

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			asx3d3_point_sb((LWPOINT *)subgeom, precision, opts, defid, sb);
			stringbuffer_aprintf(sb, " ");
		}
		else if (subgeom->type == LINETYPE)
		{
			asx3d3_line_coords_sb((LWLINE*)subgeom, precision, opts, sb);
			stringbuffer_aprintf(sb, " ");
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			asx3d3_poly_sb((LWPOLY *)subgeom, precision, opts, 0, defid, sb);
			stringbuffer_aprintf(sb, " ");
		}
	}

	/* Close outmost tag */
	if (dimension == 3){
		stringbuffer_aprintf(sb, "' /></%s>", x3dtype);
	}
	else { stringbuffer_aprintf(sb, "' />"); }
	return LW_SUCCESS;

}

/*
 * Don't call this with single-geoms inspected!
 */
static int
asx3d3_psurface_sb(const LWPSURFACE *psur, int precision, int opts, const char *defid, stringbuffer_t *sb)
{
	uint32_t i;
	uint32_t j;
	uint32_t k;
	uint32_t np;
	LWPOLY *patch;

	/* Open outmost tag */
	stringbuffer_aprintf(sb, "<IndexedFaceSet convex='false' %s coordIndex='",defid);

	j = 0;
	for (i=0; i<psur->ngeoms; i++)
	{
		patch = (LWPOLY *) psur->geoms[i];
		np = patch->rings[0]->npoints - 1;
		for (k=0; k < np ; k++)
		{
			if (k)
			{
				stringbuffer_aprintf(sb, " ");
			}
			stringbuffer_aprintf(sb,"%d", (j + k));
		}
		if (i < (psur->ngeoms - 1) )
		{
			stringbuffer_aprintf(sb, " -1 "); /* separator for each subgeom */
		}
		j += k;
	}

	if ( X3D_USE_GEOCOORDS(opts) )
		stringbuffer_aprintf(sb, "'><GeoCoordinate geoSystem='\"GD\" \"WE\" \"%s\"' point='",
			( (opts & LW_X3D_FLIP_XY) ? "latitude_first" : "longitude_first") );
	else stringbuffer_aprintf(sb, "'><Coordinate point='");

	for (i=0; i<psur->ngeoms; i++)
	{
		asx3d3_poly_sb(psur->geoms[i], precision, opts, 1, defid, sb);
		if (i < (psur->ngeoms - 1) )
		{
			stringbuffer_aprintf(sb, " "); /* only add a trailing space if its not the last polygon in the set */
		}
	}

	/* Close outmost tag */
	return stringbuffer_aprintf(sb, "' /></IndexedFaceSet>");
}

/*
 * Computes X3D representation of TIN (as IndexedTriangleSet and adds to string buffer)
 */
static int
asx3d3_tin_sb(const LWTIN *tin, int precision, int opts, const char *defid, stringbuffer_t *sb)
{
	uint32_t i;
	uint32_t k;
	/* int dimension=2; */

	stringbuffer_aprintf(sb,"<IndexedTriangleSet %s index='",defid);
	k = 0;
	/** Fill in triangle index **/
	for (i=0; i<tin->ngeoms; i++)
	{
		stringbuffer_aprintf(sb, "%d %d %d", k, (k+1), (k+2));
		if (i < (tin->ngeoms - 1) )
		{
			stringbuffer_aprintf(sb, " ");
		}
		k += 3;
	}

	if ( X3D_USE_GEOCOORDS(opts) ) stringbuffer_aprintf(sb, "'><GeoCoordinate geoSystem='\"GD\" \"WE\" \"%s\"' point='", ( (opts & LW_X3D_FLIP_XY) ? "latitude_first" : "longitude_first") );
	else stringbuffer_aprintf(sb, "'><Coordinate point='");

	for (i=0; i<tin->ngeoms; i++)
	{
		asx3d3_triangle_sb(tin->geoms[i], precision, opts, defid, sb);
		if (i < (tin->ngeoms - 1) )
		{
			stringbuffer_aprintf(sb, " ");
		}
	}

	/* Close outmost tag */

	return stringbuffer_aprintf(sb, "'/></IndexedTriangleSet>");
}

static int
asx3d3_collection_sb(const LWCOLLECTION *col, int precision, int opts, const char *defid, stringbuffer_t *sb)
{
	uint32_t i;
	LWGEOM *subgeom;

	/* Open outmost tag */
	/** @TODO: if collection should be grouped, we'll wrap in a group tag.  Still needs cleanup
	 * like the shapes should really be in a transform **/
#ifdef PGIS_X3D_OUTERMOST_TAGS
	stringbuffer_aprintf(sb, "<%sGroup>", defid);
#endif

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		stringbuffer_aprintf(sb, "<Shape%s>", defid);
		if ( subgeom->type == POINTTYPE )
		{
			asx3d3_point_sb((LWPOINT *)subgeom, precision, opts, defid, sb);
		}
		else if ( subgeom->type == LINETYPE )
		{
			asx3d3_line_sb((LWLINE *)subgeom, precision, opts, defid, sb);
		}
		else if ( subgeom->type == POLYGONTYPE )
		{
			asx3d3_poly_sb((LWPOLY *)subgeom, precision, opts, 0, defid, sb);
		}
		else if ( subgeom->type == TINTYPE )
		{
			asx3d3_tin_sb((LWTIN *)subgeom, precision, opts, defid, sb);
		}
		else if ( subgeom->type == POLYHEDRALSURFACETYPE )
		{
			asx3d3_psurface_sb((LWPSURFACE *)subgeom, precision, opts, defid, sb);
		}
		else if ( lwgeom_is_collection(subgeom) )
		{
			if ( subgeom->type == COLLECTIONTYPE )
				asx3d3_collection_sb((LWCOLLECTION *)subgeom, precision, opts, defid, sb);
			else
				asx3d3_multi_sb((LWCOLLECTION *)subgeom, precision, opts, defid, sb);
		}
		else
			lwerror("asx3d3_collection_buf: unknown geometry type");

		stringbuffer_aprintf(sb, "</Shape>");
	}

	/* Close outmost tag */
#ifdef PGIS_X3D_OUTERMOST_TAGS
	stringbuffer_aprintf(sb,  "</%sGroup>", defid);
#endif

	return LW_SUCCESS;
}

/** In X3D3, coordinates are separated by a space separator
 */
static int
ptarray_to_x3d3_sb(POINTARRAY *pa, int precision, int opts, int is_closed, stringbuffer_t *sb )
{
	uint32_t i;
	char x[OUT_DOUBLE_BUFFER_SIZE];
	char y[OUT_DOUBLE_BUFFER_SIZE];
	char z[OUT_DOUBLE_BUFFER_SIZE];

	if ( ! FLAGS_GET_Z(pa->flags) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			/** Only output the point if it is not the last point of a closed object or it is a non-closed type **/
			if ( !is_closed || i < (pa->npoints - 1) )
			{
				POINT2D pt;
				getPoint2d_p(pa, i, &pt);

				lwprint_double(pt.x, precision, x);
				lwprint_double(pt.y, precision, y);

				if ( i ) stringbuffer_append_len(sb," ",1);

				if ( ( opts & LW_X3D_FLIP_XY) )
					stringbuffer_aprintf(sb, "%s %s", y, x);
				else
					stringbuffer_aprintf(sb, "%s %s", x, y);
			}
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++)
		{
			/** Only output the point if it is not the last point of a closed object or it is a non-closed type **/
			if ( !is_closed || i < (pa->npoints - 1) )
			{
				POINT4D pt;
				getPoint4d_p(pa, i, &pt);

				lwprint_double(pt.x, precision, x);
				lwprint_double(pt.y, precision, y);
				lwprint_double(pt.z, precision, z);

				if ( i ) stringbuffer_append_len(sb," ",1);

				if ( ( opts & LW_X3D_FLIP_XY) )
					stringbuffer_aprintf(sb, "%s %s %s", y, x, z);
				else
					stringbuffer_aprintf(sb, "%s %s %s", x, y, z);
			}
		}
	}

	return LW_SUCCESS;
}
