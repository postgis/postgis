/*
 * JtsBinaryWriter.java
 * 
 * PostGIS extension for PostgreSQL JDBC driver - Binary Writer
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

import com.vividsolutions.jts.geom.CoordinateSequence;
import com.vividsolutions.jts.geom.Geometry;
import com.vividsolutions.jts.geom.GeometryCollection;
import com.vividsolutions.jts.geom.LineString;
import com.vividsolutions.jts.geom.MultiLineString;
import com.vividsolutions.jts.geom.MultiPoint;
import com.vividsolutions.jts.geom.MultiPolygon;
import com.vividsolutions.jts.geom.Point;
import com.vividsolutions.jts.geom.Polygon;

import org.postgis.binary.ByteSetter;
import org.postgis.binary.ValueSetter;

/**
 * Create binary representation of geometries. Currently, only text rep (hexed)
 * implementation is tested. Supports only 2 dimensional geometries.
 * 
 * It should be easy to add char[] and CharSequence ByteGetter instances,
 * although the latter one is not compatible with older jdks.
 * 
 * I did not implement real unsigned 32-bit integers or emulate them with long,
 * as both java Arrays and Strings currently can have only 2^31-1 elements
 * (bytes), so we cannot even get or build Geometries with more than approx.
 * 2^28 coordinates (8 bytes each).
 * 
 * @author markus.schaber@logi-track.com
 * 
 */
public class JtsBinaryWriter {

    /**
     * Get the appropriate ValueGetter for my endianness
     * 
     * @param bytes
     *            The appropriate Byte Getter
     * 
     * @return the ValueGetter
     */
    public static ValueSetter valueSetterForEndian(ByteSetter bytes, byte endian) {
        if (endian == ValueSetter.XDR.NUMBER) { // XDR
            return new ValueSetter.XDR(bytes);
        } else if (endian == ValueSetter.NDR.NUMBER) {
            return new ValueSetter.NDR(bytes);
        } else {
            throw new IllegalArgumentException("Unknown Endian type:" + endian);
        }
    }

    /**
     * Write a hex encoded geometry
     * 
     * Is synchronized to protect offset counter. (Unfortunately, Java does not
     * have neither call by reference nor multiple return values.) This is a
     * TODO item.
     * 
     * Currently, geometries with more than 2 dimensions and measures are not
     * cleanly supported, but SRID is honored.
     */
    public synchronized String writeHexed(Geometry geom, byte REP) {
        int length = estimateBytes(geom);
        ByteSetter.StringByteSetter bytes = new ByteSetter.StringByteSetter(length);
        writeGeometry(geom, valueSetterForEndian(bytes, REP));
        return bytes.result();
    }

    public synchronized String writeHexed(Geometry geom) {
        return writeHexed(geom, ValueSetter.NDR.NUMBER);
    }

    /**
     * Write a binary encoded geometry.
     * 
     * Is synchronized to protect offset counter. (Unfortunately, Java does not
     * have neither call by reference nor multiple return values.) This is a
     * TODO item.
     * 
     * Currently, geometries with more than 2 dimensions and measures are not
     * cleanly supported, but SRID is honored.
     */
    public synchronized byte[] writeBinary(Geometry geom, byte REP) {
        int length = estimateBytes(geom);
        ByteSetter.BinaryByteSetter bytes = new ByteSetter.BinaryByteSetter(length);
        writeGeometry(geom, valueSetterForEndian(bytes, REP));
        return bytes.result();
    }

    public synchronized byte[] writeBinary(Geometry geom) {
        return writeBinary(geom, ValueSetter.NDR.NUMBER);
    }

    /** Parse a geometry starting at offset. */
    protected void writeGeometry(Geometry geom, ValueSetter dest) {
        final int dimension = getCoordDim(geom);
        if (dimension < 2 || dimension > 4) {
            throw new IllegalArgumentException("Unsupported geometry dimensionality: " + dimension);
        }
        // write endian flag
        dest.setByte(dest.endian);

        // write typeword
        final int plaintype = getWKBType(geom);
        int typeword = plaintype;
        if (dimension == 3 || dimension == 4) {
            typeword |= 0x80000000;
        }
        if (dimension == 4) {
            typeword |= 0x40000000;
        }
        if (checkSrid(geom)) {
            typeword |= 0x20000000;
        }

        dest.setInt(typeword);

        if (checkSrid(geom)) {
            dest.setInt(geom.getSRID());
        }

        switch (plaintype) {
        case org.postgis.Geometry.POINT:
            writePoint((Point) geom, dest);
            break;
        case org.postgis.Geometry.LINESTRING:
            writeLineString((LineString) geom, dest);
            break;
        case org.postgis.Geometry.POLYGON:
            writePolygon((Polygon) geom, dest);
            break;
        case org.postgis.Geometry.MULTIPOINT:
            writeMultiPoint((MultiPoint) geom, dest);
            break;
        case org.postgis.Geometry.MULTILINESTRING:
            writeMultiLineString((MultiLineString) geom, dest);
            break;
        case org.postgis.Geometry.MULTIPOLYGON:
            writeMultiPolygon((MultiPolygon) geom, dest);
            break;
        case org.postgis.Geometry.GEOMETRYCOLLECTION:
            writeCollection((GeometryCollection) geom, dest);
            break;
        default:
            throw new IllegalArgumentException("Unknown Geometry Type: " + plaintype);
        }
    }

    public static int getWKBType(Geometry geom) {
        if (geom instanceof Point) {
            return org.postgis.Geometry.POINT;
        } else if (geom instanceof com.vividsolutions.jts.geom.LineString) {
            return org.postgis.Geometry.LINESTRING;
        } else if (geom instanceof com.vividsolutions.jts.geom.Polygon) {
            return org.postgis.Geometry.POLYGON;
        } else if (geom instanceof MultiPoint) {
            return org.postgis.Geometry.MULTIPOINT;
        } else if (geom instanceof MultiLineString) {
            return org.postgis.Geometry.MULTILINESTRING;
        } else if (geom instanceof com.vividsolutions.jts.geom.MultiPolygon) {
            return org.postgis.Geometry.MULTIPOLYGON;
        } else if (geom instanceof com.vividsolutions.jts.geom.GeometryCollection) {
            return org.postgis.Geometry.GEOMETRYCOLLECTION;
        } else {
            throw new IllegalArgumentException("Unknown Geometry Type: " + geom.getClass().getName());
        }
    }

    /**
     * Writes a "slim" Point (without endiannes, srid ant type, only the
     * ordinates and measure. Used by writeGeometry.
     */
    private void writePoint(Point geom, ValueSetter dest) {
        writeCoordinates(geom.getCoordinateSequence(), getCoordDim(geom), dest);
    }

    /**
     * Write a Coordinatesequence, part of LinearRing and Linestring, but not
     * MultiPoint!
     * 
     * @param haveZ
     * @param haveM
     */
    private void writeCoordinates(CoordinateSequence seq, int dims, ValueSetter dest) {
        for (int i = 0; i < seq.size(); i++) {
            for (int d = 0; d < dims; d++) {
                dest.setDouble(seq.getOrdinate(i, d));
            }
        }
    }

    private void writeMultiPoint(MultiPoint geom, ValueSetter dest) {
        dest.setInt(geom.getNumPoints());
        for (int i = 0; i < geom.getNumPoints(); i++) {
            writeGeometry(geom.getGeometryN(i), dest);
        }
    }

    private void writeLineString(LineString geom, ValueSetter dest) {
        dest.setInt(geom.getNumPoints());
        writeCoordinates(geom.getCoordinateSequence(), getCoordDim(geom), dest);
    }

    private void writePolygon(Polygon geom, ValueSetter dest) {
        dest.setInt(geom.getNumInteriorRing() + 1);
        writeLineString(geom.getExteriorRing(), dest);
        for (int i = 0; i < geom.getNumInteriorRing(); i++) {
            writeLineString(geom.getInteriorRingN(i), dest);
        }
    }

    private void writeMultiLineString(MultiLineString geom, ValueSetter dest) {
        writeGeometryArray(geom, dest);
    }

    private void writeMultiPolygon(MultiPolygon geom, ValueSetter dest) {
        writeGeometryArray(geom, dest);
    }

    private void writeCollection(GeometryCollection geom, ValueSetter dest) {
        writeGeometryArray(geom, dest);
    }

    private void writeGeometryArray(Geometry geom, ValueSetter dest) {
        dest.setInt(geom.getNumGeometries());
        for (int i = 0; i < geom.getNumGeometries(); i++) {
            writeGeometry(geom.getGeometryN(i), dest);
        }
    }

    /** Estimate how much bytes a geometry will need in WKB. */
    protected int estimateBytes(Geometry geom) {
        int result = 0;

        // write endian flag
        result += 1;

        // write typeword
        result += 4;

        if (checkSrid(geom)) {
            result += 4;
        }

        switch (getWKBType(geom)) {
        case org.postgis.Geometry.POINT:
            result += estimatePoint((Point) geom);
            break;
        case org.postgis.Geometry.LINESTRING:
            result += estimateLineString((LineString) geom);
            break;
        case org.postgis.Geometry.POLYGON:
            result += estimatePolygon((Polygon) geom);
            break;
        case org.postgis.Geometry.MULTIPOINT:
            result += estimateMultiPoint((MultiPoint) geom);
            break;
        case org.postgis.Geometry.MULTILINESTRING:
            result += estimateMultiLineString((MultiLineString) geom);
            break;
        case org.postgis.Geometry.MULTIPOLYGON:
            result += estimateMultiPolygon((MultiPolygon) geom);
            break;
        case org.postgis.Geometry.GEOMETRYCOLLECTION:
            result += estimateCollection((GeometryCollection) geom);
            break;
        default:
            throw new IllegalArgumentException("Unknown Geometry Type: " + getWKBType(geom));
        }
        return result;
    }

    private boolean checkSrid(Geometry geom) {
        final int srid = geom.getFactory().getSRID();
        // SRID is default 0 with jts geometries
        return (srid != -1) && (srid != 0);
    }

    private int estimatePoint(Point geom) {
        return 8 * getCoordDim(geom);
    }

    /** Write an Array of "full" Geometries */
    private int estimateGeometryArray(Geometry container) {
        int result = 0;
        for (int i = 0; i < container.getNumGeometries(); i++) {
            result += estimateBytes(container.getGeometryN(i));
        }
        return result;
    }

    /**
     * Estimate an Array of "slim" Points (without endianness and type, part of
     * LinearRing and Linestring, but not MultiPoint!
     * 
     * @param haveZ
     * @param haveM
     */
    private int estimatePointArray(int length, Point example) {
        // number of points
        int result = 4;

        // And the amount of the points itsself, in consistent geometries
        // all points have equal size.
        if (length > 0) {
            result += length * estimatePoint(example);
        }
        return result;
    }

    /** Estimate an array of "fat" Points */
    private int estimateMultiPoint(MultiPoint geom) {
        // int size
        int result = 4;
        if (geom.getNumGeometries() > 0) {
            // We can shortcut here, compared to estimateGeometryArray, as all
            // subgeoms have the same fixed size
            result += geom.getNumGeometries() * estimateBytes(geom.getGeometryN(0));
        }
        return result;
    }

    private int estimateLineString(LineString geom) {
        if (geom == null || geom.getNumGeometries() == 0) {
            return 0;
        } else {
            return estimatePointArray(geom.getNumPoints(), geom.getStartPoint());
        }
    }

    private int estimatePolygon(Polygon geom) {
        // int length
        int result = 4;
        result += estimateLineString(geom.getExteriorRing());
        for (int i = 0; i < geom.getNumInteriorRing(); i++) {
            result += estimateLineString(geom.getInteriorRingN(i));
        }
        return result;
    }

    private int estimateMultiLineString(MultiLineString geom) {
        // 4-byte count + subgeometries
        return 4 + estimateGeometryArray(geom);
    }

    private int estimateMultiPolygon(MultiPolygon geom) {
        // 4-byte count + subgeometries
        return 4 + estimateGeometryArray(geom);
    }

    private int estimateCollection(GeometryCollection geom) {
        // 4-byte count + subgeometries
        return 4 + estimateGeometryArray(geom);
    }

    public static final int getCoordDim(Geometry geom) {
        // TODO: Fix geometries with more dimensions
        // geom.getFactory().getCoordinateSequenceFactory()
        if (geom == null) {
            return 0;
        } else {
            return 2;
        }
    }
}
