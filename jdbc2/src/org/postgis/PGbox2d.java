package org.postgis;

import java.sql.SQLException;

public class PGbox2d extends PGboxbase {

    public PGbox2d() {
        super();
    }

    public PGbox2d(Point llb, Point urt) {
        super(llb, urt);
    }

    public PGbox2d(String value) throws SQLException {
        super(value);
    }

    public void setValue(String value) throws SQLException {
        super.setValue(value);

        if (llb.dimension!=2 || urt.dimension!=2) {
            throw new SQLException("PGbox2d is only allowed to have 2 dimensions!");
        }
    }

    public String getPrefix() {
        return "BOX";
    }

    public String getPGtype() {
        return "box2d";
    }

    protected PGboxbase newInstance() {
        return new PGbox2d();
    }
}
