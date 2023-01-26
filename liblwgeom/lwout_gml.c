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
 * Copyright 2011 Sandro Santilli <strk@kbt.io>
 * Copyright 2010-2012 Oslandia
 * Copyright 2001-2003 Refractions Research Inc.
 *
 **********************************************************************/


/**
* @file GML output routines.
*
**********************************************************************/


#include <string.h>
#include "liblwgeom_internal.h"
#include "liblwgeom.h"
#include "stringbuffer.h"

typedef struct
{
	const char *srs;
	int precision;
	int opts;
	int is_patch;
	const char *prefix;
	const char *id;
} GML_Options;


static void
asgml2_ptarray(stringbuffer_t* sb, const POINTARRAY *pa, const GML_Options* opts)
{
	uint32_t i;
	if ( ! FLAGS_GET_Z(pa->flags) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			const POINT2D *pt = getPoint2d_cp(pa, i);
			if (i) stringbuffer_append_char(sb, ' ');
			stringbuffer_append_double(sb, pt->x, opts->precision);
			stringbuffer_append_char(sb, ',');
			stringbuffer_append_double(sb, pt->y, opts->precision);
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++)
		{
			const POINT3D *pt = getPoint3d_cp(pa, i);
			if (i) stringbuffer_append_char(sb, ' ');
			stringbuffer_append_double(sb, pt->x, opts->precision);
			stringbuffer_append_char(sb, ',');
			stringbuffer_append_double(sb, pt->y, opts->precision);
			stringbuffer_append_char(sb, ',');
			stringbuffer_append_double(sb, pt->z, opts->precision);
		}
	}
}


static void
asgml2_gbox(stringbuffer_t* sb, const GBOX *bbox, const GML_Options* opts)
{
	if (!bbox)
	{
		stringbuffer_aprintf(sb, "<%sBox", opts->prefix);
		if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
		stringbuffer_append(sb, "/>");
		return;
	}
	else
	{
		POINT4D pt = { bbox->xmin, bbox->ymin, bbox->zmin, 0.0 };
		POINTARRAY *pa = ptarray_construct_empty(FLAGS_GET_Z(bbox->flags), 0, 2);
		ptarray_append_point(pa, &pt, LW_TRUE);
		pt.x = bbox->xmax; pt.y = bbox->ymax; pt.z = bbox->zmax;
		ptarray_append_point(pa, &pt, LW_TRUE);

		if (opts->srs) stringbuffer_aprintf(sb, "<%sBox srsName=\"%s\">", opts->prefix, opts->srs);
		else     stringbuffer_aprintf(sb, "<%sBox>", opts->prefix);

		stringbuffer_aprintf(sb, "<%scoordinates>", opts->prefix);
		asgml2_ptarray(sb, pa, opts);
		stringbuffer_aprintf(sb, "</%scoordinates>", opts->prefix);
		stringbuffer_aprintf(sb, "</%sBox>", opts->prefix);

		ptarray_free(pa);
	}
}

static void
asgml3_ptarray(stringbuffer_t* sb, const POINTARRAY *pa, const GML_Options* opts)
{
	uint32_t i;
	if ( ! FLAGS_GET_Z(pa->flags) )
	{
		for (i=0; i<pa->npoints; i++)
		{
			const POINT2D *pt = getPoint2d_cp(pa, i);
			if (i) stringbuffer_append_char(sb, ' ');
			if (IS_DEGREE(opts->opts))
			{
				stringbuffer_append_double(sb, pt->y, opts->precision);
				stringbuffer_append_char(sb, ' ');
				stringbuffer_append_double(sb, pt->x, opts->precision);
			}
			else
			{
				stringbuffer_append_double(sb, pt->x, opts->precision);
				stringbuffer_append_char(sb, ' ');
				stringbuffer_append_double(sb, pt->y, opts->precision);
			}
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++)
		{
			const POINT3D *pt = getPoint3d_cp(pa, i);
			if (i) stringbuffer_append_char(sb, ' ');
			if (IS_DEGREE(opts->opts))
			{
				stringbuffer_append_double(sb, pt->y, opts->precision);
				stringbuffer_append_char(sb, ' ');
				stringbuffer_append_double(sb, pt->x, opts->precision);
				stringbuffer_append_char(sb, ' ');
				stringbuffer_append_double(sb, pt->z, opts->precision);
			}
			else
			{
				stringbuffer_append_double(sb, pt->x, opts->precision);
				stringbuffer_append_char(sb, ' ');
				stringbuffer_append_double(sb, pt->y, opts->precision);
				stringbuffer_append_char(sb, ' ');
				stringbuffer_append_double(sb, pt->z, opts->precision);
			}
		}
	}
}


static void
asgml3_gbox(stringbuffer_t* sb, const GBOX *bbox, const GML_Options* opts)
{
	if (!bbox)
	{
		stringbuffer_aprintf(sb, "<%sEnvelope", opts->prefix);
		if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
		stringbuffer_append(sb, "/>");
		return;
	}
	else
	{
		int dimension = FLAGS_GET_Z(bbox->flags) ? 3 : 2;

		POINTARRAY *pa = ptarray_construct_empty(FLAGS_GET_Z(bbox->flags), 0, 1);
		POINT4D pt = { bbox->xmin, bbox->ymin, bbox->zmin, 0.0 };
		ptarray_append_point(pa, &pt, LW_TRUE);

		stringbuffer_aprintf(sb, "<%sEnvelope", opts->prefix);
		if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
		if (IS_DIMS(opts->opts)) stringbuffer_aprintf(sb, " srsDimension=\"%d\"", dimension);
		stringbuffer_append(sb, ">");

		stringbuffer_aprintf(sb, "<%slowerCorner>", opts->prefix);
		asgml3_ptarray(sb, pa, opts);
		stringbuffer_aprintf(sb, "</%slowerCorner>", opts->prefix);

		pt.x = bbox->xmax; pt.y = bbox->ymax; pt.z =bbox->zmax;
		ptarray_remove_point(pa, 0);
		ptarray_append_point(pa, &pt, LW_TRUE);

		stringbuffer_aprintf(sb, "<%supperCorner>", opts->prefix);
		asgml3_ptarray(sb, pa, opts);
		stringbuffer_aprintf(sb, "</%supperCorner>", opts->prefix);

		stringbuffer_aprintf(sb, "</%sEnvelope>", opts->prefix);
		ptarray_free(pa);
	}
}

static void
asgml2_point(stringbuffer_t* sb, const LWPOINT *point, const GML_Options* opts)
{

	stringbuffer_aprintf(sb, "<%sPoint", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (lwpoint_is_empty(point))
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");
	stringbuffer_aprintf(sb, "<%scoordinates>", opts->prefix);
	asgml2_ptarray(sb, point->point, opts);
	stringbuffer_aprintf(sb, "</%scoordinates>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sPoint>", opts->prefix);
}

static void
asgml2_line(stringbuffer_t* sb, const LWLINE *line, const GML_Options* opts)
{
	stringbuffer_aprintf(sb, "<%sLineString", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);

	if (lwline_is_empty(line))
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");

	stringbuffer_aprintf(sb, "<%scoordinates>", opts->prefix);
	asgml2_ptarray(sb, line->points, opts);
	stringbuffer_aprintf(sb, "</%scoordinates>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sLineString>", opts->prefix);
}

static void
asgml2_poly(stringbuffer_t* sb, const LWPOLY *poly, const GML_Options* opts)
{
	uint32_t i;

	stringbuffer_aprintf(sb, "<%sPolygon", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (lwpoly_is_empty(poly))
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");
	stringbuffer_aprintf(sb, "<%souterBoundaryIs>", opts->prefix);
	stringbuffer_aprintf(sb, "<%sLinearRing>", opts->prefix);
	stringbuffer_aprintf(sb, "<%scoordinates>", opts->prefix);
	asgml2_ptarray(sb, poly->rings[0], opts);
	stringbuffer_aprintf(sb, "</%scoordinates>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sLinearRing>", opts->prefix);
	stringbuffer_aprintf(sb, "</%souterBoundaryIs>", opts->prefix);
	for (i=1; i<poly->nrings; i++)
	{
		stringbuffer_aprintf(sb, "<%sinnerBoundaryIs>", opts->prefix);
		stringbuffer_aprintf(sb, "<%sLinearRing>", opts->prefix);
		stringbuffer_aprintf(sb, "<%scoordinates>", opts->prefix);
		asgml2_ptarray(sb, poly->rings[i], opts);
		stringbuffer_aprintf(sb, "</%scoordinates>", opts->prefix);
		stringbuffer_aprintf(sb, "</%sLinearRing>", opts->prefix);
		stringbuffer_aprintf(sb, "</%sinnerBoundaryIs>", opts->prefix);
	}
	stringbuffer_aprintf(sb, "</%sPolygon>", opts->prefix);
}

static void
asgml2_multi(stringbuffer_t* sb, const LWCOLLECTION *col, const GML_Options* opts)
{
	uint32_t i;
	const char* gmltype = "";
	int type = col->type;

	/* Subgeoms don't get an SRS */
	GML_Options subopts = *opts;
	subopts.srs = 0;

	if 	    (type == MULTIPOINTTYPE)   gmltype = "MultiPoint";
	else if (type == MULTILINETYPE)	   gmltype = "MultiLineString";
	else if (type == MULTIPOLYGONTYPE) gmltype = "MultiPolygon";

	/* Open outmost tag */
	stringbuffer_aprintf(sb, "<%s%s", opts->prefix, gmltype);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);

	if (!col->ngeoms)
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");

	for (i=0; i<col->ngeoms; i++)
	{
		LWGEOM* subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			stringbuffer_aprintf(sb, "<%spointMember>", opts->prefix);
			asgml2_point(sb, (LWPOINT*)subgeom, &subopts);
			stringbuffer_aprintf(sb, "</%spointMember>", opts->prefix);
		}
		else if (subgeom->type == LINETYPE)
		{
			stringbuffer_aprintf(sb, "<%slineStringMember>", opts->prefix);
			asgml2_line(sb, (LWLINE*)subgeom, &subopts);
			stringbuffer_aprintf(sb, "</%slineStringMember>", opts->prefix);
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			stringbuffer_aprintf(sb, "<%spolygonMember>", opts->prefix);
			asgml2_poly(sb, (LWPOLY*)subgeom, &subopts);
			stringbuffer_aprintf(sb, "</%spolygonMember>", opts->prefix);
		}
	}

	/* Close outmost tag */
	stringbuffer_aprintf(sb, "</%s%s>", opts->prefix, gmltype);
}

static void
asgml2_collection(stringbuffer_t* sb, const LWCOLLECTION *col, const GML_Options* opts)
{
	uint32_t i;
	LWGEOM *subgeom;

	/* Subgeoms don't get an SRS */
	GML_Options subopts = *opts;
	subopts.srs = 0;

	/* Open outmost tag */
	stringbuffer_aprintf(sb, "<%sMultiGeometry", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);

	if (!col->ngeoms)
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		stringbuffer_aprintf(sb, "<%sgeometryMember>", opts->prefix);
		switch (subgeom->type)
		{
			case POINTTYPE:
				asgml2_point(sb, (LWPOINT*)subgeom, &subopts);
				break;
			case LINETYPE:
				asgml2_line(sb, (LWLINE*)subgeom, &subopts);;
				break;
			case POLYGONTYPE:
				asgml2_poly(sb, (LWPOLY*)subgeom, &subopts);
				break;
			case MULTIPOINTTYPE:
			case MULTILINETYPE:
			case MULTIPOLYGONTYPE:
				asgml2_multi(sb, (LWCOLLECTION*)subgeom, &subopts);
				break;
			case COLLECTIONTYPE:
				asgml2_collection(sb, (LWCOLLECTION*)subgeom, &subopts);
				break;
		}
		stringbuffer_aprintf(sb, "</%sgeometryMember>", opts->prefix);
	}
	stringbuffer_aprintf(sb, "</%sMultiGeometry>", opts->prefix);
}



static void
asgml3_point(stringbuffer_t* sb, const LWPOINT *point, const GML_Options* opts)
{
	int dimension = FLAGS_GET_Z(point->flags) ? 3 : 2;

	stringbuffer_aprintf(sb, "<%sPoint", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)  stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);
	if (lwpoint_is_empty(point))
	{
		stringbuffer_append(sb, "/>");
		return;
	}

	stringbuffer_append(sb, ">");
	if (IS_DIMS(opts->opts))
		stringbuffer_aprintf(sb, "<%spos srsDimension=\"%d\">", opts->prefix, dimension);
	else
		stringbuffer_aprintf(sb, "<%spos>", opts->prefix);
	asgml3_ptarray(sb, point->point, opts);
	stringbuffer_aprintf(sb, "</%spos></%sPoint>", opts->prefix, opts->prefix);
}

static void
asgml3_line(stringbuffer_t* sb, const LWLINE *line, const GML_Options* opts)
{
	int dimension = FLAGS_GET_Z(line->flags) ? 3 : 2;
	int shortline = (opts->opts & LW_GML_SHORTLINE);

	if (shortline)
	{
		stringbuffer_aprintf(sb, "<%sLineString", opts->prefix);
	}
	else
	{
		stringbuffer_aprintf(sb, "<%sCurve", opts->prefix);
	}

	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)  stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);

	if (lwline_is_empty(line))
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");

	if (!shortline)
	{
		stringbuffer_aprintf(sb, "<%ssegments>", opts->prefix);
		stringbuffer_aprintf(sb, "<%sLineStringSegment>", opts->prefix);
	}

	if (IS_DIMS(opts->opts))
	{
		stringbuffer_aprintf(sb, "<%sposList srsDimension=\"%d\">", opts->prefix, dimension);
	}
	else
	{
		stringbuffer_aprintf(sb, "<%sposList>", opts->prefix);
	}

	asgml3_ptarray(sb, line->points, opts);
	stringbuffer_aprintf(sb, "</%sposList>", opts->prefix);

	if (shortline)
	{
		stringbuffer_aprintf(sb, "</%sLineString>", opts->prefix);
	}
	else
	{
		stringbuffer_aprintf(sb, "</%sLineStringSegment>", opts->prefix);
		stringbuffer_aprintf(sb, "</%ssegments>", opts->prefix);
		stringbuffer_aprintf(sb, "</%sCurve>", opts->prefix);
	}
}

static void
asgml3_poly(stringbuffer_t* sb, const LWPOLY *poly, const GML_Options* opts)
{
	uint32_t i;
	int dimension = FLAGS_GET_Z(poly->flags) ? 3 : 2;

	stringbuffer_aprintf(sb,
		opts->is_patch ? "<%sPolygonPatch" : "<%sPolygon",
		opts->prefix);

	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)  stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);

	if (lwpoly_is_empty(poly))
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");

	stringbuffer_aprintf(sb, "<%sexterior>", opts->prefix);
	stringbuffer_aprintf(sb, "<%sLinearRing>", opts->prefix);
	if (IS_DIMS(opts->opts))
		stringbuffer_aprintf(sb, "<%sposList srsDimension=\"%d\">", opts->prefix, dimension);
	else
		stringbuffer_aprintf(sb, "<%sposList>", opts->prefix);

	asgml3_ptarray(sb, poly->rings[0], opts);
	stringbuffer_aprintf(sb, "</%sposList>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sLinearRing>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sexterior>", opts->prefix);
	for (i=1; i<poly->nrings; i++)
	{
		stringbuffer_aprintf(sb, "<%sinterior>", opts->prefix);
		stringbuffer_aprintf(sb, "<%sLinearRing>", opts->prefix);
		if (IS_DIMS(opts->opts))
			stringbuffer_aprintf(sb, "<%sposList srsDimension=\"%d\">", opts->prefix, dimension);
		else
			stringbuffer_aprintf(sb, "<%sposList>", opts->prefix);
		asgml3_ptarray(sb, poly->rings[i], opts);
		stringbuffer_aprintf(sb, "</%sposList>", opts->prefix);
		stringbuffer_aprintf(sb, "</%sLinearRing>", opts->prefix);
		stringbuffer_aprintf(sb, "</%sinterior>", opts->prefix);
	}

	stringbuffer_aprintf(sb,
		opts->is_patch ? "</%sPolygonPatch>" : "</%sPolygon>",
		opts->prefix);
}


static void
asgml3_circstring(stringbuffer_t* sb, const LWCIRCSTRING *circ, const GML_Options* opts)
{
	int dimension = FLAGS_GET_Z(circ->flags) ? 3 : 2;

	stringbuffer_aprintf(sb, "<%sCurve", opts->prefix);
	if (opts->srs)
	{
		stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	}
	if (opts->id)
	{
		stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);
	}
	stringbuffer_append(sb, ">");
	stringbuffer_aprintf(sb, "<%ssegments>", opts->prefix);
	stringbuffer_aprintf(sb, "<%sArcString>", opts->prefix);
	stringbuffer_aprintf(sb, "<%sposList", opts->prefix);

	if (IS_DIMS(opts->opts))
	{
		stringbuffer_aprintf(sb, " srsDimension=\"%d\"", dimension);
	}
	stringbuffer_append(sb, ">");

	asgml3_ptarray(sb, circ->points, opts);

	stringbuffer_aprintf(sb, "</%sposList>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sArcString>", opts->prefix);
	stringbuffer_aprintf(sb, "</%ssegments>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sCurve>", opts->prefix);
}

static void
asgml3_compound(stringbuffer_t* sb, const LWCOMPOUND *col, const GML_Options* opts)
{
	LWGEOM *subgeom;
	uint32_t i;
	int dimension = FLAGS_GET_Z(col->flags) ? 3 : 2;

	stringbuffer_aprintf(sb, "<%sCurve", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id) stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);
	stringbuffer_append(sb, ">");
	stringbuffer_aprintf(sb, "<%ssegments>", opts->prefix);

	for (i = 0; i < col->ngeoms; ++i)
	{
		subgeom = col->geoms[i];

		if (subgeom->type != LINETYPE && subgeom->type != CIRCSTRINGTYPE)
			continue;

		if (subgeom->type == LINETYPE)
		{
			stringbuffer_aprintf(sb, "<%sLineStringSegment>", opts->prefix);
			stringbuffer_aprintf(sb, "<%sposList", opts->prefix);
			if (IS_DIMS(opts->opts))
				stringbuffer_aprintf(sb, " srsDimension=\"%d\"", dimension);

			stringbuffer_append(sb, ">");
			asgml3_ptarray(sb, ((LWCIRCSTRING*)subgeom)->points, opts);
			stringbuffer_aprintf(sb, "</%sposList>", opts->prefix);
			stringbuffer_aprintf(sb, "</%sLineStringSegment>", opts->prefix);
		}
		else if( subgeom->type == CIRCSTRINGTYPE )
		{
			stringbuffer_aprintf(sb, "<%sArcString>", opts->prefix);
			stringbuffer_aprintf(sb, "<%sposList", opts->prefix);
			if (IS_DIMS(opts->opts))
			{
				stringbuffer_aprintf(sb, " srsDimension=\"%d\"", dimension);
			}
			stringbuffer_append(sb, ">");
			asgml3_ptarray(sb, ((LWLINE*)subgeom)->points, opts);
			stringbuffer_aprintf(sb, "</%sposList>", opts->prefix);
			stringbuffer_aprintf(sb, "</%sArcString>", opts->prefix);
		}
	}
	stringbuffer_aprintf(sb, "</%ssegments>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sCurve>", opts->prefix);
}

static void
asgml3_curvepoly(stringbuffer_t* sb, const LWCURVEPOLY* poly, const GML_Options* opts)
{
	uint32_t i;
	LWGEOM* subgeom;
	int dimension = FLAGS_GET_Z(poly->flags) ? 3 : 2;

	/* Subgeoms don't get an SRS */
	GML_Options subopts = *opts;
	subopts.srs = 0;

	stringbuffer_aprintf(sb, "<%sPolygon", opts->prefix);
	if (opts->srs)
	{
		stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	}
	if (opts->id)
	{
		stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);
	}
	stringbuffer_append(sb, ">");

	for (i = 0; i < poly->nrings; ++i)
	{
		stringbuffer_aprintf(sb,
			i ? "<%sinterior>" : "<%sexterior>",
			opts->prefix);

		subgeom = poly->rings[i];
		if (subgeom->type == LINETYPE)
		{
			stringbuffer_aprintf(sb, "<%sLinearRing>", opts->prefix);
			stringbuffer_aprintf(sb, "<%sposList", opts->prefix);
			if (IS_DIMS(opts->opts))
			{
				stringbuffer_aprintf(sb, " srsDimension=\"%d\"", dimension);
			}
			stringbuffer_append(sb, ">");
			asgml3_ptarray(sb, ((LWLINE*)subgeom)->points, opts);
			stringbuffer_aprintf(sb, "</%sposList>", opts->prefix);
			stringbuffer_aprintf(sb, "</%sLinearRing>", opts->prefix);
		}
		else if (subgeom->type == CIRCSTRINGTYPE)
		{
			stringbuffer_aprintf(sb, "<%sRing>", opts->prefix);
			stringbuffer_aprintf(sb, "<%scurveMember>", opts->prefix);
			asgml3_circstring(sb, (LWCIRCSTRING*)subgeom, &subopts);
			stringbuffer_aprintf(sb, "</%scurveMember>", opts->prefix);
			stringbuffer_aprintf(sb, "</%sRing>", opts->prefix);
		}
		else if (subgeom->type == COMPOUNDTYPE)
		{
			stringbuffer_aprintf(sb, "<%sRing>", opts->prefix);
			stringbuffer_aprintf(sb, "<%scurveMember>", opts->prefix);
			asgml3_compound(sb, (LWCOMPOUND*)subgeom, &subopts);
			stringbuffer_aprintf(sb, "</%scurveMember>", opts->prefix);
			stringbuffer_aprintf(sb, "</%sRing>", opts->prefix);
		}

		stringbuffer_aprintf(sb,
			i ? "</%sinterior>" : "</%sexterior>",
			opts->prefix);
	}
	stringbuffer_aprintf(sb, "</%sPolygon>", opts->prefix);
}

static void
asgml3_triangle(stringbuffer_t* sb, const LWTRIANGLE *triangle, const GML_Options* opts)
{
	int dimension = FLAGS_GET_Z(triangle->flags) ? 3 : 2;

	stringbuffer_aprintf(sb, "<%sTriangle", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)  stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);
	stringbuffer_append(sb, ">");

	stringbuffer_aprintf(sb, "<%sexterior>", opts->prefix);
	stringbuffer_aprintf(sb, "<%sLinearRing>", opts->prefix);
	if (IS_DIMS(opts->opts))
		stringbuffer_aprintf(sb, "<%sposList srsDimension=\"%d\">", opts->prefix, dimension);
	else
		stringbuffer_aprintf(sb, "<%sposList>", opts->prefix);

	asgml3_ptarray(sb, triangle->points, opts);
	stringbuffer_aprintf(sb, "</%sposList>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sLinearRing>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sexterior>", opts->prefix);

	stringbuffer_aprintf(sb, "</%sTriangle>", opts->prefix);
}


static void
asgml3_multi(stringbuffer_t* sb, const LWCOLLECTION *col, const GML_Options* opts)
{
	int type = col->type;
	uint32_t i;
	LWGEOM *subgeom;
	/* Subgeoms don't get an SRS */
	GML_Options subopts = *opts;
	subopts.srs = 0;

	const char* gmltype = "";

	if 	    (type == MULTIPOINTTYPE)   gmltype = "MultiPoint";
	else if (type == MULTILINETYPE)    gmltype = "MultiCurve";
	else if (type == MULTIPOLYGONTYPE) gmltype = "MultiSurface";

	/* Open outmost tag */
	stringbuffer_aprintf(sb, "<%s%s", opts->prefix, gmltype);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)  stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);

	if (!col->ngeoms)
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		if (subgeom->type == POINTTYPE)
		{
			stringbuffer_aprintf(sb, "<%spointMember>", opts->prefix);
			asgml3_point(sb, (LWPOINT*)subgeom, &subopts);
			stringbuffer_aprintf(sb, "</%spointMember>", opts->prefix);
		}
		else if (subgeom->type == LINETYPE)
		{
			stringbuffer_aprintf(sb, "<%scurveMember>", opts->prefix);
			asgml3_line(sb, (LWLINE*)subgeom, &subopts);
			stringbuffer_aprintf(sb, "</%scurveMember>", opts->prefix);
		}
		else if (subgeom->type == POLYGONTYPE)
		{
			stringbuffer_aprintf(sb, "<%ssurfaceMember>", opts->prefix);
			asgml3_poly(sb, (LWPOLY*)subgeom, &subopts);
			stringbuffer_aprintf(sb, "</%ssurfaceMember>", opts->prefix);
		}
	}

	/* Close outmost tag */
	stringbuffer_aprintf(sb, "</%s%s>", opts->prefix, gmltype);
}

/*
 * Don't call this with single-geoms inspected!
 */
static void
asgml3_tin(stringbuffer_t* sb, const LWTIN *tin, const GML_Options* opts)
{
	uint32_t i;

	/* Subgeoms don't get an SRS */
	GML_Options subopts = *opts;
	subopts.srs = 0;

	/* Open outmost tag */
	stringbuffer_aprintf(sb, "<%sTin", opts->prefix);
	if (opts->srs)
		stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)
		stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);
	stringbuffer_append(sb, ">");

	stringbuffer_aprintf(sb, "<%strianglePatches>", opts->prefix);
	for (i=0; i<tin->ngeoms; i++)
	{
		asgml3_triangle(sb, tin->geoms[i], &subopts);
	}

	/* Close outmost tag */
	stringbuffer_aprintf(sb, "</%strianglePatches>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sTin>", opts->prefix);
}

static void
asgml3_psurface(stringbuffer_t* sb, const LWPSURFACE *psur, const GML_Options* opts)
{
	uint32_t i;

	/* Subgeoms don't get an SRS */
	GML_Options subopts = *opts;
	subopts.srs = 0;
	subopts.is_patch = 1;

	/* Open outmost tag */
	stringbuffer_aprintf(sb, "<%sPolyhedralSurface", opts->prefix);
	if (opts->srs)
		stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)
		stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);
	stringbuffer_append(sb, ">");
	stringbuffer_aprintf(sb, "<%spolygonPatches>", opts->prefix);

	for (i=0; i<psur->ngeoms; i++)
	{
		asgml3_poly(sb, psur->geoms[i], &subopts);
	}

	/* Close outmost tag */
	stringbuffer_aprintf(sb, "</%spolygonPatches>", opts->prefix);
	stringbuffer_aprintf(sb, "</%sPolyhedralSurface>", opts->prefix);
}


static void
asgml3_collection(stringbuffer_t* sb, const LWCOLLECTION *col, const GML_Options* opts)
{
	uint32_t i;
	LWGEOM *subgeom;

	/* Subgeoms don't get an SRS */
	GML_Options subopts = *opts;
	subopts.srs = 0;

	/* Open outmost tag */
	stringbuffer_aprintf(sb, "<%sMultiGeometry", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)  stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);

	if (!col->ngeoms)
	{
		stringbuffer_append(sb, "/>");
		return;
	}
	stringbuffer_append(sb, ">");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		stringbuffer_aprintf(sb, "<%sgeometryMember>", opts->prefix);

		switch (subgeom->type)
		{
			case POINTTYPE:
				asgml3_point(sb, (LWPOINT*)subgeom, &subopts);
				break;
			case LINETYPE:
				asgml3_line(sb, (LWLINE*)subgeom, &subopts);
				break;
			case POLYGONTYPE:
				asgml3_poly(sb, (LWPOLY*)subgeom, &subopts);
				break;
			case MULTIPOINTTYPE:
			case MULTILINETYPE:
			case MULTIPOLYGONTYPE:
				asgml3_multi(sb, (LWCOLLECTION*)subgeom, &subopts);
				break;
			case COLLECTIONTYPE:
				asgml3_collection(sb, (LWCOLLECTION*)subgeom, &subopts);
				break;
			default:
				lwerror("asgml3_collection: unknown geometry type");
		}
		stringbuffer_aprintf(sb, "</%sgeometryMember>", opts->prefix);
	}

	/* Close outmost tag */
	stringbuffer_aprintf(sb, "</%sMultiGeometry>", opts->prefix);
}

static void
asgml3_multicurve(stringbuffer_t*sb, const LWMCURVE* cur, const GML_Options* opts)
{
	LWGEOM* subgeom;
	uint32_t i;

	stringbuffer_aprintf(sb, "<%sMultiCurve", opts->prefix);
	if (opts->srs)
	{
		stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	}
	if (opts->id)
	{
		stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);
	}
	stringbuffer_append(sb, ">");

	for (i = 0; i < cur->ngeoms; ++i)
	{
		stringbuffer_aprintf(sb, "<%scurveMember>", opts->prefix);
		subgeom = cur->geoms[i];
		if (subgeom->type == LINETYPE)
		{
			asgml3_line(sb, (LWLINE*)subgeom, opts);
		}
		else if (subgeom->type == CIRCSTRINGTYPE)
		{
			asgml3_circstring(sb, (LWCIRCSTRING*)subgeom, opts);
		}
		else if (subgeom->type == COMPOUNDTYPE)
		{
			asgml3_compound(sb, (LWCOMPOUND*)subgeom, opts);
		}
		stringbuffer_aprintf(sb, "</%scurveMember>", opts->prefix);
	}
	stringbuffer_aprintf(sb, "</%sMultiCurve>", opts->prefix);
}


static void
asgml3_multisurface(stringbuffer_t* sb, const LWMSURFACE *sur, const GML_Options* opts)
{
	uint32_t i;
	LWGEOM* subgeom;

	stringbuffer_aprintf(sb, "<%sMultiSurface", opts->prefix);
	if (opts->srs) stringbuffer_aprintf(sb, " srsName=\"%s\"", opts->srs);
	if (opts->id)  stringbuffer_aprintf(sb, " %sid=\"%s\"", opts->prefix, opts->id);

	stringbuffer_append(sb, ">");

	for (i = 0; i < sur->ngeoms; ++i)
	{
		subgeom = sur->geoms[i];
		if (subgeom->type == POLYGONTYPE)
		{
			asgml3_poly(sb, (LWPOLY*)sur->geoms[i], opts);
		}
		else if (subgeom->type == CURVEPOLYTYPE)
		{
			asgml3_curvepoly(sb, (LWCURVEPOLY*)sur->geoms[i], opts);
		}
	}
	stringbuffer_aprintf(sb, "</%sMultiSurface>", opts->prefix);
}

extern lwvarlena_t *
lwgeom_to_gml2(const LWGEOM *geom, const char *srs, int precision, const char *prefix)
{
	stringbuffer_t sb;

	/* Initialize options */
	GML_Options gmlopts;
	memset(&gmlopts, 0, sizeof(gmlopts));
	gmlopts.srs = srs;
	gmlopts.precision = precision;
	gmlopts.prefix = prefix;

	/* Return null for empty (#1377) */
	if (lwgeom_is_empty(geom))
		return NULL;

	stringbuffer_init_varlena(&sb);

	switch (geom->type)
	{
	case POINTTYPE:
		asgml2_point(&sb, (LWPOINT*)geom, &gmlopts);
		break;

	case LINETYPE:
		asgml2_line(&sb, (LWLINE*)geom, &gmlopts);
		break;

	case POLYGONTYPE:
		asgml2_poly(&sb, (LWPOLY*)geom, &gmlopts);
		break;

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		asgml2_multi(&sb, (LWCOLLECTION*)geom, &gmlopts);
		break;

	case COLLECTIONTYPE:
		asgml2_collection(&sb, (LWCOLLECTION*)geom, &gmlopts);
		break;

	case TRIANGLETYPE:
	case POLYHEDRALSURFACETYPE:
	case TINTYPE:
		lwerror("Cannot convert %s to GML2. Try ST_AsGML(3, <geometry>) to generate GML3.", lwtype_name(geom->type));
		stringbuffer_release(&sb);
		return NULL;

	default:
		lwerror("lwgeom_to_gml2: '%s' geometry type not supported", lwtype_name(geom->type));
		stringbuffer_release(&sb);
		return NULL;
	}

	return stringbuffer_getvarlena(&sb);
}

extern lwvarlena_t *
lwgeom_to_gml3(const LWGEOM *geom, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
	stringbuffer_t sb;

	/* Initialize options */
	GML_Options gmlopts;
	memset(&gmlopts, 0, sizeof(gmlopts));
	gmlopts.srs = srs;
	gmlopts.precision = precision;
	gmlopts.opts = opts;
	gmlopts.prefix = prefix;
	gmlopts.id = id;

	/* Return null for empty (#1377) */
	if (lwgeom_is_empty(geom))
		return NULL;

	stringbuffer_init_varlena(&sb);

	switch (geom->type)
	{
	case POINTTYPE:
		asgml3_point(&sb, (LWPOINT*)geom, &gmlopts);
		break;

	case LINETYPE:
		asgml3_line(&sb, (LWLINE*)geom, &gmlopts);
		break;

	case CIRCSTRINGTYPE:
		asgml3_circstring(&sb, (LWCIRCSTRING*)geom, &gmlopts );
		break;

	case POLYGONTYPE:
		asgml3_poly(&sb, (LWPOLY*)geom, &gmlopts);
		break;

	case CURVEPOLYTYPE:
		asgml3_curvepoly(&sb, (LWCURVEPOLY*)geom, &gmlopts);
		break;

	case TRIANGLETYPE:
		asgml3_triangle(&sb, (LWTRIANGLE*)geom, &gmlopts);
		break;

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
		asgml3_multi(&sb, (LWCOLLECTION*)geom, &gmlopts);
		break;

	case POLYHEDRALSURFACETYPE:
		asgml3_psurface(&sb, (LWPSURFACE*)geom, &gmlopts);
		break;

	case TINTYPE:
		asgml3_tin(&sb, (LWTIN*)geom, &gmlopts);
		break;

	case COLLECTIONTYPE:
		asgml3_collection(&sb, (LWCOLLECTION*)geom, &gmlopts);
		break;

	case COMPOUNDTYPE:
		asgml3_compound(&sb, (LWCOMPOUND*)geom, &gmlopts );
		break;

	case MULTICURVETYPE:
		asgml3_multicurve(&sb, (LWMCURVE*)geom, &gmlopts );
		break;

	case MULTISURFACETYPE:
		asgml3_multisurface(&sb, (LWMSURFACE*)geom, &gmlopts );
		break;

	default:
		lwerror("lwgeom_to_gml3: '%s' geometry type not supported", lwtype_name(geom->type));
		stringbuffer_release(&sb);
		return NULL;
	}

	return stringbuffer_getvarlena(&sb);
}

extern lwvarlena_t *
lwgeom_extent_to_gml2(const LWGEOM *geom, const char *srs, int precision, const char *prefix)
{
	const GBOX* bbox = lwgeom_get_bbox(geom);
	stringbuffer_t sb;

	/* Initialize options */
	GML_Options gmlopts;
	memset(&gmlopts, 0, sizeof(gmlopts));
	gmlopts.srs = srs;
	gmlopts.precision = precision;
	gmlopts.prefix = prefix;

	stringbuffer_init_varlena(&sb);
	asgml2_gbox(&sb, bbox, &gmlopts);
	return stringbuffer_getvarlena(&sb);
}

extern lwvarlena_t *
lwgeom_extent_to_gml3(const LWGEOM *geom, const char *srs, int precision, int opts, const char *prefix)
{
	const GBOX* bbox = lwgeom_get_bbox(geom);
	stringbuffer_t sb;

	/* Initialize options */
	GML_Options gmlopts;
	memset(&gmlopts, 0, sizeof(gmlopts));
	gmlopts.srs = srs;
	gmlopts.precision = precision;
	gmlopts.opts = opts;
	gmlopts.prefix = prefix;

	stringbuffer_init_varlena(&sb);
	asgml3_gbox(&sb, bbox, &gmlopts);
	return stringbuffer_getvarlena(&sb);
}
