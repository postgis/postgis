/*
 * ComposedGeom.java
 * 
 * (C) 15.01.2005 Markus Schaber, Logi-Track ag, CH 8001 Zuerich
 * 
 * This file is licensed under GNU GPL.
 * 
 * $Id$
 */
package org.postgis;

import java.util.Iterator;

/**
 * ComposedGeom - Abstract base class for all Geometries that are composed out
 * of other Geometries.
 * 
 * In fact, this currently are all Geometry subclasses except Point.
 * 
 * @author schabi
 * 
 *  
 */
public abstract class ComposedGeom extends Geometry {
    public static final Geometry[] EMPTY = new Geometry[0];

    /**
     * The Array containing the geometries
     * 
     * This is only to be exposed by concrete subclasses, to retain type safety.
     */
    protected Geometry[] subgeoms = EMPTY;

    /**
     * @param type
     */
    public ComposedGeom(int type) {
        super(type);
    }

    public Geometry getSubGeometry(int index) {
        return subgeoms[index];
    }

    protected ComposedGeom(int type, Geometry[] geoms) {
        this(type);
        this.subgeoms = geoms;
        if (geoms.length>0) {
        dimension = geoms[0].dimension;
        } else {
            dimension=0;
        }
    }

    protected boolean equalsintern(Geometry other) {
        // Can be assumed to be the same subclass of Geometry, so it must be a
        // ComposedGeom, too.
        ComposedGeom cother = (ComposedGeom) other;

        if (cother.subgeoms == null && subgeoms == null) {
            return true;
        } else if (cother.subgeoms == null || subgeoms == null) {
            return false;
        } else if (cother.subgeoms.length != subgeoms.length) {
            return false;
        } else if (subgeoms.length == 0) {
            return true;
        } else {
            for (int i = 0; i < subgeoms.length; i++) {
                if (!cother.subgeoms[i].equalsintern(this.subgeoms[i])) {
                    return false;
                }
            }
        }
        return true;
    }

    public int numPoints() {
        if ((subgeoms == null) || (subgeoms.length == 0)) {
            return 0;
        } else {
            int result = 0;
            for (int i = 0; i < subgeoms.length; i++) {
                result += subgeoms[i].numPoints();
            }
            return result;
        }
    }

    public Point getPoint(int n) {
        if (n < 0) {
            throw new ArrayIndexOutOfBoundsException("Negative index not allowed");
        } else if ((subgeoms == null) || (subgeoms.length == 0)) {
            throw new ArrayIndexOutOfBoundsException("Empty Geometry has no Points!");
        } else {
            for (int i = 0; i < subgeoms.length; i++) {

                Geometry current = subgeoms[i];
                int np = current.numPoints();
                if (n < np) {
                    return current.getPoint(n);
                } else {
                    n -= np;
                }
            }
            throw new ArrayIndexOutOfBoundsException("Index too large!");
        }
    }

    /**
     * Optimized version
     */
    public Point getLastPoint() {
        if ((subgeoms == null) || (subgeoms.length == 0)) {
            throw new ArrayIndexOutOfBoundsException("Empty Geometry has no Points!");
        } else {
            return subgeoms[subgeoms.length - 1].getLastPoint();
        }
    }

    /**
     * Optimized version
     */
    public Point getFirstPoint() {
        if ((subgeoms == null) || (subgeoms.length == 0)) {
            throw new ArrayIndexOutOfBoundsException("Empty Geometry has no Points!");
        } else {
            return subgeoms[0].getFirstPoint();
        }
    }

    public Iterator iterator() {
        return java.util.Arrays.asList(subgeoms).iterator();
    }

    public boolean isEmpty() {
        return (subgeoms == null) || (subgeoms.length == 0);
    }

    protected void mediumWKT(StringBuffer sb) {
        if ((subgeoms == null) || (subgeoms.length == 0)) {
            sb.append(" EMPTY");
        } else {
            sb.append('(');
            innerWKT(sb);
            sb.append(')');
        }
   }

    protected void innerWKT(StringBuffer sb) {
        subgeoms[0].mediumWKT(sb);
        for (int i = 1; i < subgeoms.length; i++) {
            sb.append(',');
            subgeoms[i].mediumWKT(sb);
        }
    }

    // Hashing - still buggy!
    boolean nohash = true;
    int hashcode = 0;

    public int hashCode() {
        if (nohash) {
            hashcode = super.hashCode() ^ subgeoms.hashCode();
            nohash = false;
        }
        return hashcode;
    }
}