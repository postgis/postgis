/*
 * GeometryCollection.java
 * 
 * $Id$
 * 
 * (C) 2004 Markus Schaber, logi-track ag, Zürich, Switzerland
 * 
 * This file currently is beta test code and licensed under the GNU GPL.
 */package org.postgis;

import org.postgresql.util.PGtokenizer;

import java.sql.SQLException;

/**
 * Geometry Collection class WARNING: Currently only implements empty
 * collections
 * 
 * @author schabi
 * 
 * $Id$
 */

public class GeometryCollection extends ComposedGeom {
    public static final String GeoCollID = "GEOMETRYCOLLECTION";
    public static final String EmptyColl = GeoCollID + "(EMPTY)";

    public GeometryCollection() {
        super(GEOMETRYCOLLECTION);
    }

    public GeometryCollection(Geometry[] geoms) {
        super(GEOMETRYCOLLECTION, geoms);
    }

    public GeometryCollection(String value) throws SQLException {
        this();
        value = value.trim();
        if (value.equals(EmptyColl)) {
            //Do nothing
        } else if (value.startsWith(GeoCollID)) {
            PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.substring(GeoCollID.length()).trim()), ',');
            int ngeoms = t.getSize();
            subgeoms = new Geometry[ngeoms];
            for (int p = 0; p < ngeoms; p++) {
                subgeoms[p] = PGgeometry.geomFromString(t.getToken(p));
            }
            dimension = subgeoms[0].dimension;
        } else {
            throw new SQLException("postgis.geocollection");
        }
    }

    protected void innerWKT(StringBuffer SB) {
        subgeoms[0].outerWKT(SB);
        for (int i = 1; i < subgeoms.length; i++) {
            SB.append(',');
            subgeoms[i].outerWKT(SB);
        }
    }
}