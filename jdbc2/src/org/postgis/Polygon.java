package org.postgis;

import org.postgresql.util.PGtokenizer;

import java.sql.SQLException;

public class Polygon extends ComposedGeom {

    public Polygon() {
        super(POLYGON);
    }

    public Polygon(LinearRing[] rings) {
        super(POLYGON, rings);
    }

    public Polygon(String value) throws SQLException {
        this();
        value = value.trim();
        if (value.indexOf("POLYGON") == 0) {
            value = value.substring(7).trim();
        }
        PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value), ',');
        int nrings = t.getSize();
        subgeoms = new LinearRing[nrings];
        for (int r = 0; r < nrings; r++) {
            subgeoms[r] = new LinearRing(t.getToken(r));
        }
        dimension = subgeoms[0].dimension;
    }

    public int numRings() {
        return subgeoms.length;
    }

    public LinearRing getRing(int idx) {
        if (idx >= 0 & idx < subgeoms.length) {
            return (LinearRing) subgeoms[idx];
        } else {
            return null;
        }
    }
}