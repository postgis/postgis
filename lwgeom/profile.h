#ifndef _PROFILE_H
#define _PROFILE_H 1

#include <math.h>

/*
 * Define this to have profiling enabled
 */
#define PROFILE

#ifdef PROFILE
#include <sys/time.h>
#define PROF_P2G 0
#define PROF_G2P 1
#define PROF_GRUN 2
#define PROF_QRUN 3
struct timeval profstart[4], profstop[4];
long proftime[4];
long profipts, profopts;
#define profstart(x) do { gettimeofday(&(profstart[x]), NULL); } while (0);
#define profstop(x) do { gettimeofday(&(profstop[x]), NULL); \
	proftime[x] = ( profstop[x].tv_sec*1000000+profstop[x].tv_usec) - \
		( profstart[x].tv_sec*1000000+profstart[x].tv_usec); \
	} while (0);
#define profreport(n, x, y, r) do { \
	profipts = 0; \
	if ((x)) profipts += lwgeom_npoints(SERIALIZED_FORM((x))); \
	if ((y)) profipts += lwgeom_npoints(SERIALIZED_FORM((y))); \
	if ((r)) profopts = lwgeom_npoints(SERIALIZED_FORM((r))); \
	else profopts = 0; \
	long int conv = proftime[PROF_P2G]+proftime[PROF_G2P]; \
	long int run = proftime[PROF_GRUN]; \
	long int qrun = proftime[PROF_QRUN]; \
	long int tot = qrun; \
	int convpercent = round(((double)conv/(double)tot)*100); \
	int runpercent = round(((double)run/(double)tot)*100); \
	elog(NOTICE, \
		"PROF_DET(%s): npts:%lu+%lu=%lu cnv:%lu+%lu run:%lu gtot:%lu qtot:%lu", \
		(n), \
		profipts, profopts, profipts+profopts, \
		proftime[PROF_P2G], proftime[PROF_G2P], \
		proftime[PROF_GRUN], \
		proftime[PROF_P2G]+proftime[PROF_G2P]+proftime[PROF_GRUN], \
		proftime[PROF_QRUN]); \
	elog(NOTICE, "PROF_SUM(%s): pts %lu cnv %d%% grun %d%% tot %d%%", \
		(n), \
		profipts+profopts, \
		convpercent, \
		runpercent, \
		convpercent+runpercent); \
	} while (0);
#endif // PROFILE


#endif // _PROFILE_H
