/*
 * ValueGetter.java
 * 
 * Currently, this is not thread safe!
 * 
 * (C) 15.01.2005 Markus Schaber, Logi-Track ag, CH 8001 Zuerich
 * 
 * $Id$
 */
package org.postgis.binary;

public abstract class ValueGetter {
    ByteGetter data;
    int position;
    public final byte endian;

    public ValueGetter(ByteGetter data, byte endian) {
        this.data = data;
        this.endian = endian;
    }

    /**
     * Get a byte, should be equal for all endians
     */
    public byte getByte() {
        return (byte) data.get(position++);
    }

    public int getInt() {
        int res = getInt(position);
        position += 4;
        return res;
    }

    public long getLong() {
        long res = getLong(position);
        position += 8;
        return res;
    }

    /** Get a 32-Bit integer */
    protected abstract int getInt(int index);

    /**
     * Get a long value. This is not needed directly, but as a nice side-effect
     * from GetDouble.
     */
    protected abstract long getLong(int index);

    /**
     * Get a double.
     */
    public double getDouble() {
        long bitrep = getLong();
        return Double.longBitsToDouble(bitrep);
    }

    public static class XDR extends ValueGetter {
        public static final byte NUMBER = 0;

        public XDR(ByteGetter data) {
            super(data, NUMBER);
        }

        protected int getInt(int index) {
            return (data.get(index) << 24) + (data.get(index + 1) << 16) + (data.get(index + 2) << 8)
                    + data.get(index + 3);
        }

        protected long getLong(int index) {
            return ((long) data.get(index) << 56) + ((long) data.get(index + 1) << 48)
                    + ((long) data.get(index + 2) << 40) + ((long) data.get(index + 3) << 32)
                    + ((long) data.get(index + 4) << 24) + ((long) data.get(index + 5) << 16)
                    + ((long) data.get(index + 6) << 8) + ((long) data.get(index + 7) << 0);
        }
    }

    public static class NDR extends ValueGetter {
        public static final byte NUMBER = 1;

        public NDR(ByteGetter data) {
            super(data, NUMBER);
        }

        protected int getInt(int index) {
            int hg = data.get(index + 3);
            int high = ((int) hg << 24);
            int hmed = ((int) data.get(index + 2) << 16);
            int lmed = ((int) data.get(index + 1) << 8);
            int low = (int) data.get(index);
            return high + hmed + lmed + low;
        }

        protected long getLong(int index) {
            return ((long) data.get(index + 7) << 56) + ((long) data.get(index + 6) << 48)
                    + ((long) data.get(index + 5) << 40) + ((long) data.get(index + 4) << 32)
                    + ((long) data.get(index + 3) << 24) + ((long) data.get(index + 2) << 16)
                    + ((long) data.get(index + 1) << 8) + ((long) data.get(index) << 0);
        }
    }
}