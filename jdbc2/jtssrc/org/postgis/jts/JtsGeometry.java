/*
 * JtsGeometry.java
 * 
 * Wrapper for PostgreSQL JDBC driver to allow transparent reading and writing
 * of JTS geometries
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

package org.postgis.jts;

import com.vividsolutions.jts.geom.Geometry;
import com.vividsolutions.jts.io.WKTReader;

import org.postgresql.util.PGobject;

import java.sql.SQLException;

/**
 * JTS Geometry SQL wrapper
 * 
 * @author Markus Schaber
 */

public class JtsGeometry extends PGobject {

    Geometry geom;

    final static WKTReader reader = new WKTReader();
    final static JtsBinaryParser bp = new JtsBinaryParser();


    public JtsGeometry() {
        //Constructor called by JDBC drivers
    }

    public JtsGeometry(Geometry geom) {
        this.geom = geom;
    }

    public JtsGeometry(String value) throws SQLException {
        setValue(value);
    }

    public void setValue(String value) throws SQLException {
        geom = geomFromString(value);
    }

    public static Geometry geomFromString(String value) throws SQLException {
        try {
            value = value.trim();
            if (value.startsWith("00") || value.startsWith("01")) {
                return bp.parse(value);
            } else {
                Geometry result;
                int srid = -1;
                //break up geometry into srid and wkt
                if (value.startsWith("SRID=")) {
                    String[] temp = value.split(";");
                    value = temp[1].trim();
                    srid = Integer.parseInt(temp[0].substring(5));
                }
                result = reader.read(value);
                result.setSRID(srid);
                return result;
            }
        } catch (Exception E) {
            E.printStackTrace();
            throw new SQLException("Error parsing SQL data:" + E);
        }
    }

    public Geometry getGeometry() {
        return geom;
    }

    public String toString() {
        return geom.toString();
    }

    public String getValue() {
        return geom.toString();
    }

    public Object clone() {
        JtsGeometry obj = new JtsGeometry(geom);
        obj.setType(type);
        return obj;
    }

    public boolean equals(Object obj) {
        if (obj instanceof JtsGeometry)
            return ((JtsGeometry) obj).getValue().equals(geom);
        return false;
    }
}