/*
 * MultiLineString.java
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

public class MultiLineString extends ComposedGeom {

    double len = -1;

    public int hashCode() {
        return super.hashCode() ^ (int) this.length();
    }

    public MultiLineString() {
        super(MULTILINESTRING);
    }

    public MultiLineString(LineString[] lines) {
        super(MULTILINESTRING, lines);
    }

    public MultiLineString(String value) throws SQLException {
        this();
        value = value.trim();
        if (value.indexOf("MULTILINESTRING") == 0) {
            PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.substring(15).trim()), ',');
            int nlines = t.getSize();
            subgeoms = new LineString[nlines];
            for (int p = 0; p < nlines; p++) {
                subgeoms[p] = new LineString(t.getToken(p));
            }
            dimension = subgeoms[0].dimension;
        } else {
            throw new SQLException("postgis.multilinestringgeometry");
        }
    }

    public int numLines() {
        return subgeoms.length;
    }

    public LineString[] getLines() {
        return (LineString[]) subgeoms.clone();
    }

    public LineString getLine(int idx) {
        if (idx >= 0 & idx < subgeoms.length) {
            return (LineString) subgeoms[idx];
        } else {
            return null;
        }
    }

    public double length() {
        if (len < 0) {
            LineString[] lines = (LineString[]) subgeoms;
            if (lines.length < 1) {
                len = 0;
            } else {
                double sum = 0;
                for (int i = 0; i < lines.length; i++) {
                    sum += lines[i].length();
                }
                len = sum;
            }
        }
        return len;
    }
}