package org.postgis;

import java.sql.SQLException;

public class PGbox3d extends PGboxbase {
    public PGbox3d() {
        super();
    }

    public PGbox3d(Point llb, Point urt) {
        super(llb, urt);
    }

    public PGbox3d(String value) throws SQLException {
        super(value);
    }

    public String getPrefix() {
        return ("BOX3D");
    }

    public String getPGtype() {
        return ("box3d");
    }

    protected PGboxbase newInstance() {
        return new PGbox3d();
    }
}
