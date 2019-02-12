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
 * Copyright 2019 Mike Taves
 *
 **********************************************************************/

#include "lwrandom.h"

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

static unsigned char _lwrandom_seed_set = 0;
static int32_t _lwrandom_seed[3] = {0x330e, 0xabcd, 0x1234};

/*
 * Set seed for a random number generator.
 * Repeatable numbers are generated with seed values >= 1.
 * When seed is zero and has not previously been set, it is based on
 * Unix time (seconds) and process ID. */
void
lwrandom_set_seed(int32_t seed)
{
	if (seed == 0)
	{
		if (_lwrandom_seed_set == 0)
			seed = (unsigned int)time(NULL) + (unsigned int)getpid() - 0xbadd;
		else
			return;
	}
	/* s1 value between 1 and 2147483562 */
	_lwrandom_seed[1] = (((int64_t)seed + 0xfeed) % 2147483562) + 1;
	/* s2 value between 1 and 2147483398 */
	_lwrandom_seed[2] = ((((int64_t)seed + 0xdefeb) << 5) % 2147483398) + 1;
	_lwrandom_seed_set = 1;
}

/* for low-level external debugging */
void
_lwrandom_set_seeds(int32_t s1, int32_t s2)
{
	/* _lwrandom_seed[0] not used */
	_lwrandom_seed[1] = s1;
	_lwrandom_seed[2] = s2;
	_lwrandom_seed_set = 1;
}
int32_t
_lwrandom_get_seed(size_t idx)
{
	return _lwrandom_seed[idx];
}

/*
 * Generate a random floating-point value.
 * Values are uniformly distributed between 0 and 1.
 *
 * Authors:
 *   Pierre L'Ecuyer (1988), see source code in Figure 3.
 *   C version by John Burkardt, modified by Mike Taves.
 *
 * Reference:
 *   Pierre L'Ecuyer,
 *   Efficient and Portable Combined Random Number Generators,
 *   Communications of the ACM, Volume 31, Number 6, June 1988,
 *   pages 742-751. doi:10.1145/62959.62969
 */
double
lwrandom_uniform(void)
{
	double value;
	int32_t k;
	int32_t z;
	int32_t *s1 = &_lwrandom_seed[1];
	int32_t *s2 = &_lwrandom_seed[2];

	k = *s1 / 53668;
	*s1 = 40014 * (*s1 - k * 53668) - k * 12211;
	if (*s1 < 0)
		*s1 += 2147483563;

	k = *s2 / 52774;
	*s2 = 40692 * (*s2 - k * 52774) - k * 3791;
	if (*s2 < 0)
		*s2 += 2147483399;

	z = *s1 - *s2;
	if (z < 1)
		z += 2147483562;

	value = (double)(z) / 2147483563.0;

	return value;
}
