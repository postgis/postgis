/*
 * GeometryCollection.java
 * 
 * PostGIS extension for PostgreSQL JDBC driver - geometry model
 * 
 * (C) 2004 Paul Ramsey, pramsey@refractions.net
 * 
 * (C) 2005 Markus Schaber, schabios@logi-track.com
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 2.1 of the License.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA or visit the web at
 * http://www.gnu.org.
 * 
 * $Id$
 */

package org.postgis;

import org.postgresql.util.PGtokenizer;

import java.sql.SQLException;

/**
 * Geometry Collection class WARNING: Currently only implements empty
 * collections
 * 
 * @author schabi
 * 
 * $Id$
 */

public class GeometryCollection extends ComposedGeom {
    public static final String GeoCollID = "GEOMETRYCOLLECTION";
    public static final String EmptyColl = GeoCollID + "(EMPTY)";

    public GeometryCollection() {
        super(GEOMETRYCOLLECTION);
    }

    public GeometryCollection(Geometry[] geoms) {
        super(GEOMETRYCOLLECTION, geoms);
    }

    public GeometryCollection(String value) throws SQLException {
        this();
        value = value.trim();
        if (value.equals(EmptyColl)) {
            //Do nothing
        } else if (value.startsWith(GeoCollID)) {
            PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.substring(
                    GeoCollID.length()).trim()), ',');
            int ngeoms = t.getSize();
            subgeoms = new Geometry[ngeoms];
            for (int p = 0; p < ngeoms; p++) {
                subgeoms[p] = PGgeometry.geomFromString(t.getToken(p));
            }
            dimension = subgeoms[0].dimension;
        } else {
            throw new SQLException("postgis.geocollection");
        }
    }

    protected void innerWKT(StringBuffer SB) {
        subgeoms[0].outerWKT(SB);
        for (int i = 1; i < subgeoms.length; i++) {
            SB.append(',');
            subgeoms[i].outerWKT(SB);
        }
    }
}