/*
 * BitTest.java
 * 
 * (C) 17.01.2005 Markus Schaber, Logi-Track ag, CH 8001 Zuerich
 * 
 * $Id$
 */
package examples;

import org.postgis.binary.ByteGetter;
import org.postgis.binary.ValueGetter;

/**
 * BitTest
 * 
 * 
 * @author schabi
 * 
 *  
 */
public class BitTest {

    public static void main(String[] args) {
        long doubleToLongBits = Double.doubleToLongBits(10.0);
        System.out.println(doubleToLongBits);
        String toHexString = Long.toHexString(doubleToLongBits);
        System.out.println(toHexString);
        long parseLong = Long.parseLong(toHexString, 16);
        System.out.println(parseLong);
        System.out.println(Double.longBitsToDouble(parseLong));

        ValueGetter vg = new ValueGetter.NDR((new ByteGetter.StringByteGetter(toHexString)));
        System.out.println(Long.toHexString(vg.getLong()));
        vg = new ValueGetter.XDR((new ByteGetter.StringByteGetter(toHexString)));
        System.out.println(Long.toHexString(vg.getLong()));
    }
}