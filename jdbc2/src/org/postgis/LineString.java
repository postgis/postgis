package org.postgis;

import java.sql.SQLException;

public class LineString extends PointComposedGeom {

    double len = -1.;

    public LineString() {
        super(LINESTRING);
    }

    public LineString(Point[] points) {
        super(LINESTRING, points);
    }

    public LineString(String value) throws SQLException {
        super(LINESTRING, value);
    }

    public LineString reverse() {
        Point[] points = this.getPoints();
        int l = points.length;
        int i, j;
        Point[] p = new Point[l];
        for (i = 0, j = l - 1; i < l; i++, j--) {
            p[i] = points[j];
        }
        return new LineString(p);
    }

    public LineString concat(LineString other) {
        Point[] points = this.getPoints();
        Point[] opoints = other.getPoints();

        boolean cutPoint = this.getLastPoint() == null || this.getLastPoint().equals(other.getFirstPoint());
        int count = points.length + points.length - (cutPoint ? 1 : 0);
        Point[] p = new Point[count];

        // Maybe we should use System.arrayCopy here?
        int i, j;
        for (i = 0; i < points.length; i++) {
            p[i] = points[i];
        }
        if (!cutPoint) {
            p[i++] = other.getFirstPoint();
        }
        for (j = 1; j < opoints.length; j++, i++) {
            p[i] = opoints[j];
        }
        return new LineString(p);
    }

    public double length() {
        if (len < 0) {
            Point[] points = this.getPoints();
            if ((points == null) || (points.length < 2)) {
                len = 0;
            } else {
                double sum = 0;
                for (int i = 1; i < points.length; i++) {
                    sum += points[i - 1].distance(points[i]);
                }
                len = sum;
            }
        }
        return len;
    }
}