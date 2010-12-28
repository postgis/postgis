/**********************************************************************
 * $Id$
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://www.postgis.org
 * Copyright 2008 OpenGeo.org
 * Copyright 2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 * Maintainer: Paul Ramsey <pramsey@opengeo.org>
 *
 **********************************************************************/

#include "shp2pgsql-core.h"


/* Internal ring/point structures */
typedef struct struct_point
{
	double x, y, z, m;
} Point;

typedef struct struct_ring
{
	Point *list;		/* list of points */
	struct struct_ring *next;
	int n;			/* number of points in list */
	uint32 linked; 	/* number of "next" rings */
} Ring;


/* liblwgeom allocator callback - install the defaults (malloc/free/stdout/stderr) */
void lwgeom_init_allocators()
{
	lwgeom_install_default_allocators();
}


/*
 * Internal functions
 */

#define UTF8_GOOD_RESULT 0
#define UTF8_BAD_RESULT 1
#define UTF8_NO_RESULT 2

int utf8(const char *fromcode, char *inputbuf, char **outputbuf);
char *escape_copy_string(char *str);
char *escape_insert_string(char *str);

int GeneratePointGeometry(SHPLOADERSTATE *state, SHPObject *obj, char **geometry);
int GenerateLineStringGeometry(SHPLOADERSTATE *state, SHPObject *obj, char **geometry);
int PIP(Point P, Point *V, int n);
int FindPolygons(SHPObject *obj, Ring ***Out);
void ReleasePolygons(Ring **polys, int npolys);
int GeneratePolygonGeometry(SHPLOADERSTATE *state, SHPObject *obj, char **geometry);

/* Append variadic formatted string to a stringbuffer */
void
vasbappend(stringbuffer_t *sb, char *fmt, ... )
{
	va_list ap;
	char *msg;

	va_start(ap, fmt);

	if (!lw_vasprintf (&msg, fmt, ap))
	{
		va_end (ap);
		return;
	}

	/* Append to the stringbuffer */
	stringbuffer_append(sb, msg);
	free(msg);

	va_end(ap);
}

/* Return allocated string containing UTF8 string converted from encoding fromcode */
int utf8(const char *fromcode, char *inputbuf, char **outputbuf)
{
	iconv_t cd;
	char *outputptr;
	size_t outbytesleft;
	size_t inbytesleft;

	inbytesleft = strlen(inputbuf);

	cd = iconv_open("UTF-8", fromcode);
	if ( cd == ((iconv_t)(-1)) )
		return UTF8_NO_RESULT;

	outbytesleft = inbytesleft * 3 + 1; /* UTF8 string can be 3 times larger */
	/* then local string */
	*outputbuf = (char *)malloc(outbytesleft);
	if (!*outputbuf)
		return UTF8_NO_RESULT;

	memset(*outputbuf, 0, outbytesleft);
	outputptr = *outputbuf;

	/* Does this string convert cleanly? */
	if ( iconv(cd, &inputbuf, &inbytesleft, &outputptr, &outbytesleft) == -1 )
	{
#ifdef HAVE_ICONVCTL
		int on = 1;
		/* No. Try to convert it while transliterating. */
		iconvctl(cd, ICONV_SET_TRANSLITERATE, &on);
		if ( iconv(cd, &inputbuf, &inbytesleft, &outputptr, &outbytesleft) == -1 )
		{
			/* No. Try to convert it while discarding errors. */
			iconvctl(cd, ICONV_SET_DISCARD_ILSEQ, &on);
			if ( iconv(cd, &inputbuf, &inbytesleft, &outputptr, &outbytesleft) == -1 )
			{
				/* Still no. Throw away the buffer and return. */
				free(*outputbuf);
				iconv_close(cd);
				return UTF8_NO_RESULT;
			}
		}
		iconv_close(cd);
		return UTF8_BAD_RESULT;
#else
		free(*outputbuf);
		iconv_close(cd);
		return UTF8_NO_RESULT;
#endif
	}
	/* Return a good result, converted string is in buffer. */
	iconv_close(cd);
	return UTF8_GOOD_RESULT;
}

/**
 * Escape input string suitable for COPY. If no characters require escaping, simply return
 * the input pointer. Otherwise return a new allocated string.
 */
char *
escape_copy_string(char *str)
{
	/*
	 * Escape the following characters by adding a preceding backslash
	 *      tab, backslash, cr, lf
	 *
	 * 1. find # of escaped characters
	 * 2. make new string
	 *
	 */

	char *result;
	char *ptr, *optr;
	int toescape = 0;
	size_t size;

	ptr = str;

	/* Count how many characters we need to escape so we know the size of the string we need to return */
	while (*ptr)
	{
		if (*ptr == '\t' || *ptr == '\\' || *ptr == '\n' || *ptr == '\r')
			toescape++;

		ptr++;
	}

	/* If we don't have to escape anything, simply return the input pointer */
	if (toescape == 0)
		return str;

	size = ptr - str + toescape + 1;
	result = calloc(1, size);
	optr = result;
	ptr = str;

	while (*ptr)
	{
		if ( *ptr == '\t' || *ptr == '\\' || *ptr == '\n' || *ptr == '\r' )
			*optr++ = '\\';

		*optr++ = *ptr++;
	}

	*optr = '\0';

	return result;
}


/**
 * Escape input string suitable for INSERT. If no characters require escaping, simply return
 * the input pointer. Otherwise return a new allocated string.
 */
char *
escape_insert_string(char *str)
{
	/*
	 * Escape single quotes by adding a preceding single quote
	 *
	 * 1. find # of characters
	 * 2. make new string
	 */

	char *result;
	char *ptr, *optr;
	int toescape = 0;
	size_t size;

	ptr = str;

	/* Count how many characters we need to escape so we know the size of the string we need to return */
	while (*ptr)
	{
		if (*ptr == '\'')
			toescape++;

		ptr++;
	}

	/* If we don't have to escape anything, simply return the input pointer */
	if (toescape == 0)
		return str;

	size = ptr - str + toescape + 1;
	result = calloc(1, size);
	optr = result;
	ptr = str;

	while (*ptr)
	{
		if (*ptr == '\'')
			*optr++='\'';

		*optr++ = *ptr++;
	}

	*optr='\0';

	return result;
}


/**
 * @brief Generate an allocated geometry string for shapefile object obj using the state parameters
 */
int
GeneratePointGeometry(SHPLOADERSTATE *state, SHPObject *obj, char **geometry)
{
	LWGEOM **lwmultipoints;
	LWGEOM *lwgeom = NULL;

	POINT4D point4d;

	int dims = 0, hasz = 0, hasm = 0;
	int u;

	char *mem;
	size_t mem_length;


	/* Determine the correct dimensions: note that in hwgeom-compatible mode we cannot use
	   the M coordinate */
	if (state->wkbtype & WKBZOFFSET)
		hasz = 1;

	if (!state->config->hwgeom)
		if (state->wkbtype & WKBMOFFSET)
			hasm = 1;

	FLAGS_SET_Z(dims, hasz?1:0);
	FLAGS_SET_M(dims, hasm?1:0);

	/* Allocate memory for our array of LWPOINTs and our dynptarrays */
	lwmultipoints = malloc(sizeof(LWPOINT *) * obj->nVertices);

	/* We need an array of pointers to each of our sub-geometries */
	for (u = 0; u < obj->nVertices; u++)
	{
		/* Create a ptarray containing a single point */
		POINTARRAY *pa = ptarray_construct_empty(hasz, hasm, 1);
		
		/* Generate the point */
		point4d.x = obj->padfX[u];
		point4d.y = obj->padfY[u];

		if (state->wkbtype & WKBZOFFSET)
			point4d.z = obj->padfZ[u];
		if (state->wkbtype & WKBMOFFSET)
			point4d.m = obj->padfM[u];

		/* Add in the point! */
		ptarray_append_point(pa, &point4d, REPEATED_POINTS_OK);

		/* Generate the LWPOINT */
		lwmultipoints[u] = lwpoint_as_lwgeom(lwpoint_construct(state->config->sr_id, NULL, pa));
	}

	/* If we have more than 1 vertex then we are working on a MULTIPOINT and so generate a MULTIPOINT
	rather than a POINT */
	if (obj->nVertices > 1)
	{
		lwgeom = lwcollection_as_lwgeom(lwcollection_construct(MULTIPOINTTYPE, state->config->sr_id, NULL, obj->nVertices, lwmultipoints));
	}
	else
	{
		lwgeom = lwmultipoints[0];
		lwfree(lwmultipoints);
	}

	if (state->config->hwgeom)
	{
		/* Old style "heavy" geometries (PostGIS < 1.0) */
		mem = lwgeom_to_wkt(lwgeom, WKT_EXTENDED, 12, &mem_length);
	}
	else
	{
		/* New "lightweight" geometries (PostGIS >= 1.0) */
		mem = lwgeom_to_hexwkb(lwgeom, WKB_EXTENDED, &mem_length);
	}

	if ( !mem )
	{
		snprintf(state->message, SHPLOADERMSGLEN, "unable to write geometry");
		return SHPLOADERERR;
	}

	/* Free all of the allocated items */
	lwgeom_free(lwgeom);
	
	/* Return the string - everything ok */
	*geometry = mem;

	return SHPLOADEROK;
}


/**
 * @brief Generate an allocated geometry string for shapefile object obj using the state parameters
 */
int
GenerateLineStringGeometry(SHPLOADERSTATE *state, SHPObject *obj, char **geometry)
{

	LWGEOM **lwmultilinestrings;
	LWGEOM *lwgeom = NULL;
	POINT4D point4d;
	int dims = 0, hasz = 0, hasm = 0;
	int u, v, start_vertex, end_vertex;
	char *mem;
	size_t mem_length;


	/* Determine the correct dimensions: note that in hwgeom-compatible mode we cannot use
	   the M coordinate */
	if (state->wkbtype & WKBZOFFSET)
		hasz = 1;

	if (!state->config->hwgeom)
		if (state->wkbtype & WKBMOFFSET)
			hasm = 1;

	FLAGS_SET_Z(dims, hasz?1:0);
	FLAGS_SET_M(dims, hasm?1:0);

	if (state->config->simple_geometries == 1 && obj->nParts > 1)
	{
		snprintf(state->message, SHPLOADERMSGLEN, _("We have a Multilinestring with %d parts, can't use -S switch!"), obj->nParts);

		return SHPLOADERERR;
	}

	/* Allocate memory for our array of LWLINEs and our dynptarrays */
	lwmultilinestrings = malloc(sizeof(LWPOINT *) * obj->nParts);

	/* We need an array of pointers to each of our sub-geometries */
	for (u = 0; u < obj->nParts; u++)
	{
		/* Create a ptarray containing the line points */
		POINTARRAY *pa = ptarray_construct_empty(hasz, hasm, obj->nParts);

		/* Set the start/end vertices depending upon whether this is
		a MULTILINESTRING or not */
		if ( u == obj->nParts-1 )
			end_vertex = obj->nVertices;
		else
			end_vertex = obj->panPartStart[u + 1];

		start_vertex = obj->panPartStart[u];

		for (v = start_vertex; v < end_vertex; v++)
		{
			/* Generate the point */
			point4d.x = obj->padfX[v];
			point4d.y = obj->padfY[v];

			if (state->wkbtype & WKBZOFFSET)
				point4d.z = obj->padfZ[v];
			if (state->wkbtype & WKBMOFFSET)
				point4d.m = obj->padfM[v];

			ptarray_append_point(pa, &point4d, REPEATED_POINTS_NOT_OK);
		}

		/* Generate the LWLINE */
		lwmultilinestrings[u] = lwline_as_lwgeom(lwline_construct(state->config->sr_id, NULL, pa));
	}

	/* If using MULTILINESTRINGs then generate the serialized collection, otherwise just a single LINESTRING */
	if (state->config->simple_geometries == 0)
	{
		lwgeom = lwcollection_as_lwgeom(lwcollection_construct(MULTILINETYPE, state->config->sr_id, NULL, obj->nParts, lwmultilinestrings));
	}
	else
	{
		lwgeom = lwmultilinestrings[0];
		lwfree(lwmultilinestrings);
	}

	if (!state->config->hwgeom)
		mem = lwgeom_to_hexwkb(lwgeom, WKB_EXTENDED, &mem_length);
	else
		mem = lwgeom_to_wkt(lwgeom, WKT_EXTENDED, 12, &mem_length);

	if ( !mem )
	{
		snprintf(state->message, SHPLOADERMSGLEN, "unable to write geometry");
		return SHPLOADERERR;
	}

	/* Free all of the allocated items */
	lwgeom_free(lwgeom);

	/* Return the string - everything ok */
	*geometry = mem;

	return SHPLOADEROK;
}


/**
 * @brief PIP(): crossing number test for a point in a polygon
 *      input:   P = a point,
 *               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
 * @return   0 = outside, 1 = inside
 */
int
PIP(Point P, Point *V, int n)
{
	int cn = 0;    /* the crossing number counter */
	int i;

	/* loop through all edges of the polygon */
	for (i = 0; i < n-1; i++)      /* edge from V[i] to V[i+1] */
	{
		if (((V[i].y <= P.y) && (V[i + 1].y > P.y))    /* an upward crossing */
		        || ((V[i].y > P.y) && (V[i + 1].y <= P.y)))   /* a downward crossing */
		{
			double vt = (float)(P.y - V[i].y) / (V[i + 1].y - V[i].y);
			if (P.x < V[i].x + vt * (V[i + 1].x - V[i].x)) /* P.x < intersect */
				++cn;   /* a valid crossing of y=P.y right of P.x */
		}
	}

	return (cn&1);    /* 0 if even (out), and 1 if odd (in) */
}


int
FindPolygons(SHPObject *obj, Ring ***Out)
{
	Ring **Outer;    /* Pointers to Outer rings */
	int out_index=0; /* Count of Outer rings */
	Ring **Inner;    /* Pointers to Inner rings */
	int in_index=0;  /* Count of Inner rings */
	int pi; /* part index */

#if POSTGIS_DEBUG_LEVEL > 0
	static int call = -1;
	call++;
#endif

	LWDEBUGF(4, "FindPolygons[%d]: allocated space for %d rings\n", call, obj->nParts);

	/* Allocate initial memory */
	Outer = (Ring **)malloc(sizeof(Ring *) * obj->nParts);
	Inner = (Ring **)malloc(sizeof(Ring *) * obj->nParts);

	/* Iterate over rings dividing in Outers and Inners */
	for (pi=0; pi < obj->nParts; pi++)
	{
		int vi; /* vertex index */
		int vs; /* start index */
		int ve; /* end index */
		int nv; /* number of vertex */
		double area = 0.0;
		Ring *ring;

		/* Set start and end vertexes */
		if (pi == obj->nParts - 1)
			ve = obj->nVertices;
		else
			ve = obj->panPartStart[pi + 1];

		vs = obj->panPartStart[pi];

		/* Compute number of vertexes */
		nv = ve - vs;

		/* Allocate memory for a ring */
		ring = (Ring *)malloc(sizeof(Ring));
		ring->list = (Point *)malloc(sizeof(Point) * nv);
		ring->n = nv;
		ring->next = NULL;
		ring->linked = 0;

		/* Iterate over ring vertexes */
		for (vi = vs; vi < ve; vi++)
		{
			int vn = vi+1; /* next vertex for area */
			if (vn == ve)
				vn = vs;

			ring->list[vi - vs].x = obj->padfX[vi];
			ring->list[vi - vs].y = obj->padfY[vi];
			ring->list[vi - vs].z = obj->padfZ[vi];
			ring->list[vi - vs].m = obj->padfM[vi];

			area += (obj->padfX[vi] * obj->padfY[vn]) -
			        (obj->padfY[vi] * obj->padfX[vn]);
		}

		/* Close the ring with first vertex  */
		/*ring->list[vi].x = obj->padfX[vs]; */
		/*ring->list[vi].y = obj->padfY[vs]; */
		/*ring->list[vi].z = obj->padfZ[vs]; */
		/*ring->list[vi].m = obj->padfM[vs]; */

		/* Clockwise (or single-part). It's an Outer Ring ! */
		if (area < 0.0 || obj->nParts == 1)
		{
			Outer[out_index] = ring;
			out_index++;
		}
		else
		{
			/* Counterclockwise. It's an Inner Ring ! */
			Inner[in_index] = ring;
			in_index++;
		}
	}

	LWDEBUGF(4, "FindPolygons[%d]: found %d Outer, %d Inners\n", call, out_index, in_index);

	/* Put the inner rings into the list of the outer rings */
	/* of which they are within */
	for (pi = 0; pi < in_index; pi++)
	{
		Point pt, pt2;
		int i;
		Ring *inner = Inner[pi], *outer = NULL;

		pt.x = inner->list[0].x;
		pt.y = inner->list[0].y;

		pt2.x = inner->list[1].x;
		pt2.y = inner->list[1].y;

		for (i = 0; i < out_index; i++)
		{
			int in;

			in = PIP(pt, Outer[i]->list, Outer[i]->n);
			if ( in || PIP(pt2, Outer[i]->list, Outer[i]->n) )
			{
				outer = Outer[i];
				break;
			}
			/*fprintf(stderr, "!PIP %s\nOUTE %s\n", dump_ring(inner), dump_ring(Outer[i])); */
		}

		if (outer)
		{
			outer->linked++;
			while (outer->next)
				outer = outer->next;

			outer->next = inner;
		}
		else
		{
			/* The ring wasn't within any outer rings, */
			/* assume it is a new outer ring. */
			LWDEBUGF(4, "FindPolygons[%d]: hole %d is orphan\n", call, pi);

			Outer[out_index] = inner;
			out_index++;
		}
	}

	*Out = Outer;
	free(Inner);

	return out_index;
}


void
ReleasePolygons(Ring **polys, int npolys)
{
	int pi;

	/* Release all memory */
	for (pi = 0; pi < npolys; pi++)
	{
		Ring *Poly, *temp;
		Poly = polys[pi];
		while (Poly != NULL)
		{
			temp = Poly;
			Poly = Poly->next;
			free(temp->list);
			free(temp);
		}
	}

	free(polys);
}


/**
 * @brief Generate an allocated geometry string for shapefile object obj using the state parameters
 *
 * This function basically deals with the polygon case. It sorts the polys in order of outer,
 * inner,inner, so that inners always come after outers they are within.
 *
 */
int
GeneratePolygonGeometry(SHPLOADERSTATE *state, SHPObject *obj, char **geometry)
{
	Ring **Outer;
	int polygon_total, ring_total;
	int pi, vi; /* part index and vertex index */

	LWGEOM **lwpolygons;
	LWGEOM *lwgeom;

	POINT4D point4d;

	int dims = 0, hasz = 0, hasm = 0;

	char *mem;
	size_t mem_length;

	/* Determine the correct dimensions: note that in hwgeom-compatible mode we cannot use
	   the M coordinate */
	if (state->wkbtype & WKBZOFFSET)
		hasz = 1;

	if (!state->config->hwgeom)
		if (state->wkbtype & WKBMOFFSET)
			hasm = 1;

	FLAGS_SET_Z(dims, hasz?1:0);
	FLAGS_SET_M(dims, hasm?1:0);

	polygon_total = FindPolygons(obj, &Outer);

	if (state->config->simple_geometries == 1 && polygon_total != 1) /* We write Non-MULTI geometries, but have several parts: */
	{
		snprintf(state->message, SHPLOADERMSGLEN, _("We have a Multipolygon with %d parts, can't use -S switch!"), polygon_total);

		return SHPLOADERERR;
	}

	/* Allocate memory for our array of LWPOLYs */
	lwpolygons = malloc(sizeof(LWPOLY *) * polygon_total);

	/* Cycle through each individual polygon */
	for (pi = 0; pi < polygon_total; pi++)
	{
		LWPOLY *lwpoly = lwpoly_construct_empty(state->config->sr_id, hasz, hasm);
		
		Ring *polyring;
		int ring_index = 0;

		/* Firstly count through the total number of rings in this polygon */
		ring_total = 0;
		polyring = Outer[pi];
		while (polyring)
		{
			ring_total++;
			polyring = polyring->next;
		}

		/* Cycle through each ring within the polygon, starting with the outer */
		polyring = Outer[pi];

		while (polyring)
		{
			/* Create a POINTARRAY containing the points making up the ring */
			POINTARRAY *pa = ptarray_construct_empty(hasz, hasm, polyring->n);

			for (vi = 0; vi < polyring->n; vi++)
			{
				/* Build up a point array of all the points in this ring */
				point4d.x = polyring->list[vi].x;
				point4d.y = polyring->list[vi].y;

				if (state->wkbtype & WKBZOFFSET)
					point4d.z = polyring->list[vi].z;
				if (state->wkbtype & WKBMOFFSET)
					point4d.m = polyring->list[vi].m;

				ptarray_append_point(pa, &point4d, REPEATED_POINTS_OK);
			}

			/* Copy the POINTARRAY pointer so we can use the LWPOLY constructor */
			lwpoly_add_ring(lwpoly, pa);

			polyring = polyring->next;
			ring_index++;
		}

		/* Generate the LWGEOM */
		lwpolygons[pi] = lwpoly_as_lwgeom(lwpoly);
	}

	/* If using MULTIPOLYGONS then generate the serialized collection, otherwise just a single POLYGON */
	if (state->config->simple_geometries == 0)
	{
		lwgeom = lwcollection_as_lwgeom(lwcollection_construct(MULTIPOLYGONTYPE, state->config->sr_id, NULL, polygon_total, lwpolygons));
	}
	else
	{
		lwgeom = lwpolygons[0];
		lwfree(lwpolygons);
	}

	if (!state->config->hwgeom)
		mem = lwgeom_to_hexwkb(lwgeom, WKB_EXTENDED, &mem_length);
	else
		mem = lwgeom_to_wkt(lwgeom, WKT_EXTENDED, 12, &mem_length);

	if ( !mem )
	{
		snprintf(state->message, SHPLOADERMSGLEN, "unable to write geometry");
		return SHPLOADERERR;
	}

	/* Free all of the allocated items */
	lwgeom_free(lwgeom);

	/* Free the linked list of rings */
	ReleasePolygons(Outer, polygon_total);

	/* Return the string - everything ok */
	*geometry = mem;

	return SHPLOADEROK;
}


/*
 * External functions (defined in shp2pgsql-core.h)
 */


/* Convert the string to lower case */
void
strtolower(char *s)
{
	int j;

	for (j = 0; j < strlen(s); j++)
		s[j] = tolower(s[j]);
}


/* Default configuration settings */
void
set_config_defaults(SHPLOADERCONFIG *config)
{
	config->opt = 'c';
	config->table = NULL;
	config->schema = NULL;
	config->geom = strdup(GEOMETRY_DEFAULT);
	config->shp_file = NULL;
	config->dump_format = 0;
	config->simple_geometries = 0;
	config->geography = 0;
	config->quoteidentifiers = 0;
	config->forceint4 = 0;
	config->createindex = 0;
	config->readshape = 1;
	config->encoding = strdup(ENCODING_DEFAULT);
	config->null_policy = POLICY_NULL_INSERT;
	config->sr_id = -1;
	config->hwgeom = 0;
}

/* Create a new shapefile state object */
SHPLOADERSTATE *
ShpLoaderCreate(SHPLOADERCONFIG *config)
{
	SHPLOADERSTATE *state;

	/* Create a new state object and assign the config to it */
	state = malloc(sizeof(SHPLOADERSTATE));
	state->config = config;

	/* Set any state defaults */
	state->hSHPHandle = NULL;
	state->hDBFHandle = NULL;
	state->wkbtype = 0;
	state->types = NULL;
	state->widths = NULL;
	state->precisions = NULL;
	state->col_names = NULL;

	return state;
}


/* Open the shapefile and extract the relevant field information */
int
ShpLoaderOpenShape(SHPLOADERSTATE *state)
{
	SHPObject *obj = NULL;
	int j, z;
	int ret = SHPLOADEROK;

	int field_precision, field_width;
	char name[MAXFIELDNAMELEN];
	char name2[MAXFIELDNAMELEN];
	DBFFieldType type = -1;
	char *utf8str;

	/* If we are reading the entire shapefile, open it */
	if (state->config->readshape == 1)
	{
		state->hSHPHandle = SHPOpen(state->config->shp_file, "rb");

		if (state->hSHPHandle == NULL)
		{
			snprintf(state->message, SHPLOADERMSGLEN, _("%s: shape (.shp) or index files (.shx) can not be opened, will just import attribute data."), state->config->shp_file);
			state->config->readshape = 0;

			ret = SHPLOADERWARN;
		}
	}

	/* Open the DBF (attributes) file */
	state->hDBFHandle = DBFOpen(state->config->shp_file, "rb");
	if ((state->hSHPHandle == NULL && state->config->readshape == 1) || state->hDBFHandle == NULL)
	{
		snprintf(state->message, SHPLOADERMSGLEN, _("%s: dbf file (.dbf) can not be opened."), state->config->shp_file);

		return SHPLOADERERR;
	}

	/* If reading the whole shapefile (not just attributes)... */
	if (state->config->readshape == 1)
	{
		SHPGetInfo(state->hSHPHandle, &state->num_entities, &state->shpfiletype, NULL, NULL);

		/* If null_policy is set to abort, check for NULLs */
		if (state->config->null_policy == POLICY_NULL_ABORT)
		{
			/* If we abort on null items, scan the entire file for NULLs */
			for (j = 0; j < state->num_entities; j++)
			{
				obj = SHPReadObject(state->hSHPHandle, j);

				if (!obj)
				{
					snprintf(state->message, SHPLOADERMSGLEN, _("Error reading shape object %d"), j);
					return SHPLOADERERR;
				}

				if (obj->nVertices == 0)
				{
					snprintf(state->message, SHPLOADERMSGLEN, _("Empty geometries found, aborted.)"));
					return SHPLOADERERR;
				}

				SHPDestroyObject(obj);
			}
		}

		/* Check the shapefile type */
		switch (state->shpfiletype)
		{
		case SHPT_POINT:
			/* Point */
			state->pgtype = "POINT";
			state->wkbtype = POINTTYPE;
			state->pgdims = 2;
			break;

		case SHPT_ARC:
			/* PolyLine */
			state->pgtype = "MULTILINESTRING";
			state->wkbtype = MULTILINETYPE ;
			state->pgdims = 2;
			break;

		case SHPT_POLYGON:
			/* Polygon */
			state->pgtype = "MULTIPOLYGON";
			state->wkbtype = MULTIPOLYGONTYPE;
			state->pgdims = 2;
			break;

		case SHPT_MULTIPOINT:
			/* MultiPoint */
			state->pgtype = "MULTIPOINT";
			state->wkbtype = MULTIPOINTTYPE;
			state->pgdims = 2;
			break;

		case SHPT_POINTM:
			/* PointM */
			state->wkbtype = POINTTYPE | WKBMOFFSET;

			if (!state->config->hwgeom)
			{
				state->pgtype = "POINTM";
				state->pgdims = 3;
				state->istypeM = 1;
			}
			else
			{
				state->pgtype = "POINT";
				state->pgdims = 2;
			}
			break;

		case SHPT_ARCM:
			/* PolyLineM */
			state->wkbtype = MULTILINETYPE | WKBMOFFSET;

			if (!state->config->hwgeom)
			{
				state->pgtype = "MULTILINESTRINGM";
				state->pgdims = 3;
				state->istypeM = 1;
			}
			else
			{
				state->pgtype = "MULTILINESTRING";
				state->pgdims = 2;
			}
			break;

		case SHPT_POLYGONM:
			/* PolygonM */
			state->wkbtype = MULTIPOLYGONTYPE | WKBMOFFSET;

			if (!state->config->hwgeom)
			{
				state->pgtype = "MULTIPOLYGONM";
				state->pgdims = 3;
				state->istypeM = 1;
			}
			else
			{
				state->pgtype = "MULTIPOLYGON";
				state->pgdims = 2;
			}
			break;

		case SHPT_MULTIPOINTM:
			/* MultiPointM */
			state->wkbtype = MULTIPOINTTYPE | WKBMOFFSET;

			if (!state->config->hwgeom)
			{
				state->pgtype = "MULTIPOINTM";
				state->pgdims = 3;
				state->istypeM = 1;
			}
			else
			{
				state->pgtype = "MULTIPOINT";
				state->pgdims = 2;
			}
			break;

		case SHPT_POINTZ:
			/* PointZ */
			state->wkbtype = POINTTYPE | WKBMOFFSET | WKBZOFFSET;
			state->pgtype = "POINT";

			if (!state->config->hwgeom)
				state->pgdims = 4;
			else
				state->pgdims = 3;

			break;

		case SHPT_ARCZ:
			/* PolyLineZ */
			state->pgtype = "MULTILINESTRING";
			state->wkbtype = MULTILINETYPE | WKBZOFFSET | WKBMOFFSET;

			if (!state->config->hwgeom)
				state->pgdims = 4;
			else
				state->pgdims = 3;

			break;

		case SHPT_POLYGONZ:
			/* MultiPolygonZ */
			state->pgtype = "MULTIPOLYGON";
			state->wkbtype = MULTIPOLYGONTYPE | WKBZOFFSET | WKBMOFFSET;

			if (!state->config->hwgeom)
				state->pgdims = 4;
			else
				state->pgdims = 3;

			break;

		case SHPT_MULTIPOINTZ:
			/* MultiPointZ */
			state->pgtype = "MULTIPOINT";
			state->wkbtype = MULTIPOINTTYPE | WKBZOFFSET | WKBMOFFSET;

			if (!state->config->hwgeom)
				state->pgdims = 4;
			else
				state->pgdims = 3;

			break;

		default:
			state->pgtype = "GEOMETRY";
			state->wkbtype = COLLECTIONTYPE | WKBZOFFSET | WKBMOFFSET;
			state->pgdims = 4;

			snprintf(state->message, SHPLOADERMSGLEN, _("Unknown geometry type: %d\n"), state->shpfiletype);
			return SHPLOADERERR;

			break;
		}

		/* If in simple geometry mode, alter names for CREATE TABLE by skipping MULTI */
		if (state->config->simple_geometries)
		{
			if ((state->wkbtype & 0x7) == MULTIPOLYGONTYPE)
				state->pgtype += 5;

			if ((state->wkbtype & 0x7) == MULTILINETYPE)
				state->pgtype += 5;
		}

	}
	else
	{
		/* Otherwise just count the number of records in the DBF */
		state->num_entities = DBFGetRecordCount(state->hDBFHandle);
	}


	/* Get the field information from the DBF */
	state->num_fields = DBFGetFieldCount(state->hDBFHandle);
	state->num_records = DBFGetRecordCount(state->hDBFHandle);

	/* Allocate storage for field information */
	state->field_names = malloc(state->num_fields * sizeof(char*));
	state->types = (DBFFieldType *)malloc(state->num_fields * sizeof(int));
	state->widths = malloc(state->num_fields * sizeof(int));
	state->precisions = malloc(state->num_fields * sizeof(int));
	state->col_names = malloc((state->num_fields + 2) * sizeof(char) * MAXFIELDNAMELEN);

	/* Generate a string of comma separated column names of the form "(col1, col2 ... colN)" for the SQL
	   insertion string */
	strcpy(state->col_names, "(" );

	for (j = 0; j < state->num_fields; j++)
	{
		type = DBFGetFieldInfo(state->hDBFHandle, j, name, &field_width, &field_precision);

		state->types[j] = type;
		state->widths[j] = field_width;
		state->precisions[j] = field_precision;

		if (state->config->encoding)
		{
			char *encoding_msg = _("Try \"LATIN1\" (Western European), or one of the values described at http://www.postgresql.org/docs/current/static/multibyte.html.");

			int rv = utf8(state->config->encoding, name, &utf8str);

			if (rv != UTF8_GOOD_RESULT)
			{
				if ( rv == UTF8_BAD_RESULT )
					snprintf(state->message, SHPLOADERMSGLEN, _("Unable to convert field name \"%s\" to UTF-8 (iconv reports \"%s\"). Current encoding is \"%s\". %s"), utf8str, strerror(errno), state->config->encoding, encoding_msg);
				else if ( rv == UTF8_NO_RESULT )
					snprintf(state->message, SHPLOADERMSGLEN, _("Unable to convert field name to UTF-8 (iconv reports \"%s\"). Current encoding is \"%s\". %s"), strerror(errno), state->config->encoding, encoding_msg);
				else
					snprintf(state->message, SHPLOADERMSGLEN, _("Unexpected return value from utf8()"));

				if ( rv == UTF8_BAD_RESULT )
					free(utf8str);

				return SHPLOADERERR;
			}

			strncpy(name, utf8str, MAXFIELDNAMELEN);
			free(utf8str);
		}

		/*
		 * Make field names lowercase unless asked to
		 * keep identifiers case.
		 */
		if (!state->config->quoteidentifiers)
			strtolower(name);

		/*
		 * Escape names starting with the
		 * escape char (_), those named 'gid'
		 * or after pgsql reserved attribute names
		 */
		if (name[0] == '_' ||
		        ! strcmp(name, "gid") || ! strcmp(name, "tableoid") ||
		        ! strcmp(name, "cmin") ||
		        ! strcmp(name, "cmax") ||
		        ! strcmp(name, "xmin") ||
		        ! strcmp(name, "xmax") ||
		        ! strcmp(name, "primary") ||
		        ! strcmp(name, "oid") || ! strcmp(name, "ctid"))
		{
			strncpy(name2 + 2, name, MAXFIELDNAMELEN - 2);
			name2[0] = '_';
			name2[1] = '_';
			strcpy(name, name2);
		}

		/* Avoid duplicating field names */
		for (z = 0; z < j ; z++)
		{
			if (strcmp(state->field_names[z], name) == 0)
			{
				strncat(name, "__", MAXFIELDNAMELEN);
				snprintf(name + strlen(name), MAXFIELDNAMELEN, "%i", j);
				break;
			}
		}

		state->field_names[j] = malloc(strlen(name) + 1);
		strcpy(state->field_names[j], name);

		strcat(state->col_names, "\"");
		strcat(state->col_names, name);

		if (state->config->readshape == 1 || j < (state->num_fields - 1))
		{
			/* Don't include last comma if its the last field and no geometry field will follow */
			strcat(state->col_names, "\",");
		}
		else
		{
			strcat(state->col_names, "\"");
		}
	}

	/* Append the geometry column if required */
	if (state->config->readshape == 1)
		strcat(state->col_names, state->config->geom);

	strcat(state->col_names, ")");


	/* Return status */
	return ret;
}

/* Return a pointer to an allocated string containing the header for the specified loader state */
int
ShpLoaderGetSQLHeader(SHPLOADERSTATE *state, char **strheader)
{
	stringbuffer_t *sb;
	char *ret;
	int j;

	/* Create the stringbuffer containing the header; we use this API as it's easier
	   for handling string resizing during append */
	sb = stringbuffer_create();
	stringbuffer_clear(sb);

	/* Set the client encoding if required */
	if (state->config->encoding)
	{
		stringbuffer_aprintf(sb, "SET CLIENT_ENCODING TO UTF8;\n");
	}

	/* Use SQL-standard string escaping rather than PostgreSQL standard */
	stringbuffer_aprintf(sb, "SET STANDARD_CONFORMING_STRINGS TO ON;\n");

	/* Drop table if requested */
	if (state->config->opt == 'd')
	{
		/**
		 * TODO: if the table has more then one geometry column
		 * 	the DROP TABLE call will leave spurious records in
		 * 	geometry_columns.
		 *
		 * If the geometry column in the table being dropped
		 * does not match 'the_geom' or the name specified with
		 * -g an error is returned by DropGeometryColumn.
		 *
		 * The table to be dropped might not exist.
		 */
		if (state->config->schema)
		{
			if (state->config->readshape == 1 && (! state->config->geography) )
			{
				stringbuffer_aprintf(sb, "SELECT DropGeometryColumn('%s','%s','%s');\n",
				                     state->config->schema, state->config->table, state->config->geom);
			}

			stringbuffer_aprintf(sb, "DROP TABLE \"%s\".\"%s\";\n", state->config->schema,
			                     state->config->table);
		}
		else
		{
			if (state->config->readshape == 1  && (! state->config->geography) )
			{
				stringbuffer_aprintf(sb, "SELECT DropGeometryColumn('','%s','%s');\n",
				                     state->config->table, state->config->geom);
			}

			stringbuffer_aprintf(sb, "DROP TABLE \"%s\";\n", state->config->table);
		}
	}

	/* Start of transaction */
	stringbuffer_aprintf(sb, "BEGIN;\n");

	/* If not in 'append' mode create the spatial table */
	if (state->config->opt != 'a')
	{
		/*
		* Create a table for inserting the shapes into with appropriate
		* columns and types
		*/
		if (state->config->schema)
		{
			stringbuffer_aprintf(sb, "CREATE TABLE \"%s\".\"%s\" (gid serial PRIMARY KEY",
			                     state->config->schema, state->config->table);
		}
		else
		{
			stringbuffer_aprintf(sb, "CREATE TABLE \"%s\" (gid serial PRIMARY KEY", state->config->table);
		}

		/* Generate the field types based upon the shapefile information */
		for (j = 0; j < state->num_fields; j++)
		{
			stringbuffer_aprintf(sb, ",\n\"%s\" ", state->field_names[j]);

			switch (state->types[j])
			{
			case FTString:
				/* use DBF attribute size as maximum width */
				stringbuffer_aprintf(sb, "varchar(%d)", state->widths[j]);
				break;

			case FTDate:
				stringbuffer_aprintf(sb, "date");
				break;

			case FTInteger:
				/* Determine exact type based upon field width */
				if (state->config->forceint4)
				{
					stringbuffer_aprintf(sb, "int4");
				}
				else if (state->widths[j] < 5)
				{
					stringbuffer_aprintf(sb, "int2");
				}
				else if (state->widths[j] < 10)
				{
					stringbuffer_aprintf(sb, "int4");
				}
				else
				{
					stringbuffer_aprintf(sb, "numeric(%d,0)", state->widths[j]);
				}
				break;

			case FTDouble:
				/* Determine exact type based upon field width */
				if (state->widths[j] > 18)
				{
					stringbuffer_aprintf(sb, "numeric");
				}
				else
				{
					stringbuffer_aprintf(sb, "float8");
				}
				break;

			case FTLogical:
				stringbuffer_aprintf(sb, "boolean");
				break;

			default:
				snprintf(state->message, SHPLOADERMSGLEN, _("Invalid type %x in DBF file"), state->types[j]);
				stringbuffer_destroy(sb);
				return SHPLOADERERR;
			}
		}

		/* Add the geography column directly to the table definition, we don't
		   need to do an AddGeometryColumn() call. */
		if (state->config->readshape == 1 && state->config->geography)
		{
			char *dimschar;
			if ( state->pgdims == 4 )
				dimschar = "ZM";
			else
				dimschar = "";
			if (state->config->sr_id != SRID_UNKNOWN && state->config->sr_id != 4326)
			{
				snprintf(state->message, SHPLOADERMSGLEN, _("Invalid SRID for geography type: %x"), state->config->sr_id);
				stringbuffer_destroy(sb);
				return SHPLOADERERR;
			}
			stringbuffer_aprintf(sb, ",\n\"%s\" geography(%s%s,%d)", state->config->geom, state->pgtype, dimschar, 4326);
		}

		stringbuffer_aprintf(sb, ");\n");

		/* Create the geometry column with an addgeometry call */
		if (state->config->readshape == 1 && (!state->config->geography))
		{
			if (state->config->schema)
			{
				stringbuffer_aprintf(sb, "SELECT AddGeometryColumn('%s','%s','%s','%d',",
				                     state->config->schema, state->config->table, state->config->geom, state->config->sr_id);
			}
			else
			{
				stringbuffer_aprintf(sb, "SELECT AddGeometryColumn('','%s','%s','%d',",
				                     state->config->table, state->config->geom, state->config->sr_id);
			}

			stringbuffer_aprintf(sb, "'%s',%d);\n", state->pgtype, state->pgdims);
		}
	}

	/* Copy the string buffer into a new string, destroying the string buffer */
	ret = (char *)malloc(strlen((char *)stringbuffer_getstring(sb)) + 1);
	strcpy(ret, (char *)stringbuffer_getstring(sb));
	stringbuffer_destroy(sb);

	*strheader = ret;

	return SHPLOADEROK;
}


/* Return an allocated string containing the copy statement for this state */
int
ShpLoaderGetSQLCopyStatement(SHPLOADERSTATE *state, char **strheader)
{
	char *copystr;

	/* Allocate the string for the COPY statement */
	if (state->config->dump_format)
	{
		if (state->config->schema)
		{
			copystr = malloc(strlen(state->config->schema) + strlen(state->config->table) +
			                 strlen(state->col_names) + 40);

			sprintf(copystr, "COPY \"%s\".\"%s\" %s FROM stdin;\n",
			        state->config->schema, state->config->table, state->col_names);
		}
		else
		{
			copystr = malloc(strlen(state->config->table) + strlen(state->col_names) + 40);

			sprintf(copystr, "COPY \"%s\" %s FROM stdin;\n", state->config->table, state->col_names);
		}

		*strheader = copystr;
		return SHPLOADEROK;
	}
	else
	{
		/* Flag an error as something has gone horribly wrong */
		snprintf(state->message, SHPLOADERMSGLEN, _("Internal error: attempt to generate a COPY statement for data that hasn't been requested in COPY format"));

		return SHPLOADERERR;
	}
}


/* Return a count of the number of entities in this shapefile */
int
ShpLoaderGetRecordCount(SHPLOADERSTATE *state)
{
	return state->num_entities;
}


/* Return an allocated string representation of a specified record item */
int
ShpLoaderGenerateSQLRowStatement(SHPLOADERSTATE *state, int item, char **strrecord)
{
	SHPObject *obj = NULL;
	stringbuffer_t *sb;
	stringbuffer_t *sbwarn;
	char val[MAXVALUELEN];
	char *escval;
	char *geometry=NULL, *ret;
	char *utf8str;
	int res, i;
	int rv;

	/* Clear the stringbuffers */
	sbwarn = stringbuffer_create();
	stringbuffer_clear(sbwarn);
	sb = stringbuffer_create();
	stringbuffer_clear(sb);

	/* If we are reading the DBF only and the record has been marked deleted, return deleted record status */
	if (state->config->readshape == 0 && DBFReadDeleted(state->hDBFHandle, item))
	{
		*strrecord = NULL;
		return SHPLOADERRECDELETED;
	}

	/* If we are reading the shapefile, open the specified record */
	if (state->config->readshape == 1)
	{
		obj = SHPReadObject(state->hSHPHandle, item);
		if (!obj)
		{
			snprintf(state->message, SHPLOADERMSGLEN, _("Error reading shape object %d"), item);
			return SHPLOADERERR;
		}

		/* If we are set to skip NULLs, return a NULL record status */
		if (state->config->null_policy == POLICY_NULL_SKIP && obj->nVertices == 0 )
		{
			SHPDestroyObject(obj);

			*strrecord = NULL;
			return SHPLOADERRECISNULL;
		}
	}

	/* If not in dump format, generate the INSERT string */
	if (!state->config->dump_format)
	{
		if (state->config->schema)
		{
			stringbuffer_aprintf(sb, "INSERT INTO \"%s\".\"%s\" %s VALUES (", state->config->schema,
			                     state->config->table, state->col_names);
		}
		else
		{
			stringbuffer_aprintf(sb, "INSERT INTO \"%s\" %s VALUES (", state->config->table,
			                     state->col_names);
		}
	}


	/* Read all of the attributes from the DBF file for this item */
	for (i = 0; i < DBFGetFieldCount(state->hDBFHandle); i++)
	{
		/* Special case for NULL attributes */
		if (DBFIsAttributeNULL(state->hDBFHandle, item, i))
		{
			if (state->config->dump_format)
				stringbuffer_aprintf(sb, "\\N");
			else
				stringbuffer_aprintf(sb, "NULL");
		}
		else
		{
			/* Attribute NOT NULL */
			switch (state->types[i])
			{
			case FTInteger:
			case FTDouble:
				rv = snprintf(val, MAXVALUELEN, "%s", DBFReadStringAttribute(state->hDBFHandle, item, i));
				if (rv >= MAXVALUELEN || rv == -1)
				{
					stringbuffer_aprintf(sbwarn, "Warning: field %d name truncated\n", i);
					val[MAXVALUELEN - 1] = '\0';
				}

				/* If the value is an empty string, change to 0 */
				if (val[0] == '\0')
				{
					val[0] = '0';
					val[1] = '\0';
				}

				/* If the value ends with just ".", remove the dot */
				if (val[strlen(val) - 1] == '.')
					val[strlen(val) - 1] = '\0';
				break;

			case FTString:
			case FTLogical:
			case FTDate:
				rv = snprintf(val, MAXVALUELEN, "%s", DBFReadStringAttribute(state->hDBFHandle, item, i));
				if (rv >= MAXVALUELEN || rv == -1)
				{
					stringbuffer_aprintf(sbwarn, "Warning: field %d name truncated\n", i);
					val[MAXVALUELEN - 1] = '\0';
				}
				break;

			default:
				snprintf(state->message, SHPLOADERMSGLEN, _("Error: field %d has invalid or unknown field type (%d)"), i, state->types[i]);

				SHPDestroyObject(obj);
				stringbuffer_destroy(sbwarn);
				stringbuffer_destroy(sb);

				return SHPLOADERERR;
			}

			if (state->config->encoding)
			{
				char *encoding_msg = _("Try \"LATIN1\" (Western European), or one of the values described at http://www.postgresql.org/docs/current/static/multibyte.html.");

				rv = utf8(state->config->encoding, val, &utf8str);

				if (rv != UTF8_GOOD_RESULT)
				{
					if ( rv == UTF8_BAD_RESULT )
						snprintf(state->message, SHPLOADERMSGLEN, _("Unable to convert data value \"%s\" to UTF-8 (iconv reports \"%s\"). Current encoding is \"%s\". %s"), utf8str, strerror(errno), state->config->encoding, encoding_msg);
					else if ( rv == UTF8_NO_RESULT )
						snprintf(state->message, SHPLOADERMSGLEN, _("Unable to convert data value to UTF-8 (iconv reports \"%s\"). Current encoding is \"%s\". %s"), strerror(errno), state->config->encoding, encoding_msg);
					else
						snprintf(state->message, SHPLOADERMSGLEN, _("Unexpected return value from utf8()"));

					if ( rv == UTF8_BAD_RESULT )
						free(utf8str);

					return SHPLOADERERR;
				}
			}

			/* Escape attribute correctly according to dump format */
			if (state->config->dump_format)
			{
				escval = escape_copy_string(val);
				stringbuffer_aprintf(sb, "%s", escval);
			}
			else
			{
				escval = escape_insert_string(val);
				stringbuffer_aprintf(sb, "'%s'", escval);
			}

			/* Free the escaped version if required */
			if (val != escval)
				free(escval);
		}

		/* Only put in delimeter if not last field or a shape will follow */
		if (state->config->readshape == 1 || i < DBFGetFieldCount(state->hDBFHandle) - 1)
		{
			if (state->config->dump_format)
				stringbuffer_aprintf(sb, "\t");
			else
				stringbuffer_aprintf(sb, ",");
		}

		/* End of DBF attribute loop */
	}


	/* Add the shape attribute if we are reading it */
	if (state->config->readshape == 1)
	{
		/* Handle the case of a NULL shape */
		if (obj->nVertices == 0)
		{
			if (state->config->dump_format)
				stringbuffer_aprintf(sb, "\\N");
			else
				stringbuffer_aprintf(sb, "NULL");
		}
		else
		{
			/* Handle all other shape attributes */
			switch (obj->nSHPType)
			{
			case SHPT_POLYGON:
			case SHPT_POLYGONM:
			case SHPT_POLYGONZ:
				res = GeneratePolygonGeometry(state, obj, &geometry);
				if (res != SHPLOADEROK)
				{
					/* Error message has already been set */
					SHPDestroyObject(obj);
					stringbuffer_destroy(sbwarn);
					stringbuffer_destroy(sb);

					return SHPLOADERERR;
				}
				break;

			case SHPT_POINT:
			case SHPT_POINTM:
			case SHPT_POINTZ:
			case SHPT_MULTIPOINT:
			case SHPT_MULTIPOINTM:
			case SHPT_MULTIPOINTZ:
				res = GeneratePointGeometry(state, obj, &geometry);
				if (res != SHPLOADEROK)
				{
					/* Error message has already been set */
					SHPDestroyObject(obj);
					stringbuffer_destroy(sbwarn);
					stringbuffer_destroy(sb);

					return SHPLOADERERR;
				}
				break;

			case SHPT_ARC:
			case SHPT_ARCM:
			case SHPT_ARCZ:
				res = GenerateLineStringGeometry(state, obj, &geometry);
				if (res != SHPLOADEROK)
				{
					/* Error message has already been set */
					SHPDestroyObject(obj);
					stringbuffer_destroy(sbwarn);
					stringbuffer_destroy(sb);

					return SHPLOADERERR;
				}
				break;

			default:
				snprintf(state->message, SHPLOADERMSGLEN, _("Shape type is not supported, type id = %d"), obj->nSHPType);

				SHPDestroyObject(obj);
				stringbuffer_destroy(sbwarn);
				stringbuffer_destroy(sb);

				return SHPLOADERERR;
			}


			/* Now generate the geometry string according to the current configuration */
			if (state->config->hwgeom)
			{
				/* Old-style hwgeom (WKT) */
				if (!state->config->dump_format)
					stringbuffer_aprintf(sb, "GeomFromText('");
				else
				{
					/* Output SRID if relevant */
					if (state->config->sr_id != 0)
						stringbuffer_aprintf(sb, "SRID=%d;", state->config->sr_id);
				}

				stringbuffer_aprintf(sb, "%s", geometry);

				if (!state->config->dump_format)
				{
					stringbuffer_aprintf(sb, "'");

					/* Output SRID if relevant */
					if (state->config->sr_id != 0)
						stringbuffer_aprintf(sb, ", %d)", state->config->sr_id);
					else
						stringbuffer_aprintf(sb, ")");
				}
			}
			else
			{
				/* New style lwgeom (HEXEWKB) */
				if (!state->config->dump_format)
					stringbuffer_aprintf(sb, "'");

				stringbuffer_aprintf(sb, "%s", geometry);

				if (!state->config->dump_format)
					stringbuffer_aprintf(sb, "'");
			}

			free(geometry);
		}

		/* Tidy up everything */
		SHPDestroyObject(obj);
	}

	/* Close the line correctly for dump/insert format */
	if (!state->config->dump_format)
		stringbuffer_aprintf(sb, ");");


	/* Copy the string buffer into a new string, destroying the string buffer */
	ret = (char *)malloc(strlen((char *)stringbuffer_getstring(sb)) + 1);
	strcpy(ret, (char *)stringbuffer_getstring(sb));
	stringbuffer_destroy(sb);

	*strrecord = ret;

	/* If any warnings occurred, set the returned message string and warning status */
	if (strlen((char *)stringbuffer_getstring(sbwarn)) > 0)
	{
		snprintf(state->message, SHPLOADERMSGLEN, "%s", stringbuffer_getstring(sbwarn));
		stringbuffer_destroy(sbwarn);

		return SHPLOADERWARN;
	}
	else
	{
		/* Everything went okay */
		stringbuffer_destroy(sbwarn);

		return SHPLOADEROK;
	}
}


/* Return a pointer to an allocated string containing the header for the specified loader state */
int
ShpLoaderGetSQLFooter(SHPLOADERSTATE *state, char **strfooter)
{
	stringbuffer_t *sb;
	char *ret;
	char *ops;

	if ( state->config->geography )
		ops = "gist_geography_ops";
	else
		ops = "gist_geometry_ops";

	/* Create the stringbuffer containing the header; we use this API as it's easier
	   for handling string resizing during append */
	sb = stringbuffer_create();
	stringbuffer_clear(sb);

	/* Create gist index if specified and not in "prepare" mode */
	if (state->config->createindex)
	{
		if (state->config->schema)
		{
			stringbuffer_aprintf(sb, "CREATE INDEX \"%s_%s_gist\" ON \"%s\".\"%s\" using gist (\"%s\" %s);\n", state->config->table, state->config->geom,
			                     state->config->schema, state->config->table, state->config->geom, ops);
		}
		else
		{
			stringbuffer_aprintf(sb, "CREATE INDEX \"%s_%s_gist\" ON \"%s\" using gist (\"%s\" %s);\n", state->config->table, state->config->geom, state->config->table, state->config->geom, ops);
		}
	}

	/* End the transaction */
	stringbuffer_aprintf(sb, "COMMIT;\n");

	/* Copy the string buffer into a new string, destroying the string buffer */
	ret = (char *)malloc(strlen((char *)stringbuffer_getstring(sb)) + 1);
	strcpy(ret, (char *)stringbuffer_getstring(sb));
	stringbuffer_destroy(sb);

	*strfooter = ret;

	return SHPLOADEROK;
}


void
ShpLoaderDestroy(SHPLOADERSTATE *state)
{
	/* Destroy a state object created with ShpLoaderOpenShape */

	if (state != NULL)
	{
		if (state->hSHPHandle)
			SHPClose(state->hSHPHandle);
		if (state->hDBFHandle)
			DBFClose(state->hDBFHandle);
		if (state->field_names)
		{
			int i;
			for (i = 0; i < state->num_fields; i++)
				free(state->field_names[i]);

			free(state->field_names);
		}
		if (state->types)
			free(state->types);
		if (state->widths)
			free(state->widths);
		if (state->precisions)
			free(state->precisions);
		if (state->col_names)
			free(state->col_names);

		/* Free the state itself */
		free(state);
	}
}



