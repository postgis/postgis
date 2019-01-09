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
 * Copyright 2019 Raúl Marín
 *
 **********************************************************************/

#ifndef LWGEOM_WAGYU_INTERRUPT_H
#define LWGEOM_WAGYU_INTERRUPT_H

#include <stdexcept>

namespace {
    bool WAGYU_INTERRUPT_REQUESTED = false;
}

namespace mapbox {
namespace geometry {
namespace wagyu {

    static void lwgeom_interrupt_reset(void)
    {
        WAGYU_INTERRUPT_REQUESTED = false;
    }

    static void lwgeom_interrupt_request(void)
    {
        WAGYU_INTERRUPT_REQUESTED = true;
    }

    static void lwgeom_interrupt_check(void)
    {
        if (WAGYU_INTERRUPT_REQUESTED)
        {
            lwgeom_interrupt_reset();
            throw std::runtime_error("Wagyu interrupted");
        }
    }
} //namespace wagyu
} //namespace geometry
} //namespace mapbox



#endif /* LWGEOM_WAGYU_INTERRUPT_H */

