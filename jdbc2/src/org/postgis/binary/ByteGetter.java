/*
 * ByteGetter.java
 * 
 * (C) 15.01.2005 Markus Schaber, Logi-Track ag, CH 8001 Zuerich
 * 
 * $Id$
 */
package org.postgis.binary;

public abstract class ByteGetter {
    /**
     * Get ab byte.
     * 
     * @return The result is returned as Int to eliminate sign problems when
     *         or'ing several values together.
     */
    public abstract int get(int index);

    public static class BinaryByteGetter extends ByteGetter {
        private byte[] array;

        public BinaryByteGetter(byte[] array) {
            this.array = array;
        }

        public int get(int index) {
            return array[index] & 0xFF; //mask out sign-extended bits.
        }
    }

    public static class StringByteGetter extends ByteGetter {
        private String rep;

        public StringByteGetter(String rep) {
            this.rep = rep;
        }

        public int get(int index) {
            index *= 2;
            int high = unhex(rep.charAt(index));
            int low = unhex(rep.charAt(index + 1));
            return (high << 4) + low;
        }

        public static byte unhex(char c) {
            if (c >= '0' && c <= '9') {
                return (byte) (c - '0');
            } else if (c >= 'A' && c <= 'F') {
                return (byte) (c - 'A'+10);
            } else if (c >= 'a' && c <= 'f') {
                return (byte) (c - 'a'+10);
            } else {
                throw new IllegalArgumentException("No valid Hex char " + c);
            }
        }
    }
}