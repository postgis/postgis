/*
 * Version.java
 * 
 * PostGIS extension for PostgreSQL JDBC driver - current version identification
 * 
 * (C) 2005 Markus Schaber, markus@schabi.de
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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;

/** Corresponds to the appropriate PostGIS that carried this source */
public class Version {
    /** We read our version information from this ressource... */
    private static final String RESSOURCENAME = "org/postgis/version.properties";
    
    /** The major version */
    public static final int MAJOR;

    /** The minor version */
    public static final int MINOR;

    /**
     * The micro version, usually a number including possibly textual suffixes
     * like RC3.
     */
    public static final String MICRO;

    static {
        int major = -1;
        int minor = -1;
        String micro = "INVALID";
        try {
            ClassLoader loader = Version.class.getClassLoader();
            URL ver = loader.getResource(RESSOURCENAME);
            if (ver == null) {
                throw new ExceptionInInitializerError(
                        "Error initializing PostGIS JDBC version! Cause: Ressource "+RESSOURCENAME+" not found!");
            }
            BufferedReader rd = new BufferedReader(new InputStreamReader(ver.openStream()));

            String line;
            while ((line = rd.readLine()) != null) {
                line = line.trim();
                if (line.startsWith("JDBC_MAJOR_VERSION")) {
                    major = Integer.parseInt(line.substring(19));
                } else if (line.startsWith("JDBC_MINOR_VERSION")) {
                    minor = Integer.parseInt(line.substring(19));
                } else if (line.startsWith("JDBC_MICRO_VERSION")) {
                    micro = line.substring(19);
                } else {
                    // ignore line
                }
            }
            if (major == -1 || minor == -1 || micro.equals("INVALID")) {
                throw new ExceptionInInitializerError("Error initializing PostGIS JDBC version! Cause: Ressource "+RESSOURCENAME+" incomplete!");
            }
        } catch (IOException e) {
            throw new ExceptionInInitializerError("Error initializing PostGIS JDBC version! Cause: " + e.getMessage());
        } finally {
            MAJOR = major;
            MINOR = minor;
            MICRO = micro;
        }
    }

    /** Full version for human reading - code should use the constants above */
    public static final String FULL = "PostGIS JDBC V" + MAJOR + "." + MINOR + "." + MICRO;

    public static void main(String[] args) {
        System.out.println(FULL);
    }

    public static String getFullVersion() {
        return FULL;
    }
}
