/**********************************************************************
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright (C) 2004 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _PROFILE_H
#define _PROFILE_H 1

#include <math.h>

/* Only enable profiling code if POSTGIS_PROFILE in postgis_config.h is > 0 */
#if POSTGIS_PROFILE > 0

#include <sys/time.h>

#define PROF_P2G1 0
#define PROF_P2G2 1
#define PROF_G2P 2
#define PROF_GRUN 3
#define PROF_QRUN 4

struct timeval profstart[5], profstop[5];
long proftime[5];
long profipts1, profipts2, profopts;

#define PROFSTART(x) do { gettimeofday(&(profstart[x]), NULL); } while (0);
#define PROFSTOP(x) do { gettimeofday(&(profstop[x]), NULL); \
	proftime[x] = ( profstop[x].tv_sec*1000000+profstop[x].tv_usec) - \
		( profstart[x].tv_sec*1000000+profstart[x].tv_usec); \
	} while (0);
#define PROFREPORT(n, i1, i2, o) do { \
	profipts1 = profipts2 = profopts = 0; \
	if ((i1)) profipts1 += lwgeom_npoints(SERIALIZED_FORM((i1))); \
	else proftime[PROF_P2G1] = 0; \
	if ((i2)) profipts2 += lwgeom_npoints(SERIALIZED_FORM((i2))); \
	else proftime[PROF_P2G2] = 0; \
	if ((o)) profopts += lwgeom_npoints(SERIALIZED_FORM((o))); \
	else proftime[PROF_G2P] = 0; \
	long int conv = proftime[PROF_P2G1]+proftime[PROF_P2G2]+proftime[PROF_G2P]; \
	long int run = proftime[PROF_GRUN]; \
	long int qrun = proftime[PROF_QRUN]; \
	long int tot = qrun; \
	int convpercent = round(((double)conv/(double)tot)*100); \
	int runpercent = round(((double)run/(double)tot)*100); \
	elog(NOTICE, \
		"PRF(%s) ipt1 %lu ipt2 %lu opt %lu p2g1 %lu p2g2 %lu g2p %lu grun %lu qrun %lu perc %d%%", \
		(n), \
		profipts1, profipts2, profopts, \
		proftime[PROF_P2G1], proftime[PROF_P2G2], \
		proftime[PROF_G2P], \
		proftime[PROF_GRUN], \
		proftime[PROF_QRUN], \
		runpercent+convpercent); \
	} while (0);

#else

/* Empty prototype that can be optimised away by the compiler for non-profiling builds */
#define PROFSTART(x) ((void) 0)
#define PROFSTOP(x) ((void) 0)
#define PROFREPORT(n, i1, i2, o) ((void) 0)

#endif /* POSTGIS_PROFILE */

#endif /* _PROFILE_H */
