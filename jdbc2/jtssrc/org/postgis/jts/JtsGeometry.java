/*
 * JtsGeometry.java
 * 
 * Wrapper for PostgreSQL JDBC driver to allow transparent reading and writing
 * of JTS geometries
 * 
 * (C) 2005 Markus Schaber, markus.schaber@logix-tt.com
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

import gnu.trove.TIntObjectHashMap;

import java.sql.SQLException;

import org.postgresql.util.PGobject;

import com.vividsolutions.jts.geom.CoordinateSequenceFactory;
import com.vividsolutions.jts.geom.Geometry;
import com.vividsolutions.jts.geom.GeometryFactory;
import com.vividsolutions.jts.geom.PrecisionModel;
import com.vividsolutions.jts.geom.impl.PackedCoordinateSequenceFactory;
import com.vividsolutions.jts.io.WKTReader;

/**
 * JTS Geometry SQL wrapper. Supports PostGIS 1.x (lwgeom hexwkb) for writing
 * and both PostGIS 0.x (EWKT) and 1.x (lwgeom hexwkb) for reading.
 * 
 * @author Markus Schaber
 */

public class JtsGeometry extends PGobject {
    /* JDK 1.5 Serialization */
    private static final long serialVersionUID = 0x100;

    Geometry geom;

    final static JtsBinaryParser bp = new JtsBinaryParser();
    final static JtsBinaryWriter bw = new JtsBinaryWriter();
    final static PrecisionModel prec = new PrecisionModel();
    final static CoordinateSequenceFactory csfac = PackedCoordinateSequenceFactory.DOUBLE_FACTORY;

    private static TIntObjectHashMap readers = new TIntObjectHashMap();
    private static TIntObjectHashMap factories = new TIntObjectHashMap();

    static synchronized GeometryFactory getFactory(int SRID) {
        GeometryFactory factory = (GeometryFactory) factories.get(SRID);
        if (factory == null) {
            factory = new GeometryFactory(prec, SRID, csfac);
            factories.put(SRID, factory);
        }
        return factory;
    }

    static synchronized WKTReader getReader(int SRID) {
        WKTReader reader = (WKTReader) readers.get(SRID);
        if (reader == null) {
            reader = new WKTReader(getFactory(SRID));
            readers.put(SRID, reader);
        }
        return reader;
    }

    /** Constructor called by JDBC drivers */
    public JtsGeometry() {
        setType("geometry");
    }

    public JtsGeometry(Geometry geom) {
        this();
        this.geom = geom;
    }

    public JtsGeometry(String value) throws SQLException {
        this();
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
                // no srid := 0 in JTS world
                int srid = 0;
                // break up geometry into srid and wkt
                if (value.startsWith("SRID=")) {
                    String[] temp = value.split(";");
                    value = temp[1].trim();
                    srid = Integer.parseInt(temp[0].substring(5));
                }

                result = getReader(srid).read(value);
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
        return bw.writeHexed(getGeometry());
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
