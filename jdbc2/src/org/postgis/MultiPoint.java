package org.postgis;

import java.sql.SQLException;

public class MultiPoint extends PointComposedGeom {

    public MultiPoint() {
        super(MULTIPOINT);
    }

    public MultiPoint(Point[] points) {
        super(MULTIPOINT, points);
    }

    public MultiPoint(String value) throws SQLException {
        super(MULTIPOINT, value);
    }
}