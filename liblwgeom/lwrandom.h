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

#include <stddef.h>
#include <stdint.h>

void lwrandom_set_seed(int32_t seed);
double lwrandom_uniform(void);

/* for low-level external debugging */
void _lwrandom_set_seeds(int32_t s1, int32_t s2);
int32_t _lwrandom_get_seed(size_t idx);
