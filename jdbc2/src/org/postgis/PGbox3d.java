package org.postgis;

import org.postgresql.util.PGobject;
import org.postgresql.util.PGtokenizer;

import java.sql.SQLException;

/*
 * Updates Oct 2002 - data members made private - getLLB() and getURT() methods
 * added
 */

public class PGbox3d extends PGobject {

    /**
     * The lower left bottom corner of the box.
     */
    private Point llb;

    /**
     * The upper right top corner of the box.
     */
    private Point urt;

    public PGbox3d() {
        // do nothing.
    }

    public PGbox3d(Point llb, Point urt) {
        this.llb = llb;
        this.urt = urt;
    }

    public PGbox3d(String value) throws SQLException {
        setValue(value);
    }

    public void setValue(String value) throws SQLException {
        value = value.trim();
        if (value.startsWith("BOX3D")) {
            value = value.substring(5);
        }
        PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.trim()), ',');
        llb = new Point(t.getToken(0));
        urt = new Point(t.getToken(1));
    }

    public String getValue() {
        return "BOX3D (" + llb.getValue() + "," + urt.getValue() + ")";
    }

    public String toString() {
        return getValue();
    }

    public Object clone() {
        PGbox3d obj = new PGbox3d(llb, urt);
        obj.setType(type);
        return obj;
    }

    /** Returns the lower left bottom corner of the box as a Point object */
    public Point getLLB() {
        return llb;
    }

    /** Returns the upper right top corner of the box as a Point object */
    public Point getURT() {
        return urt;
    }

}