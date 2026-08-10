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
* Copyright 2014 Kashif Rasul <kashif.rasul@gmail.com> and
 *
 **********************************************************************/


#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "liblwgeom.h"
#include "lwgeom_log.h"
#include "../postgis_config.h"

static int
encoded_polyline_read_varint(const char *encodedpolyline, int length, int *idx, uint32_t *value)
{
	uint32_t res = 0;
	unsigned int shift = 0;

	while (1)
	{
		int byte;
		if (*idx >= length)
		{
			lwerror("lwgeom_from_encoded_polyline: input is truncated");
			return LW_FALSE;
		}

		byte = (unsigned char)encodedpolyline[(*idx)++] - 63;
		if (byte < 0)
		{
			lwerror("lwgeom_from_encoded_polyline: input contains an invalid byte");
			return LW_FALSE;
		}
		if (shift > 30 || (shift == 30 && ((byte & 0x1C) || (byte >= 0x20))))
		{
			lwerror("lwgeom_from_encoded_polyline: coordinate value is too large");
			return LW_FALSE;
		}

		res |= (uint32_t)(byte & 0x1F) << shift;
		if (byte < 0x20)
			break;

		shift += 5;
	}

	*value = res;
	return LW_TRUE;
}

static int32_t
encoded_polyline_zigzag_decode(uint32_t value)
{
	return (int32_t)((value >> 1) ^ (uint32_t)(-(int32_t)(value & 1)));
}

LWGEOM*
lwgeom_from_encoded_polyline(const char *encodedpolyline, int precision)
{
  LWGEOM *geom = NULL;
  POINTARRAY *pa = NULL;
  int length = strlen(encodedpolyline);
  int idx = 0;
	double scale = pow(10,precision);

  int32_t latitude = 0;
  int32_t longitude = 0;

  pa = ptarray_construct_empty(LW_FALSE, LW_FALSE, 1);

  while (idx < length) {
    POINT4D pt;
    uint32_t res = 0;

    if (!encoded_polyline_read_varint(encodedpolyline, length, &idx, &res))
    {
	    ptarray_free(pa);
	    return NULL;
    }
    int32_t deltaLat = encoded_polyline_zigzag_decode(res);
    latitude += deltaLat;

    if (!encoded_polyline_read_varint(encodedpolyline, length, &idx, &res))
    {
	    ptarray_free(pa);
	    return NULL;
    }
    int32_t deltaLon = encoded_polyline_zigzag_decode(res);
    longitude += deltaLon;

    pt.x = longitude/scale;
    pt.y = latitude/scale;
	pt.m = pt.z = 0.0;
    ptarray_append_point(pa, &pt, LW_FALSE);
  }

  geom = (LWGEOM *)lwline_construct(4326, NULL, pa);
  lwgeom_add_bbox(geom);

  return geom;
}
