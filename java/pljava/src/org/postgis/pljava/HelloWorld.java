package org.postgis.pljava;

import org.postgresql.pljava.example.Point;

public class HelloWorld {
    public static String helloWorld() {
        return "Hello Postgis-World";
    }
    
    public static int getSize(PLJGeometry geom) {
        return geom.rep.length();
    }
    
    public static String getHex(PLJGeometry geom) {
        return geom.rep.toString();
    }

    public static PLJGeometry getPoint() {
        return new PLJGeometry();
    }
    
    public static int getHash(Point p) {
        return p.hashCode();
    }
    
    public static int getHash(PLJGeometry p) {
        return p.hashCode();
    }
}
