/*
 * Polygon.java
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

public class Polygon extends ComposedGeom {

    public Polygon() {
        super(POLYGON);
    }

    public Polygon(LinearRing[] rings) {
        super(POLYGON, rings);
    }

    public Polygon(String value) throws SQLException {
        this(value, false);
    }
    
    public Polygon(String value, boolean haveM) throws SQLException{
        this();
        value = value.trim();
        if (value.indexOf(typestring) == 0) {
            int pfxlen = typestring.length();
            if (value.charAt(pfxlen) == 'M') {
                pfxlen += 1;
                haveM = true;
            }
            value = value.substring(pfxlen).trim();
        }
        PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value), ',');
        int nrings = t.getSize();
        subgeoms = new LinearRing[nrings];
        for (int r = 0; r < nrings; r++) {
            subgeoms[r] = new LinearRing(t.getToken(r), haveM);
        }
        dimension = subgeoms[0].dimension;
        // fetch haveMeasure from subpoint because haveM does only work with
        // 2d+M, not with 3d+M geometries
        this.haveMeasure = subgeoms[0].haveMeasure;
    }

    public int numRings() {
        return subgeoms.length;
    }

    public LinearRing getRing(int idx) {
        if (idx >= 0 & idx < subgeoms.length) {
            return (LinearRing) subgeoms[idx];
        } else {
            return null;
        }
    }
}