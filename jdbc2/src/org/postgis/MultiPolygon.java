package org.postgis;

import org.postgresql.util.PGtokenizer;

import java.sql.SQLException;

public class MultiPolygon extends ComposedGeom {

    public MultiPolygon() {
        super(MULTIPOLYGON);
    }

    public MultiPolygon(Polygon[] polygons) {
        super(MULTIPOLYGON, polygons);
    }

    public MultiPolygon(String value) throws SQLException {
        this();
        value = value.trim();
        if (value.indexOf("MULTIPOLYGON") == 0) {
            PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.substring(12).trim()), ',');
            int npolygons = t.getSize();
            subgeoms = new Polygon[npolygons];
            for (int p = 0; p < npolygons; p++) {
                subgeoms[p] = new Polygon(t.getToken(p));
            }
            dimension = subgeoms[0].dimension;
        } else {
            throw new SQLException("postgis.multipolygongeometry");
        }
    }

    public int numPolygons() {
        return subgeoms.length;
    }

    public Polygon getPolygon(int idx) {
        if (idx >= 0 & idx < subgeoms.length) {
            return (Polygon) subgeoms[idx];
        } else {
            return null;
        }
    }

}