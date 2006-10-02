package org.postgis.pljava;

public class HelloWorld {
    public static String helloWorld() {
        return "Hello Postgis-World";
    }
    
    public static int getSize(PLJGeometry geom) {
        return geom.geom.getNumPoints();
    }
    
    public static String getString(PLJGeometry geom) {
        return geom.toString();
    }
}
