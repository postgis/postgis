/*
 * Version.java
 * 
 * PostGIS extension for PostgreSQL JDBC driver - current version identification
 * 
 * (C) 2005 Markus Schaber, schabios@logi-track.com
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 2.1 of the License.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA or visit the web at
 * http://www.gnu.org.
 * 
 * $Id$
 */

package org.postgis;

/** Corresponds to the appropriate PostGIS that carried this source */
public class Version {

    /** The major version */
    public static final int MAJOR = 1;

    /** The minor version */
    public static final int MINOR = 0;

    /**
     * The micro version, usually a number including possibly textual suffixes
     * like RC3.
     */
    public static final String MICRO = "0RC4";

    /** Full version for human reading - code should use the constants above */
    public static final String FULL = "PostGIS JDBC V" + MAJOR + "." + MINOR + "." + MICRO;

    public static void main(String[] args) {
        System.out.println(FULL);
    }

    public static String getFullVersion() {
        return FULL;
    }
}
