package org.postgis;

import org.postgresql.util.PGtokenizer;

import java.sql.SQLException;

/**
 * This represents the LinearRing GIS datatype. This type is used to construct
 * the polygon types, but is not stored or retrieved directly from the database.
 */
public class LinearRing extends PointComposedGeom {

    public LinearRing(Point[] points) {
        super(LINEARRING, points);
    }

    /**
     * This is called to construct a LinearRing from the PostGIS string
     * representation of a ring.
     * 
     * @param value Definition of this ring in the PostGIS string format.
     */
    public LinearRing(String value) throws SQLException {
        super(LINEARRING);
        PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.trim()), ',');
        int npoints = t.getSize();
        Point[] points = new Point[npoints];
        for (int p = 0; p < npoints; p++) {
            points[p] = new Point(t.getToken(p));
        }
        dimension = points[0].dimension;
        this.subgeoms = points;
    }
}