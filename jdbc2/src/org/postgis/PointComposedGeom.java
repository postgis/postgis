/*
 * ComposedGeom.java
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
 * PointComposedGeom - base class for all composed geoms that contain only
 * points.
 * 
 * @author schabi
 *  
 */

public abstract class PointComposedGeom extends ComposedGeom {

    protected PointComposedGeom(int type) {
        super(type);
    }

    protected PointComposedGeom(int type, Point[] points) {
        super(type, points);
    }

    public PointComposedGeom(int type, String value) throws SQLException {
        this(type);
        value = value.trim();
        if (value.indexOf(typestring) == 0) {
            value = value.substring(typestring.length()).trim();
        }
        if (value.charAt(0) == '(') {
            PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value), ',');
            int npoints = t.getSize();
            subgeoms = new Point[npoints];
            for (int p = 0; p < npoints; p++) {
                subgeoms[p] = new Point(t.getToken(p));
            }
            dimension = subgeoms[0].dimension;
        } else {
            throw new SQLException("postgis.multipointgeometry");
        }
    }

    protected void innerWKT(StringBuffer sb) {
        subgeoms[0].innerWKT(sb);
        for (int i = 1; i < subgeoms.length; i++) {
            sb.append(',');
            subgeoms[i].innerWKT(sb);
        }
    }

    /**
     * optimized version
     */
    public int numPoints() {
        return subgeoms.length;
    }

    /**
     * optimized version
     */
    public Point getPoint(int idx) {
        if (idx >= 0 & idx < subgeoms.length) {
            return (Point) subgeoms[idx];
        } else {
            return null;
        }
    }

    /** Get the underlying Point array */
    public Point[] getPoints() {
        return (Point[]) subgeoms;
    }
}