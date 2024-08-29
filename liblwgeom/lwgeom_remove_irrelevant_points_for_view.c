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
 * Copyright (C) 2024 Sam Peters <gluser1357@gmx.de>
 *
 **********************************************************************/

#include "lwgeom_remove_irrelevant_points_for_view.h"

// ===============================================================================
// Encodes the location of a value related to min and max.

// Returns
// - 0x1 for value < min
// - 0x2 for min <= value <= max
// - 0x4 for value > max
// ===============================================================================
int encodeToBits(double value, double min, double max) {
    return value < min ? 0x1 : value <= max ? 0x2 : 0x4;
}

// ===============================================================================
// Encodes the location where a line segment S given by (xa, ya) and (xb, yb)
// cuts a straight L (either xmin, ymin, xmax or ymax) of a bounding box
// defined by (xmin, ymin) and (xmax, ymax) without actually computing
// the cutting point (xs, ys) for performance reasons.
//
// Allowed values for the straightPosition are
// - 1 for top (ymin)
// - 2 for bottom (ymax)
// - 3 for left (xmin)
// - 4 for right (xmax)
//
// Returns
// - 0x1 if xs < xmin (top or bottom), or ys < ymin (left or right)
// - 0x2 if xmin <= xs < xmax (top or bottom), or ymin <= ys < ymax (left or right)
// - 0x4 if xs >= xmax (top or bottom), or ys >= ymax (left or right)
// - 0x0 if no cutting point can be determined (if S and L are parallel or straightPosition is not valid)
// ===============================================================================
int encodeToBitsStraight(double xa, double ya, double xb, double yb, double xmin, double ymin, double xmax, double ymax, int straightPosition) {

	double x, y, dx, dy, d, c;

	if (straightPosition == 1 || straightPosition == 2) {

		// top and bottom
		if (ya == yb) return 0;

		y = straightPosition == 2 ? ymax : ymin;
		if (ya < y && yb < y) return 0;
		if (ya > y && yb > y) return 0;

		dx = xb - xa;
		dy = yb - ya;
		d = dy;
		c = dx * (y - ya);
		if (dy < 0) {
			d = -d;
			c = -c;
		}
		return c < d * (xmin - xa) ? 0x1 : c < d * (xmax - xa) ? 0x2 : 0x4;
	}

	if (straightPosition == 3 || straightPosition == 4) {

		// left and right
		if (xa == xb) return 0;

		x = straightPosition == 4 ? xmax : xmin;
		if (xa < x && xb < x) return 0;
		if (xa > x && xb > x) return 0;

		dx = xb - xa;
		dy = yb - ya;
		d = dx;
		c = dy * (x - xa);
		if (dx < 0) {
			d = -d;
			c = -c;
		}
		return c < d * (ymin - ya) ? 0x1 : c < d * (ymax - ya) ? 0x2 : 0x4;
	}

	return 0;
}

// ===============================================================================
// Helper function for polygon and polyline POINTARRAY's.
// Removes points being irrelevant for rendering the geometry
// within a view specified by rectangular bounds without introducing
// new points. The main idea is to sequentially evaluate a group of
// three consecutive points and decide if the second point has impact
// on the rendering result within the given bounds. If it doesn't
// have impact it will be skipped.
//
// Note on the algorithm:
// The algorithm tries to remove points outside the given bounds
// on a best-effort basis, optimized for speed. It doesn't use allocs,
// instead it reuses the given point array.
// There are some known cases where a minor improvement (slightly less points
// in the result) could be achieved by checking which point(s) of a sequence of
// outside points would be optimal to keep. Since this would introduce a lot
// more code complexity and a backing array and would likely have
// no real practical impact this step is skipped.
//
// Note on cartesian_hint:
// - if false, the algorithm removes one or a sequence of points
//   lying on "the same side" (either top, bottom, left or right) of the
//   given bounds except the first and last point of that sequence.
// - if true, the algorithm assumes that the coordinates are rendered in
//   a cartesian coordinate system and tries to remove further points
//   if the resulting connection lines do not cross the borders of
//   the rectangular view given by the bounds.
//   Please note that this option might produce rendering artifacts
//   if the coordinates are used for rendering in a non-cartesian
//   coordinate system.
// ===============================================================================
void removePoints(POINTARRAY *points, GBOX *bounds, bool closed, bool cartesian_hint) {

	int npoints, minpoints;
	double xmin, ymin, xmax, ymax;

	int i, j, next, w;
	int vx, vy, vx0, vy0, vx1, vy1, vxall, vyall;
	double xx, yy, xx0, yy0, xx1, yy1;
	bool sameX, sameY, same, insideX, insideY, inside, insideAll, skip, clear;
	int vvx, vvy;
	POINT4D p, p0, p1;  // current, previous, next;

	double xa, ya, xb, yb;
	bool cutting;
	int crossingN;

	// point number check
	npoints = points->npoints;
	minpoints = closed ? 4 : 2; // min points for each polygon ring or linestring
	if (npoints < minpoints) {
		// clear if not expected minimum number of points
		points->npoints = 0;
		return;
	}

	xmin = bounds->xmin;
	ymin = bounds->ymin;
	xmax = bounds->xmax;
	ymax = bounds->ymax;

	// get previous point [i-1]
	if (closed) {
		getPoint4d_p(points, 0, &p);
		getPoint4d_p(points, npoints - 1, &p0);
		if (p.x != p0.x || p.y != p0.y) return; // requirement for polygons: startpoint equals endpoint. Leave untouched of not met.
		npoints--; // remove double here, and re-add at the end
		getPoint4d_p(points, npoints - 1, &p0);
	}
	else {
		getPoint4d_p(points, 0, &p0);  // for linestrings reuse start point
	}

	xx0 = p0.x;
	yy0 = p0.y;
	vx0 = encodeToBits(xx0, xmin, xmax);
	vy0 = encodeToBits(yy0, ymin, ymax);

	// for all points
	w = 0;
	vxall = 0;
	vyall = 0;
	insideAll = false;
	for (i = 0; i < npoints; i++) {

		// get current point [i]
		getPoint4d_p(points, i, &p);
		xx = p.x;
		yy = p.y;
		vx = encodeToBits(xx, xmin, xmax);
		vy = encodeToBits(yy, ymin, ymax);

		// get subsequent point [i+1]
		next = i + 1;
		if (next == npoints) {
			if (closed) next = 0; // for polygons, use (new) start point as end point
			else next = i;  // for linestrings reuse last point as end point
		}
		getPoint4d_p(points, next, &p1);
		xx1 = p1.x;
		yy1 = p1.y;
		vx1 = encodeToBits(xx1, xmin, xmax);
		vy1 = encodeToBits(yy1, ymin, ymax);

		sameX = vx == vx1 && vx == vx0;
		sameY = vy == vy1 && vy == vy0;
		same = sameX && sameY;
		insideX = vx == 0x02;
		insideY = vy == 0x02;
		inside = insideX && insideY;

		skip = sameX && sameY && !inside;	// three consecutive points in same outside quarter, leave out central one
		skip |= sameX && !insideX;			// three consecutive points in same outside area (left or right), leave out central one
		skip |= sameY && !insideY;			// three consecutive points in same outside area (top or bottom), leave out central one

		// check for irrelevant points that would introduce "diagonal"
		// lines between different outside quadrants which may cross the bounds
		if (cartesian_hint && !skip && !same && !inside && (vx0 | vy0) != 0x02 && (vx1 | vy1) != 0x02) {

			vvx = 0;
			vvy = 0;
			for (j = 0; j < 2; j++) {
				// left, right
				vvx |= encodeToBitsStraight(xx0, yy0, xx, yy, xmin, ymin, xmax, ymax, j + 1);
				vvx |= encodeToBitsStraight(xx, yy, xx1, yy1, xmin, ymin, xmax, ymax, j + 1);
				vvx |= encodeToBitsStraight(xx0, yy0, xx1, yy1, xmin, ymin, xmax, ymax, j + 1);
				if ((vvx & 0x2) != 0) break;

				// top, bottom
				vvy |= encodeToBitsStraight(xx0, yy0, xx, yy, xmin, ymin, xmax, ymax, j + 3);
				vvy |= encodeToBitsStraight(xx, yy, xx1, yy1, xmin, ymin, xmax, ymax, j + 3);
				vvy |= encodeToBitsStraight(xx0, yy0, xx1, yy1, xmin, ymin, xmax, ymax, j + 3);
				if ((vvy & 0x2) != 0) break;
			}

			if (((vvx | vvy) & 0x2) == 0) {
				// if no bbox bounds crossed:
				skip |= vvx == 0x1;		// three cutting points are left outside
				skip |= vvx == 0x4;		// three cutting points are right outside
				skip |= vvy == 0x1;		// three cutting points are top outside
				skip |= vvy == 0x4;		// three cutting points are bottom outside
			}
		}

		if (skip) continue;

		// save current point at [w <= i]
		ptarray_set_point4d(points, w++, &p);
		vx0 = vx;
		vy0 = vy;
		xx0 = xx;
		yy0 = yy;
		vxall |= vx;
		vyall |= vy;
		insideAll |= inside;
	}

	if (closed && w > 0) {
		// re-add first new point at the end if closed
		getPoint4d_p(points, 0, &p);
		ptarray_set_point4d(points, w++, &p);
	}

	// eval empty cases
	clear = w < minpoints; 		// too less points
	clear |= vxall == 0x01;		// completely left outside
	clear |= vxall == 0x04;		// completely right outside
	clear |= vyall == 0x01;		// completely top outside
	clear |= vyall == 0x04;		// completely bottom outside

	// clear if everything is outside and not enclosing
	if (cartesian_hint && !clear && !insideAll) { // not required if points inside bbox

		cutting = false;
		for (int r = 0; r < w - 1; r++) {

			getPoint4d_p(points, r, &p);
			getPoint4d_p(points, r + 1, &p1);

			xa = p.x;
			ya = p.y;
			xb = p1.x;
			yb = p1.y;

			for (j = 0; j < 4 && !cutting; j++) {
				cutting |= encodeToBitsStraight(xa, ya, xb, yb, xmin, ymin, xmax, ymax, j + 1) == 0x2;
			}
		}

		if (!cutting && closed) {
			// test if polygon surrounds bbox completely or is fully contained within bbox
			// using even-odd rule algorithm
			crossingN = 0;
			for (int r = 0; r < w - 1; r++) {

				getPoint4d_p(points, r, &p);
				getPoint4d_p(points, r + 1, &p1);

				xa = p.x;
				ya = p.y;
				xb = p1.x;
				yb = p1.y;

				if (encodeToBitsStraight(xa, ya, xb, yb, xmin, ymin, xmax, ymax, 1) == 0x1) crossingN++;
			}
			clear |= crossingN % 2 == 0; // not surrounding, we can clear
		}
	}
	if (clear) w = 0;

	points->npoints = w;
}


void lwgeom_remove_irrelevant_points_for_view(LWGEOM *geom, GBOX *bbox, bool cartesian_hint) {

	unsigned int i, j, iw, jw;

	if (geom->type == LINETYPE) {

		LWLINE* line = (LWLINE*)geom;
		removePoints(line->points, bbox, false, cartesian_hint);
	}

	if (geom->type == MULTILINETYPE) {

		LWMLINE* mline = (LWMLINE*)geom;
		iw = 0;
		for (i=0; i<mline->ngeoms; i++) {
			LWLINE* line = mline->geoms[i];
			removePoints(line->points, bbox, false, cartesian_hint);

			if (line->points->npoints) {
				// keep (reduced) line
				mline->geoms[iw++] = line;
			}
			else {
				// discard current line
				lwfree(line);
			}
		}
		mline->ngeoms = iw;
	}

	if (geom->type == POLYGONTYPE) {

		LWPOLY* polygon = (LWPOLY*)geom;
		iw = 0;
		for (i=0; i<polygon->nrings; i++) {
			removePoints(polygon->rings[i], bbox, true, cartesian_hint);

			if (polygon->rings[i]->npoints) {
				// keep (reduced) ring
				polygon->rings[iw++] = polygon->rings[i];
			}
			else {
				if (!i) {
					// exterior ring outside, free and skip all rings
					unsigned int k;
					for (k=0; k<polygon->nrings; k++) {
						lwfree(polygon->rings[k]);
					}
					break;
				}
				else {
					// free and remove current interior ring
					lwfree(polygon->rings[i]);
				}
			}
		}
		polygon->nrings = iw;
	}

	if (geom->type == MULTIPOLYGONTYPE) {

		LWMPOLY* mpolygon = (LWMPOLY*)geom;
		jw = 0;
		for (j=0; j<mpolygon->ngeoms; j++) {

			LWPOLY* polygon = mpolygon->geoms[j];
			iw = 0;
			for (i=0; i<polygon->nrings; i++) {
				removePoints(polygon->rings[i], bbox, true, cartesian_hint);

				if (polygon->rings[i]->npoints) {
					// keep (reduced) ring
					polygon->rings[iw++] = polygon->rings[i];
				}
				else {
					if (!i) {
						// exterior ring outside, free and skip all rings
						unsigned int k;
						for (k=0; k<polygon->nrings; k++) {
							lwfree(polygon->rings[k]);
						}
						break;
					}
					else {
						// free and remove current interior ring
						lwfree(polygon->rings[i]);
					}
				}
			}
			polygon->nrings = iw;

			if (iw) {
				mpolygon->geoms[jw++] = polygon;
			}
			else {
				// free and remove polygon from multipolygon
				lwfree(polygon);
			}
		}
		mpolygon->ngeoms = jw;
	}
}
