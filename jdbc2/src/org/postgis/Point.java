package org.postgis;

import org.postgresql.util.PGtokenizer;

import java.sql.SQLException;

public class Point extends Geometry {

    public int hashCode() {
        return super.hashCode() ^ hashCode(x) ^ hashCode(y) ^ hashCode(z) ^ hashCode(m);
    }

    public static int hashCode(double value) {
        long v = Double.doubleToLongBits(value);
        return (int) (v ^ (v >>> 32));
    }

    protected boolean equalsintern(Geometry otherg) {
        Point other = (Point) otherg;
        return x == other.x && y == other.y && ((dimension == 2) || (z == other.z))
                && ((haveMeasure == false) || (m == other.m));
    }

    public Point getPoint(int index) {
        if (index == 0) {
            return this;
        } else {
            throw new ArrayIndexOutOfBoundsException("Point only has a single Point! " + index);
        }
    }

    /** Optimized versions for this special case */
    public Point getFirstPoint() {
        return this;
    }

    /** Optimized versions for this special case */
    public Point getLastPoint() {
        return this;
    }

    public int numPoints() {
        return 1;
    }

    /**
     * The X coordinate of the point.
     */
    public double x;

    /**
     * The Y coordinate of the point.
     */
    public double y;

    /**
     * The Z coordinate of the point.
     */
    public double z;

    /**
     * The measure of the point.
     */
    public double m = 0.0;

    public Point() {
        super(POINT);
    }

    public Point(double x, double y, double z) {
        this();
        this.x = x;
        this.y = y;
        this.z = z;
        dimension = 3;
    }

    public Point(double x, double y) {
        this();
        this.x = x;
        this.y = y;
        this.z = 0.0;
        dimension = 2;
    }

    public Point(String value) throws SQLException {
        this();
        value = value.trim();
        if (value.indexOf("POINT") == 0) {
            value = value.substring(5).trim();
        }
        PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value), ' ');
        try {
            if (t.getSize() == 3) {
                x = Double.valueOf(t.getToken(0)).doubleValue();
                y = Double.valueOf(t.getToken(1)).doubleValue();
                z = Double.valueOf(t.getToken(2)).doubleValue();
                dimension = 3;
            } else {
                x = Double.valueOf(t.getToken(0)).doubleValue();
                y = Double.valueOf(t.getToken(1)).doubleValue();
                z = 0.0;
                dimension = 2;
            }
        } catch (NumberFormatException e) {
            throw new SQLException("postgis.Point" + e.toString());
        }
    }

    public void innerWKT(StringBuffer sb) {
        sb.append(x);
        sb.append(' ');
        sb.append(y);
        if (dimension == 3) {
            sb.append(' ');
            sb.append(z);
        }
        if (haveMeasure) {
            sb.append(' ');
            sb.append(m);
        }
    }

    public double getX() {
        return x;
    }

    public double getY() {
        return y;
    }

    public double getZ() {
        return z;
    }

    public double getM() {
        return m;
    }

    public void setX(double x) {
        this.x = x;
    }

    public void setY(double y) {
        this.y = y;
    }

    public void setZ(double z) {
        this.z = z;
    }

    public void setM(double m) {
        haveMeasure = true;
        this.m = m;
    }

    public void setX(int x) {
        this.x = x;
    }

    public void setY(int y) {
        this.y = y;
    }

    public void setZ(int z) {
        this.z = z;
    }

    public double distance(Point other) {
        double tx, ty, tz;
        if (this.dimension != other.dimension) {
            throw new IllegalArgumentException("Points have different dimensions!");
        }
        tx = this.x - other.x;
        switch (this.dimension) {
        case 1 :
            return Math.sqrt(tx * tx);
        case 2 :
            ty = this.y - other.y;
            return Math.sqrt(tx * tx + ty * ty);
        case 3 :
            ty = this.y - other.y;
            tz = this.z - other.z;
            return Math.sqrt(tx * tx + ty * ty + tz * tz);
        default :
            throw new AssertionError("Illegal dimension of Point" + this.dimension);
        }
    }
}