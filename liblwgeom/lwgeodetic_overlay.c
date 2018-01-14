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
 * Copyright 2017 Danny Götte <danny.goette@fem.tu-ilmenau.de>
 *
 **********************************************************************/

#include "lwgeodetic_overlay.h"


typedef struct _overlay_point {
	double x;
	double y;

	struct _overlay_point* next;
	struct _overlay_point* prev;

	struct _overlay_point* nextPoly;	/* next poly */
	struct _overlay_point* nextHole;	/* next ring */

	struct _overlay_point* neighbor;	/* related vertex in next polygon */
	char intersection;			/* boolean, is intersection point */
	char entry;					/* boolean, next entry in clockwise order */
	char visited;				/* boolean, is visited */

	float sort;					/* sort indicator for intersection points on an edge */


} OVERLAY_POINT; /* overlay vertex */



#define FOREACH_OVERLAY_POINT(point, start) 	point = start; do {
#define ENDFOREACH_OVERLAY_POINT(point, start, next) point = next; } while (point != start);


const int COMBINE_DIFFERENCE = 1;
const int COMBINE_SYMDIFFERENCE = 2;
const int COMBINE_INTERSECTION = 3;
const int COMBINE_UNION = 4;


int lwmpoly_assign_hole(LWMPOLY* target, POINTARRAY* source);
void overlay_add_holes_lwmpoly(LWMPOLY* target, OVERLAY_POINT* first_hole, int useUnvistedHoles);
LWGEOM *lwgeodetic_overlay(const LWMPOLY *lwgeom1, const LWMPOLY *lwgeom2, const int combination);

LWGEOM*
lwmpoly_as_simple_lwgeom(LWMPOLY* complex) {
	if (complex->ngeoms != 1) {
		return lwmpoly_as_lwgeom(complex);
	}

	LWPOLY* poly = lwpoly_clone(complex->geoms[0]);
	lwfree(complex);

	return lwpoly_as_lwgeom(poly);
}

LWGEOM *
lwgeodetic_difference(const LWGEOM *geom1, const LWGEOM *geom2) {
	LWGEOM *result = NULL;
	int srid;

	/* A.SymDifference(Empty) == A */
	if (lwgeom_is_empty(geom2))
		return lwgeom_clone_deep(geom1);

	/* Empty.DymDifference(B) == B */
	if (lwgeom_is_empty(geom1))
		return lwgeom_clone_deep(geom2);

	/* ensure srids are identical */
	srid = (int) (geom1->srid);
	error_if_srid_mismatch(srid, (int) (geom2->srid));

	int type1 = lwgeom_get_type(geom1);
	int type2 = lwgeom_get_type(geom2);

	if (type1 == MULTIPOLYGONTYPE && type2 == MULTIPOLYGONTYPE) {
		fprintf(stderr, "yes\n\n");
		result = lwgeodetic_overlay(lwgeom_as_lwmpoly(geom1), lwgeom_as_lwmpoly(geom2), COMBINE_DIFFERENCE);
	} else if (type1 == POLYGONTYPE) {

		fprintf(stderr, "part1\n\n");
		LWMPOLY* mpoly1 = lwmpoly_construct_empty(srid, 0, 0);
		lwmpoly_add_lwpoly(mpoly1, lwgeom_as_lwpoly(geom1));

		result = lwgeodetic_difference(lwmpoly_as_lwgeom(mpoly1), geom2);

		lwmpoly_free(mpoly1);
	} else if (type2 == POLYGONTYPE) {
		fprintf(stderr, "part2\n\n");
		LWMPOLY* mpoly2 = lwmpoly_construct_empty(srid, 0, 0);
		lwmpoly_add_lwpoly(mpoly2, lwgeom_as_lwpoly(geom2));

		result = lwgeodetic_difference(geom1, lwmpoly_as_lwgeom(mpoly2));

		lwmpoly_free(mpoly2);
	} else {
		fprintf(stderr, "nope\n\n");
//		lwerror("Unsupported geometry types 1");
		return lwgeom_construct_empty(POLYGONTYPE, srid, 0, 0);
	}

	return result;
}

LWGEOM *
lwgeodetic_symdifference(const LWGEOM *geom1, const LWGEOM *geom2) {
	LWGEOM *result = NULL;
	int srid;

	/* A.SymDifference(Empty) == A */
	if (lwgeom_is_empty(geom2))
		return lwgeom_clone_deep(geom1);

	/* Empty.DymDifference(B) == B */
	if (lwgeom_is_empty(geom1))
		return lwgeom_clone_deep(geom2);

	/* ensure srids are identical */
	srid = (int) (geom1->srid);
	error_if_srid_mismatch(srid, (int) (geom2->srid));

	int type1 = lwgeom_get_type(geom1);
	int type2 = lwgeom_get_type(geom2);


	if (type1 == MULTIPOLYGONTYPE && type2 == MULTIPOLYGONTYPE) {
		fprintf(stderr, "yes\n\n");
		LWGEOM* diff1 = lwgeodetic_overlay(lwgeom_as_lwmpoly(geom1), lwgeom_as_lwmpoly(geom2), COMBINE_DIFFERENCE);
		LWGEOM* diff2 = lwgeodetic_overlay(lwgeom_as_lwmpoly(geom2), lwgeom_as_lwmpoly(geom1), COMBINE_DIFFERENCE);

		return lwgeodetic_union(diff1, diff2);
	} else if (type1 == POLYGONTYPE) {

		fprintf(stderr, "part1\n\n");
		LWMPOLY* mpoly1 = lwmpoly_construct_empty(srid, 0, 0);
		lwmpoly_add_lwpoly(mpoly1, lwgeom_as_lwpoly(geom1));

		result = lwgeodetic_symdifference(lwmpoly_as_lwgeom(mpoly1), geom2);

		lwmpoly_free(mpoly1);
	} else if (type2 == POLYGONTYPE) {
		fprintf(stderr, "part2\n\n");
		LWMPOLY* mpoly2 = lwmpoly_construct_empty(srid, 0, 0);
		lwmpoly_add_lwpoly(mpoly2, lwgeom_as_lwpoly(geom2));

		result = lwgeodetic_symdifference(geom1, lwmpoly_as_lwgeom(mpoly2));

		lwmpoly_free(mpoly2);
	} else {
		fprintf(stderr, "nope\n\n");
//		lwerror("Unsupported geometry types 1");
		return lwgeom_construct_empty(POLYGONTYPE, srid, 0, 0);
	}

	return result;
}

int
lwgeoms_related(LWGEOM* geom1, LWGEOM* geom2)
{
	POINT3D point;
	GBOX gbox1, gbox2;

	/* get a gbox for geom1 */
	if ( geom1->bbox ) gbox1 = *(geom1->bbox);
	else lwgeom_calculate_gbox_geodetic(geom1, &gbox1);

	/* get a gbox for geom2 */
	if ( geom2->bbox ) gbox2 = *(geom2->bbox);
	else lwgeom_calculate_gbox_geodetic(geom2, &gbox2);

	/* overlapping boundaries? */
	if (gbox_overlaps(&gbox1, &gbox2)) {
		fprintf(stderr, "overlaps bbox\n");
		return LW_TRUE;
	}

	/* 2 inside 1 ? */
	point.x = geom2->bbox->xmin;
	point.y = geom2->bbox->ymin;
	point.z = geom2->bbox->zmin;
	if (gbox_contains_point3d(&gbox1, &point)) {
		fprintf(stderr, "overlaps 2 in 1\n");
		return LW_TRUE;
	}

	/* 1 inside 2 ? */
	point.x = geom1->bbox->xmin;
	point.y = geom1->bbox->ymin;
	point.z = geom1->bbox->zmin;
	if (gbox_contains_point3d(&gbox2, &point)) {
		fprintf(stderr, "overlaps 1 in 2\n");
		return LW_TRUE;
	}

	/* not related */
	return LW_FALSE;
}

LWGEOM *
lwgeodetic_union(const LWGEOM *geom1, const LWGEOM *geom2) {
	LWGEOM *result = NULL;
	int srid;
	int i, j;

	/* A.SymDifference(Empty) == A */
	if (lwgeom_is_empty(geom2))
		return lwgeom_clone_deep(geom1);

	/* Empty.DymDifference(B) == B */
	if (lwgeom_is_empty(geom1))
		return lwgeom_clone_deep(geom2);

	/* ensure srids are identical */
	srid = (int) (geom1->srid);
	error_if_srid_mismatch(srid, (int) (geom2->srid));

	int type1 = lwgeom_get_type(geom1);
	int type2 = lwgeom_get_type(geom2);

	if (type1 == MULTIPOLYGONTYPE && type2 == MULTIPOLYGONTYPE) {
		fprintf(stderr, "yes\n\n");
		result = lwgeodetic_overlay(lwgeom_as_lwmpoly(geom1), lwgeom_as_lwmpoly(geom2), COMBINE_UNION);
	} else if (type1 == POLYGONTYPE) {

		fprintf(stderr, "part1\n\n");
		LWMPOLY* mpoly1 = lwmpoly_construct_empty(srid, 0, 0);
		lwmpoly_add_lwpoly(mpoly1, lwgeom_as_lwpoly(geom1));

		result = lwgeodetic_union(lwmpoly_as_lwgeom(mpoly1), geom2);

		lwmpoly_free(mpoly1);
	} else if (type2 == POLYGONTYPE) {
		fprintf(stderr, "part2\n\n");
		LWMPOLY* mpoly2 = lwmpoly_construct_empty(srid, 0, 0);
		lwmpoly_add_lwpoly(mpoly2, lwgeom_as_lwpoly(geom2));

		result = lwgeodetic_union(geom1, lwmpoly_as_lwgeom(mpoly2));

		lwmpoly_free(mpoly2);
	} else {
		fprintf(stderr, "nope\n\n");
//		lwerror("Unsupported geometry types 1");
		return lwgeom_construct_empty(POLYGONTYPE, srid, 0, 0);
	}

	return result;
}

LWGEOM *
lwgeodetic_intersection(const LWGEOM *geom1, const LWGEOM *geom2) {
	LWGEOM *result = NULL;
	int srid;
	int i, j;
	fprintf(stderr, "int1\n");

	/* A.SymDifference(Empty) == A */
	if (lwgeom_is_empty(geom2))
		return lwgeom_clone_deep(geom1);

	fprintf(stderr, "int2\n");

	/* Empty.DymDifference(B) == B */
	if (lwgeom_is_empty(geom1))
		return lwgeom_clone_deep(geom2);

	fprintf(stderr, "int3\n");

	/* ensure srids are identical */
	srid = (int) (geom1->srid);
	error_if_srid_mismatch(srid, (int) (geom2->srid));

	fprintf(stderr, "int4\n");

	int type1 = lwgeom_get_type(geom1);
	int type2 = lwgeom_get_type(geom2);
	fprintf(stderr, "int5 %d - %d\n", type1, type2);

	if (type1 == MULTIPOLYGONTYPE && type2 == MULTIPOLYGONTYPE) {
		fprintf(stderr, "yes\n\n");

//		if (!lwgeoms_related(geom1, geom2)) {
//			fprintf(stderr, "yes3\n\n");
//			return lwcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
//		}
		fprintf(stderr, "yes2\n\n");

		result = lwgeodetic_overlay(lwgeom_as_lwmpoly(geom1), lwgeom_as_lwmpoly(geom2), COMBINE_INTERSECTION);
	} else if (type1 == POLYGONTYPE) {

		fprintf(stderr, "part1\n\n");
		LWMPOLY* mpoly1 = lwmpoly_construct_empty(srid, 0, 0);
		fprintf(stderr, "part1\n\n");
		lwmpoly_add_lwpoly(mpoly1, lwgeom_as_lwpoly(geom1));

		fprintf(stderr, "part1\n\n");
		result = lwgeodetic_intersection(lwmpoly_as_lwgeom(mpoly1), geom2);

		fprintf(stderr, "part1\n\n");
		lwmpoly_free(mpoly1);
	} else if (type2 == POLYGONTYPE) {
		fprintf(stderr, "part2\n\n");
		LWMPOLY* mpoly2 = lwmpoly_construct_empty(srid, 0, 0);
		lwmpoly_add_lwpoly(mpoly2, lwgeom_as_lwpoly(geom2));

		result = lwgeodetic_intersection(geom1, lwmpoly_as_lwgeom(mpoly2));

		lwmpoly_free(mpoly2);
	} else {
		fprintf(stderr, "nope\n\n");
//		lwerror("Unsupported geometry types 1");
		return lwgeom_construct_empty(POLYGONTYPE, srid, 0, 0);
	}


	return result;
}

/**
 * construct a new overlay point
 */
OVERLAY_POINT*
overlay_create_point(const double x, const double y)
{
//	fprintf(stderr, "Create Vertex: %.2f %.2f\n", x, y);
	OVERLAY_POINT* vertex = lwalloc(sizeof(OVERLAY_POINT));
	vertex->x = x;
	vertex->y = y;
	vertex->next = NULL;
	vertex->prev = NULL;
	vertex->nextPoly = NULL;
	vertex->nextHole = NULL;
	vertex->neighbor = NULL;
	vertex->intersection = LW_FALSE;
	vertex->entry = LW_TRUE;
	vertex->visited = LW_FALSE;
	vertex->sort = 0;

	return vertex;
}

/**
 * construct a new overlay intersection point
 */
OVERLAY_POINT*
overlay_create_intersection(double x, double y, float sort)
{
	OVERLAY_POINT* vertex = overlay_create_point(x, y);
	vertex->intersection = LW_TRUE;
	vertex->sort = sort;

	return vertex;
}

/**
 * insert a single intersection point on an edge, position is selected by the sort property of the intersection point.
 */
void
overlay_insert_intersection(OVERLAY_POINT* start, OVERLAY_POINT* intersection)
{
	OVERLAY_POINT* current = start;
	while (current->next->intersection && current->next->sort < intersection->sort) {
		current = current->next;

		if (current == start) {
			fprintf(stderr, "hit start\n");
			break;
		}
	}

	/* place intersection after current */
	OVERLAY_POINT* next = current->next;

	/* From:  current - next
	 * TO: current - intersection - next
	 */
	current->next = intersection;
	intersection->next = next;
	intersection->prev = current;
	next->prev = intersection;
}

/**
 * delete a ring
 */
void
overlay_delete(OVERLAY_POINT* vertex)
{
	if (vertex->neighbor != NULL) {
		vertex->neighbor->neighbor = NULL;
	}

	vertex->next->prev = vertex->prev;
	vertex->prev->next = vertex->next;

	lwfree(vertex);
	vertex = NULL;
}

/**
 * delete a whole structure
 */
void
overlay_delete_all(OVERLAY_POINT* vertex)
{
	if (vertex->nextPoly != NULL) {
		overlay_delete_all(vertex->nextPoly);
		vertex->nextPoly = NULL;
	}

	if (vertex->nextHole != NULL) {
		overlay_delete_all(vertex->nextHole);
		vertex->nextHole = NULL;
	}

	while (vertex->next != vertex) {
		vertex = vertex->next;
		overlay_delete(vertex->prev);
	}

	overlay_delete(vertex);
}


/**
 * get next non intersection point
 */
OVERLAY_POINT*
overlay_get_next_point(const OVERLAY_POINT* vertex, const OVERLAY_POINT* stop)
{
	OVERLAY_POINT* next = vertex->next;
	while (next->intersection) {
		next = next->next;

		/* test inside loop, to not quit on start point */
//		if (next == stop) {
//			fprintf(stderr, "hit start\n");
//			break;
//		}
	}

//	if (next == stop) {
//		return NULL;
//	}

	return next;
}

/**
 * convert lwpoly to a more suitable structure to handle this algorithm.
 */
OVERLAY_POINT*
overlay_from_lwpoly(const LWPOLY* geom)
{
	int i, j;

	OVERLAY_POINT* result_vertex;
	OVERLAY_POINT* first_vertex = NULL;
	OVERLAY_POINT* prev_vertex;
	OVERLAY_POINT* vertex;

	int start, until, step;

	for (i = 0; i < geom->nrings; i++)
	{
		/* reset ring */
		prev_vertex = NULL;

		/* rings may be in wrong order, fix that ad-hoc */
		if ( (i == 0 && ptarray_isccw(geom->rings[i]))
			|| (i > 0 && !ptarray_isccw(geom->rings[i])) ) {
			start = geom->rings[i]->npoints - 1;
			until = 0;
			step = -1;
		} else {
			start = 0;
			until = geom->rings[i]->npoints - 1;
			step = 1;
		}


		/* skip last point as it is the same as the first one */
		for (j = start; (start == 0 && j < until) || (start > 0 && j > until); j+=step)
		{
			POINT4D p;
//			fprintf(stderr, "Insert: %d %d\n", i, j);

			/* get current vertex */
			getPoint4d_p(geom->rings[i], j, &p);
			vertex = overlay_create_point(p.x, p.y);

			// double link current vertex
			if (prev_vertex != NULL) {
				vertex->prev = prev_vertex;
				prev_vertex->next = vertex;
			}
			prev_vertex = vertex;


			/* keep reference for the first one */
			if (j == start) {

				// keep reference to the entry vertex for the whole polygon
				if (i == 0) {
					first_vertex = vertex;
					result_vertex = vertex;
				} else {
					first_vertex->nextHole = vertex;
					first_vertex = vertex;
				}
			}
		}

		/* close ring */
		first_vertex->prev = vertex;
		vertex->next = first_vertex;
	}
//	fprintf(stderr, "EndIns\n");

	return result_vertex;
}

OVERLAY_POINT*
overlay_from_lwmpoly(const LWMPOLY* mpoly)
{
	int i;
	OVERLAY_POINT* point;
	OVERLAY_POINT* last = NULL;
	OVERLAY_POINT* first = NULL;

	for (i = 0; i < mpoly->ngeoms; i++)
	{
		point = overlay_from_lwpoly(mpoly->geoms[i]);
		if (!first) {
			first = point;
		}
		if (last) {
			last->nextPoly = point;
		}

		last = point;
	}

	fprintf(stderr, "EndInsM\n");
	return first;
}

/**
 * Check if any unvisited intersection exists. Returns it if found.
 */
OVERLAY_POINT*
overlay_get_next_intersection(OVERLAY_POINT* start)
{
	// always start on an intersection point, to get a direction
	OVERLAY_POINT* point = start->next;
	do {
		if (point->intersection && point->visited == LW_FALSE) {
			fprintf(stderr, "\n * unhandled %f-%f\n", point->x, point->y);
			return point;
		}

		point = point->next;
	} while (point != start);

	fprintf(stderr, "\n * iend\n");
	return NULL;
}

/**
 * helper function to simply add a new point to a pointarray by x and y coordinates
 */
void
addpoint(POINTARRAY* ring, double x, double y)
{
	POINT4D p;
	p.x = x;
	p.y = y;

	ptarray_insert_point(ring, &p, ring->npoints);
}


/**
 * from the point, which has to be an intersection, crawl through the lists and return the enclosing geography.
 */
void
overlay_points_to_ring(POINTARRAY* ring, OVERLAY_POINT* point)
{
	if (point->intersection == LW_FALSE) {
		// ERROR
//		LWDEBUG(4, "  point %.2f,%.2f has to be an intersection \n", point->x, point->y);
		return;
	}

	addpoint(ring, point->x, point->y);
	do {
		fprintf(stderr, "  * from %.2f,%.2f - %d,%d \n", point->x, point->y, point->intersection, point->entry);

		point->visited = LW_TRUE;
		point->neighbor->visited = LW_TRUE;

		if (point->entry) {
			do {
				point = point->next;
				fprintf(stderr, "   * point %.2f,%.2f - %d,%d \n", point->x, point->y, point->intersection, point->entry);
				addpoint(ring, point->x, point->y);
			} while (!point->intersection);
		} else {
			do {
				point = point->prev;
				fprintf(stderr, "   - point %.2f,%.2f - %d,%d \n", point->x, point->y, point->intersection, point->visited);
				addpoint(ring, point->x, point->y);
			} while (!point->intersection);
		}
		point = point->neighbor;
	} while (!point->visited);

	fprintf(stderr, "endring\n");
}

/**
 * convert overlay to pointarray, no fancy stuff.
 */
void
overlay_points_to_ring_simple(POINTARRAY* ring, OVERLAY_POINT* start)
{
	OVERLAY_POINT* point = start;
	addpoint(ring, point->x, point->y);
	do {
		point = point->next;
		addpoint(ring, point->x, point->y);
	} while (point != start);
}

/**
 * create lwmpoly from a starting overlay point.
 * @return NULL if no intersection was found
 */
LWMPOLY*
overlay_get_lwmpoly(OVERLAY_POINT* start, int srid)
{
	OVERLAY_POINT* point;
	OVERLAY_POINT* poly;

	LWMPOLY *result = lwmpoly_construct_empty(srid, 0, 0);

	for (poly = start; poly != NULL; poly = poly->nextPoly) {

		// handle exterior ring
		while ((point = overlay_get_next_intersection(poly)) != NULL) {

			/* construct lw structure */
			POINTARRAY *ring = ptarray_construct_empty(0, 0, 32);
			overlay_points_to_ring(ring, point);
			if (ring->npoints == 0) {
				fprintf(stderr, "skip empty ring\n");
				ptarray_free(ring);
				continue;
			}

			fprintf(stderr, "isHole?\n");
			// try to add as hole first
			if (lwmpoly_assign_hole(result, ring) == LW_FALSE) {
				fprintf(stderr, "addHole?\n");

				/* add points to lw structure */
				LWPOLY *resultpoly = lwpoly_construct_empty(srid, 0, 0);
				lwpoly_add_ring(resultpoly, ring);
				lwmpoly_add_lwpoly(result, resultpoly);
			}
			fprintf(stderr, "before next intersection\n");

		}
	}
	fprintf(stderr, "got mpoly?\n");


	if (!result->ngeoms) {
		LWDEBUG(4, "no intersections on exterior ring detected");
		lwmpoly_free(result);
		return NULL;
	}

	return result;
}


/**
 * add or mark intersection points in both lists
 */
void
overlay_insert_intersections(OVERLAY_POINT* geom1, OVERLAY_POINT* geom2)
{
	OVERLAY_POINT* poly1;
	OVERLAY_POINT* poly2;

	OVERLAY_POINT* ring1;
	OVERLAY_POINT* ring2;

	OVERLAY_POINT* point1;
	OVERLAY_POINT* point2;

	OVERLAY_POINT* next1;
	OVERLAY_POINT* next2;

	OVERLAY_POINT* intersection1;
	OVERLAY_POINT* intersection2;

	GEOGRAPHIC_POINT g;
	GEOGRAPHIC_EDGE e1;
	GEOGRAPHIC_EDGE e2;

	for (poly1 = geom1; poly1 != NULL; poly1 = poly1->nextPoly) {
		for (ring1 = poly1; ring1 != NULL; ring1 = ring1->nextHole) {
			FOREACH_OVERLAY_POINT(point1, ring1)
				{
					fprintf(stderr, "add intersection 1\n");
					/* get next not intersection point */
					next1 = overlay_get_next_point(point1, ring1);
					if (!next1) {
						fprintf(stderr, "not intersection 1\n");
						break;
					}

					/* construct edge for poly1 */
					geographic_point_init(point1->x, point1->y, &(e1.start));
					geographic_point_init(next1->x, next1->y, &(e1.end));

					for (poly2 = geom2; poly2 != NULL; poly2 = poly2->nextPoly) {
						for (ring2 = geom2; ring2 != NULL; ring2 = ring2->nextHole) {
//							fprintf(stderr, "check ring 2 %p\n", ring2->nextHole);

							FOREACH_OVERLAY_POINT(point2, ring2)
								{
									/* get next not intersection point */
									next2 = overlay_get_next_point(point2, ring2);
									if (!next2) {
										break;
									}
//
//									fprintf(stderr, "check intersection 2 %.2f,%.2f - %.2f,%.2f\n", point2->x,
//											point2->y,
//											next2->x,
//											next2->y);
									// TODO check for edge same

									/* construct edge for poly 2 */
									geographic_point_init(point2->x, point2->y, &(e2.start));
									geographic_point_init(next2->x, next2->y, &(e2.end));

									/* check for intersection */
									if (edge_intersection(&e1, &e2, &g) == LW_TRUE) {
										fprintf(stderr, "add intersection 2\n");
										float position1 =
												sphere_distance(&(e1.start), &g) /
												sphere_distance(&(e1.start), &(e1.end));
										float position2 =
												sphere_distance(&(e2.start), &g) /
												sphere_distance(&(e2.start), &(e2.end));

										if (position1 == 1 || position2 == 1) {
											// intersection at end point is not handled, instead the start processed
										} else {
											double x = rad2deg(g.lon);
											double y = rad2deg(g.lat);

											if (position1 == 0) {
												intersection1 = point1;
												intersection1->intersection = LW_TRUE;
											} else {
												intersection1 = overlay_create_intersection(x, y, position1);
												overlay_insert_intersection(point1, intersection1);
											}

											if (position2 == 0) {
												intersection2 = point2;
												intersection2->intersection = LW_TRUE;
											} else {
												intersection2 = overlay_create_intersection(x, y, position2);
												overlay_insert_intersection(point2, intersection2);
											}

											intersection1->neighbor = intersection2;
											intersection2->neighbor = intersection1;
										}
									}

//									fprintf(stderr, "nextp2\n");
								}
									ENDFOREACH_OVERLAY_POINT(point2, ring2, next2);
//							fprintf(stderr, "next22\n");
						}
					}
//					fprintf(stderr, "next1p\n");
				}
					ENDFOREACH_OVERLAY_POINT(point1, ring1, next1);
//			fprintf(stderr, "next11\n");
		}
	}
}


/**
 * mark intersections alternating with entry/exit.
 */
void
overlay_mark_ring_entries(OVERLAY_POINT* geom, int direction)
{
	OVERLAY_POINT* point;

	int isOutside = direction;
	FOREACH_OVERLAY_POINT(point, geom)
		{
			point->visited = LW_FALSE;

			// get next intersection
			if (point->intersection) {
				fprintf(stderr, "mark intersection %d\n", isOutside);
				point->entry = isOutside;

				// toggle
				isOutside = isOutside ? LW_FALSE : LW_TRUE;
			}

		} ENDFOREACH_OVERLAY_POINT(point, geom, point->next)
}


/**
 * assign ring to matching polygon in target
 */
int
lwmpoly_assign_hole(LWMPOLY* target, POINTARRAY* ring)
{
	int i;

	// find suitable polygon to add hole into

	POINT2D test_point;
	getPoint2d_p(ring, 0, &test_point);
	for (i = 0; i < target->ngeoms; i++) {

//		fprintf(stderr, "inside %d?\n", lwgeom_get_type(target->geoms[i]));

		if (lwpoly_covers_point2d(target->geoms[i], &test_point)) {
//			fprintf(stderr, "haa?\n");
//			fprintf(stderr, "hui?\n");
			/* alternative would be to construct from scratch */
			if (!ptarray_isccw(ring)) {
//				fprintf(stderr, "arr?\n");
				ptarray_reverse_in_place(ring);
			}
//			fprintf(stderr, "oh?\n");
			lwpoly_add_ring(target->geoms[i], ring);
			return LW_TRUE;
		}
		/* else hole is now in an unused area */
	}

	return LW_FALSE;
}


/**
 * add holes related to given first_hole to the target lwmpoly.
 */
void
overlay_add_holes_lwmpoly(LWMPOLY* target, OVERLAY_POINT* first_hole, int useUnvisitedHoles)
{
	OVERLAY_POINT* hole = first_hole;
	OVERLAY_POINT* point;
	int j = 0;
	int visits = 0;

	while (hole != NULL) {
		j++;


		// handle exterior ring
		point = hole;
		visits = 0;
		while ((point = overlay_get_next_intersection(point)) != NULL)
		{
			visits++;
			fprintf(stderr, "\n\n  * hole %.2f,%.2f \n", point->x, point->y);

			/* construct lw structure */
			POINTARRAY* interior = ptarray_construct_empty(0, 0, 32);

			/* add points to lw structure */
			overlay_points_to_ring(interior, point);

			/* add hole to existing polygon */
			lwmpoly_assign_hole(target, interior);
		}

		if (useUnvisitedHoles && visits == 0)
		{

			POINTARRAY* interior = ptarray_construct_empty(0, 0, 32);

			/* add points to lw structure */
			overlay_points_to_ring_simple(interior, hole);

			/* add hole to existing polygon */
			lwmpoly_assign_hole(target, interior);
		}
		fprintf(stderr, "end render hole to lwpoly\n", hole,  hole->nextHole);

		hole = hole->nextHole;
	}
}

/**
 * Tests if ring starts in polygon.
 * Returns LW_TRUE if test is successful.
 */
int
starts_inside(const LWMPOLY* mpoly, const POINTARRAY* ring)
{
	int i;
	POINT2D test_point;
	getPoint2d_p(ring, 0, &test_point);
	for (i = 0; i < mpoly->ngeoms; i++) {

//		fprintf(stderr, "inside %d?\n", lwgeom_get_type(mpoly->geoms[i]));

		if (mpoly->geoms[i]->nrings > 0 && lwpoly_covers_point2d(mpoly->geoms[i], &test_point)) {
			return LW_TRUE;
		}
	}

	return LW_FALSE;
}

/**
 * mark intersection points as entry/exit
 */
LWMPOLY*
overlay_mark_entries(OVERLAY_POINT* first, OVERLAY_POINT* second, const LWMPOLY* lwfirst, const LWMPOLY* lwsecond, int useFirst, int useSecond) {
	OVERLAY_POINT *poly = NULL;
	OVERLAY_POINT *hole = NULL;
	int ring, pn;

	for (pn = 0, poly = first; poly != NULL; poly = poly->nextPoly, pn++) {
		for (ring = 0, hole = poly; hole != NULL; hole = hole->nextHole, ring++) {
			overlay_mark_ring_entries(hole, useFirst != starts_inside(lwsecond, lwfirst->geoms[pn]->rings[ring]));
		}
	}
	for (pn = 0, poly = second; poly != NULL; poly = poly->nextPoly, pn++) {
		for (ring = 0, hole = poly; hole != NULL; hole = hole->nextHole, ring++) {
			overlay_mark_ring_entries(hole, useSecond != starts_inside(lwfirst, lwsecond->geoms[pn]->rings[ring]));
		}
	}

	LWMPOLY *result = overlay_get_lwmpoly(first, lwfirst->srid);

	/* handle interior - interior rings */
	if (result)
	{
		if (first->nextHole) {
			fprintf(stderr, "add holes 1\n");
			overlay_add_holes_lwmpoly(result, first->nextHole, useFirst);
		}
		fprintf(stderr, "end holes 1\n");

		if (second->nextHole) {
			fprintf(stderr, "add holes 2\n");
			overlay_add_holes_lwmpoly(result, second->nextHole, useSecond);
		}
		fprintf(stderr, "add holes 2\n");
	}

	fprintf(stderr, "end mark\n");

	return result;
}

/**
 * performs overlay action between two polygons
 */
LWGEOM *
lwgeodetic_overlay(const LWMPOLY *lwfirst, const LWMPOLY *lwsecond, const int combination)
{
	/* convert to more comfortable structure, ccw is converted on the fly to cw */
	fprintf(stderr, "phase 0a done\n");
	OVERLAY_POINT* first = overlay_from_lwmpoly(lwfirst);
	fprintf(stderr, "phase 0b done\n");
	OVERLAY_POINT* second = overlay_from_lwmpoly(lwsecond);

	fprintf(stderr, "phase 0 done\n");
	/* insert intersections points between both overlays */
	overlay_insert_intersections(first, second);
	fprintf(stderr, "phase 1 done\n");

	int is_first_in_second = starts_inside(lwsecond, lwfirst->geoms[0]->rings[0]);
	int is_second_in_first = starts_inside(lwfirst, lwsecond->geoms[0]->rings[0]);

	LWMPOLY *result = NULL;
	switch (combination) {

		// A and B
		case COMBINE_INTERSECTION: {
			result = overlay_mark_entries(first, second, lwfirst, lwsecond, LW_TRUE, LW_TRUE);

			fprintf(stderr, "end mark2 %p\n", result);
			/* no intersection points found */
			if (result == NULL)
			{
				fprintf(stderr, "intersection result empty: %d %d \n", is_first_in_second, is_second_in_first);
				if (is_first_in_second)
				{
					// A inside B -> return A
					result = (LWGEOM*)lwpoly_clone_deep(lwfirst);
				}
				else if (is_second_in_first)
				{
					// B inside A -> return B
					result = (LWGEOM*)lwpoly_clone_deep(lwsecond);
				}
				else
				{
					// A and B have no common areas -> return empty
					result = lwgeom_construct_empty(POLYGONTYPE, lwfirst->srid, 0, 0);
				}
			}
		}
			break;

		// A or B
		case COMBINE_UNION:
			result = overlay_mark_entries(first, second, lwfirst, lwsecond, LW_FALSE, LW_FALSE);

			// no intersections found
			if (result == NULL)
			{
				fprintf(stderr, "union result empty: %d %d \n", is_first_in_second, is_second_in_first);
				if (is_first_in_second)
				{
					// A inside B -> return B
					result = (LWGEOM*)lwpoly_clone_deep(lwsecond);
				}
				else if (is_second_in_first)
				{
					// B inside A -> return A
					result = (LWGEOM*)lwpoly_clone_deep(lwfirst);
				}
				else
				{
					// A and B have no common areas -> return (A + B)
					result = lwmpoly_construct_empty(lwfirst->srid, 0, 0);
					lwmpoly_add_lwpoly(result, lwpoly_clone_deep(lwfirst));
					lwmpoly_add_lwpoly(result, lwpoly_clone_deep(lwsecond));
				}
			}
			break;

		// A without B
		case COMBINE_DIFFERENCE:
			result = overlay_mark_entries(first, second, lwfirst, lwsecond, LW_FALSE, LW_TRUE);

			// no intersections found
			if (result == NULL)
			{
				fprintf(stderr, "difference result empty: %d %d \n", is_first_in_second, is_second_in_first);
				if (is_first_in_second)
				{
					// A inside B -> return empty
					result = lwgeom_construct_empty(POLYGONTYPE, lwfirst->srid, 0, 0);
				}
				else if (is_second_in_first)
				{
					// B inside A -> return A and add outer Ring of B + drop all existings holes in A that are covered by B
					result = (LWGEOM*)lwpoly_clone_deep(lwfirst);
					// TODO add outer ring of B to A (union with all inner rings)
				}
				else
				{
					// A and B have no common areas -> return A
					result = (LWGEOM*)lwpoly_clone_deep(lwfirst);
				}
			}
			break;
	}

	fprintf(stderr, "before delete\n");
	overlay_delete_all(first);
	overlay_delete_all(second);
	fprintf(stderr, "after delete %p\n", result);
	return lwmpoly_as_simple_lwgeom(result);
} // function
