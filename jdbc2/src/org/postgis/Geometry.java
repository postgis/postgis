/*
 * Geometry.java
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
 */package org.postgis;

import java.io.Serializable;

/** The base class of all geometries */
public abstract class Geometry implements Serializable {

    // OpenGIS Geometry types as defined in the OGC WKB Spec
    // (May we replace this with an ENUM as soon as JDK 1.5
    //  has gained widespread usage?)

    /** Fake type for linear ring */
    public static final int LINEARRING = 0;
    /**
     * The OGIS geometry type number for points.
     */
    public static final int POINT = 1;

    /**
     * The OGIS geometry type number for lines.
     */
    public static final int LINESTRING = 2;

    /**
     * The OGIS geometry type number for polygons.
     */
    public static final int POLYGON = 3;

    /**
     * The OGIS geometry type number for aggregate points.
     */
    public static final int MULTIPOINT = 4;

    /**
     * The OGIS geometry type number for aggregate lines.
     */
    public static final int MULTILINESTRING = 5;

    /**
     * The OGIS geometry type number for aggregate polygons.
     */
    public static final int MULTIPOLYGON = 6;

    /**
     * The OGIS geometry type number for feature collections.
     */
    public static final int GEOMETRYCOLLECTION = 7;

    public static final String[] ALLTYPES = new String[]{
        "", // internally used LinearRing does not have any text in front of it
        "POINT",
        "LINESTRING",
        "POLYGON",
        "MULTIPOINT",
        "MULTILINESTRING",
        "MULTIPOLYGON",
        "GEOMETRYCOLLECTION"};

    /**
     * The Text representations of the geometry types
     */
    public static String getTypeString(int type) {
        if (type >= 0 && type <= 7) {
            return ALLTYPES[type];
        } else {
            throw new IllegalArgumentException("Unknown Geometry type" + type);
        }
    }

    // Properties common to all geometries
    /**
     * The dimensionality of this feature (2,3)
     */
    public int dimension;

    /**
     * Do we have a measure (4th dimension)
     */
    public boolean haveMeasure = false;

    /**
     * The OGIS geometry type of this feature. this is final as it never
     * changes, it is bound to the subclass of the instance.
     */
    public final int type;

    /**
     * The spacial reference system id of this geometry, default (no srid) is -1
     */
    public int srid = -1;

    /**
     * Constructor for subclasses
     * 
     * @param type has to be given by all subclasses.
     */
    protected Geometry(int type) {
        this.type = type;
    }

    /**
     * java.lang.Object hashCode implementation
     */
    public int hashCode() {
        return dimension | (type * 4) | (srid * 32);
    }

    /**
     * java.lang.Object equals implementation
     */
    public boolean equals(Object other) {
        return (other instanceof Geometry) && equals((Geometry) other);
    }

    /**
     * geometry specific equals implementation
     */
    public boolean equals(Geometry other) {
        boolean firstline = (other != null) && (this.dimension == other.dimension) && (this.type == other.type);
        boolean sridequals = (this.srid == other.srid);
        boolean measEquals = (this.haveMeasure == other.haveMeasure);
        boolean secondline = sridequals && measEquals;
        boolean classequals = other.getClass().equals(this.getClass());
        boolean equalsintern = this.equalsintern(other);
        boolean result = firstline
                && secondline
                && classequals && equalsintern;
        return result;
    }

    /**
     * Whether test coordinates for geometry - subclass specific code
     * 
     * Implementors can assume that dimensin, type, srid and haveMeasure are
     * equal, other != null and other is the same subclass.
     */
    protected abstract boolean equalsintern(Geometry other);

    /**
     * Return the number of Points of the geometry
     */
    public abstract int numPoints();

    /**
     * Get the nth Point of the geometry
     * 
     * @param n the index of the point, from 0 to numPoints()-1;
     * @throws ArrayIndexOutOfBoundsException in case of an emtpy geometry or
     *             bad index.
     */
    public abstract Point getPoint(int n);

    /**
     * Same as getPoint(0);
     */
    public abstract Point getFirstPoint();

    /**
     * Same as getPoint(numPoints()-1);
     */
    public abstract Point getLastPoint();

    /**
     * The OGIS geometry type number of this geometry.
     */
    public int getType() {
        return this.type;
    }

    /**
     * Return the Type as String
     */
    public String getTypeString() {
        return getTypeString(this.type);
    }

    /**
     * @return The dimensionality (eg, 2D or 3D) of this geometry.
     */
    public int getDimension() {
        return this.dimension;
    }

    /**
     * The OGIS geometry type number of this geometry.
     */
    public int getSrid() {
        return this.srid;
    }

    /**
     * Recursively sets the srid on this geometry and all contained
     * subgeometries
     */
    public void setSrid(int srid) {
        this.srid = srid;
    }

    public String toString() {
        StringBuffer sb = new StringBuffer();
        if (srid != -1) {
            sb.append("SRID=");
            sb.append(srid);
            sb.append(';');
        }
        outerWKT(sb, true);
        return sb.toString();
    }

    /**
     * Render the WKT version of this Geometry (without SRID) into the given
     * StringBuffer.
     */
    public void outerWKT(StringBuffer sb, boolean putM) {
        sb.append(getTypeString());
        if (putM && haveMeasure && dimension == 2) {
            sb.append('M');
        }
        mediumWKT(sb);
    }

    public final void outerWKT(StringBuffer sb) {
        outerWKT(sb, true);
    }
    
    /**
     * Render the WKT without the type name, but including the brackets into the
     * StringBuffer
     */
    protected void mediumWKT(StringBuffer sb) {
        sb.append('(');
        innerWKT(sb);
        sb.append(')');
    }

    /**
     * Render the "inner" part of the WKT (inside the brackets) into the
     * StringBuffer.
     */
    protected abstract void innerWKT(StringBuffer SB);

    /**
     * backwards compatibility method
     */
    public String getValue() {
        StringBuffer sb = new StringBuffer();
        mediumWKT(sb);
        return sb.toString();
    }

}