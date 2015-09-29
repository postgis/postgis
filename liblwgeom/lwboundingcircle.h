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

#ifndef _LWBOUNDINGCIRCLE
#define _LWBOUNDINGCIRCLE 1

typedef struct {
	POINT2D center;
	double radius;
} LW_BOUNDINGCIRCLE;

/* Calculates the minimum circle that encloses all of the points in g, using a
 * two-dimensional implementation of the algorithm proposed in:
 *
 * Welzl, Emo (1991), "Smallest enclosing disks (balls and elipsoids)."  
 * New Results and Trends in Computer Science (H. Maurer, Ed.), Lecture Notes
 * in Computer Science, 555 (1991) 359-370.
 *
 * Available online at the time of this writing at 
 * https://www.inf.ethz.ch/personal/emo/PublFiles/SmallEnclDisk_LNCS555_91.pdf
 *
 * Returns LW_SUCCESS if the circle could be calculated, LW_FAILURE otherwise.
 */
int lwgeom_calculate_mbc(const LWGEOM* g, LW_BOUNDINGCIRCLE* result);

#endif
