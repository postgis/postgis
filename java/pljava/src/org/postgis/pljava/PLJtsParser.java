/*
 * JtsBinaryParser.java
 * 
 * Binary Parser for JTS - relies on org.postgis V1.0.0+ package.
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
 * $Id: JtsBinaryParser.java 2407 2006-07-18 18:13:31Z mschaber $
 */
package org.postgis.pljava;

import java.sql.SQLException;
import java.sql.SQLInput;


import com.vividsolutions.jts.geom.Coordinate;
import com.vividsolutions.jts.geom.CoordinateSequence;
import com.vividsolutions.jts.geom.Geometry;
import com.vividsolutions.jts.geom.GeometryCollection;
import com.vividsolutions.jts.geom.LineString;
import com.vividsolutions.jts.geom.LinearRing;
import com.vividsolutions.jts.geom.MultiLineString;
import com.vividsolutions.jts.geom.MultiPoint;
import com.vividsolutions.jts.geom.MultiPolygon;
import com.vividsolutions.jts.geom.Point;
import com.vividsolutions.jts.geom.Polygon;
import com.vividsolutions.jts.geom.impl.PackedCoordinateSequence;

/**
 * Parse binary representation of geometries. Currently, only text rep (hexed)
 * implementation is tested.
 * 
 * It should be easy to add char[] and CharSequence ByteGetter instances,
 * although the latter one is not compatible with older jdks.
 * 
 * I did not implement real unsigned 32-bit integers or emulate them with long,
 * as both java Arrays and Strings currently can have only 2^31-1 elements
 * (bytes), so we cannot even get or build Geometries with more than approx.
 * 2^28 coordinates (8 bytes each).
 * 
 * @author Markus Schaber, markus.schaber@logix-tt.com
 * 
 */
public class PLJtsParser {

    /** Parse a geometry 
     * @throws SQLException */
    public Geometry parseGeometry(SQLInput data) throws SQLException {
        return parseGeometry(data, 0, false);
    }

    /** Parse with a known geometry factory 
     * @throws SQLException */
    protected Geometry parseGeometry(SQLInput data, int srid, boolean inheritSrid) throws SQLException {
        int typeword = data.readByte() & 0xFF;
        
        int realtype = typeword & 0x0F; // cut off high flag bits

        boolean haveBBox = (typeword & 0x80) != 0;
        boolean haveS = (typeword & 0x40) != 0;
        boolean haveZ = (typeword & 0x20) != 0;
        boolean haveM = (typeword & 0x10) != 0;

        if (haveBBox) {
            // skip bbox, currently ignored
            data.readFloat();
            data.readFloat();
            data.readFloat();
            data.readFloat();
        }
        
        if (haveS) {
            int newsrid = data.readInt();
            if (inheritSrid && newsrid != srid) {
                throw new IllegalArgumentException("Inconsistent srids in complex geometry: " + srid + ", " + newsrid);
            } else {
                srid = newsrid;
            }
        } else if (!inheritSrid) {
            srid = -1;
        }
       
        Geometry result;
        switch (realtype) {
        case org.postgis.Geometry.POINT:
            result = parsePoint(data, haveZ, haveM);
            break;
        case org.postgis.Geometry.LINESTRING:
            result = parseLineString(data, haveZ, haveM);
            break;
        case org.postgis.Geometry.POLYGON:
            result = parsePolygon(data, haveZ, haveM, srid);
            break;
        case org.postgis.Geometry.MULTIPOINT:
            result = parseMultiPoint(data, srid);
            break;
        case org.postgis.Geometry.MULTILINESTRING:
            result = parseMultiLineString(data, srid);
            break;
        case org.postgis.Geometry.MULTIPOLYGON:
            result = parseMultiPolygon(data, srid);
            break;
        case org.postgis.Geometry.GEOMETRYCOLLECTION:
            result = parseCollection(data, srid);
            break;
        default:
            throw new IllegalArgumentException("Unknown Geometry Type!");
        }
        
        result.setSRID(srid);
        
        return result;
    }

    private Point parsePoint(SQLInput data, boolean haveZ, boolean haveM) throws SQLException {
        double X = data.readDouble();
        double Y = data.readDouble();
        Point result;
        if (haveZ) {
            double Z = data.readDouble();
            result = PLJGeometry.geofac.createPoint(new Coordinate(X, Y, Z));
        } else {
            result = PLJGeometry.geofac.createPoint(new Coordinate(X, Y));
        }

        if (haveM) { // skip M value
            data.readDouble();
        }
        
        return result;
    }

    /** Parse an Array of "full" Geometries 
     * @throws SQLException */
    private void parseGeometryArray(SQLInput data, Geometry[] container, int srid) throws SQLException {
        for (int i = 0; i < container.length; i++) {
            container[i] = parseGeometry(data, srid, true);
        }
    }

    /**
     * Parse an Array of "slim" Points (without endianness and type, part of
     * LinearRing and Linestring, but not MultiPoint!
     * 
     * @param haveZ
     * @param haveM
     * @throws SQLException 
     */
    private CoordinateSequence parseCS(SQLInput data, boolean haveZ, boolean haveM) throws SQLException {
        int count = data.readInt();
        int dims = haveZ ? 3 : 2;
        CoordinateSequence cs = new PackedCoordinateSequence.Double(count, dims);

        for (int i = 0; i < count; i++) {
            for (int d = 0; d < dims; d++) {
                cs.setOrdinate(i, d, data.readDouble());
            }
            if (haveM) { // skip M value
                data.readDouble();
            }
        }
        return cs;
    }

    private MultiPoint parseMultiPoint(SQLInput data, int srid) throws SQLException {
        Point[] points = new Point[data.readInt()];
        parseGeometryArray(data, points, srid);
        return PLJGeometry.geofac.createMultiPoint(points);
    }

    private LineString parseLineString(SQLInput data, boolean haveZ, boolean haveM) throws SQLException {
        return PLJGeometry.geofac.createLineString(parseCS(data, haveZ, haveM));
    }

    private LinearRing parseLinearRing(SQLInput data, boolean haveZ, boolean haveM) throws SQLException {
        return PLJGeometry.geofac.createLinearRing(parseCS(data, haveZ, haveM));
    }

    private Polygon parsePolygon(SQLInput data, boolean haveZ, boolean haveM, int srid) throws SQLException {
        int holecount = data.readInt() - 1;
        LinearRing[] rings = new LinearRing[holecount];
        LinearRing shell = parseLinearRing(data, haveZ, haveM);
        shell.setSRID(srid);
        for (int i = 0; i < holecount; i++) {
            rings[i] = parseLinearRing(data, haveZ, haveM);
            rings[i].setSRID(srid);
        }
        return PLJGeometry.geofac.createPolygon(shell, rings);
    }

    private MultiLineString parseMultiLineString(SQLInput data, int srid) throws SQLException {
        int count = data.readInt();
        LineString[] strings = new LineString[count];
        parseGeometryArray(data, strings, srid);
        return PLJGeometry.geofac.createMultiLineString(strings);
    }

    private MultiPolygon parseMultiPolygon(SQLInput data, int srid) throws SQLException {
        int count = data.readInt();
        Polygon[] polys = new Polygon[count];
        parseGeometryArray(data, polys, srid);
        return PLJGeometry.geofac.createMultiPolygon(polys);
    }

    private GeometryCollection parseCollection(SQLInput data, int srid) throws SQLException {
        int count = data.readInt();
        Geometry[] geoms = new Geometry[count];
        parseGeometryArray(data, geoms, srid);
        return PLJGeometry.geofac.createGeometryCollection(geoms);
    }
}
