/*
 * DriverWrapper.java
 * 
 * PostGIS extension for PostgreSQL JDBC driver - Wrapper utility class
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

import org.postgresql.Driver;
import org.postgresql.PGConnection;

import java.sql.Connection;
import java.sql.SQLException;
import java.util.Properties;

/**
 * DriverWrapper
 * 
 * Wraps the PostGreSQL Driver to transparently add the PostGIS Object Classes.
 * This avoids the need of explicit addDataType() calls from the driver users
 * side.
 * 
 * This method currently works with J2EE DataSource implementations, and with
 * DriverManager framework.
 * 
 * Simply replace the "jdbc:postgresql:" with a "jdbc:postgresql_postGIS" in the
 * jdbc URL.
 * 
 * When using the drivermanager, you need to initialize DriverWrapper instead of
 * (or in addition to) org.postgresql.Driver. When using a J2EE DataSource
 * implementation, set the driver class property in the datasource config, the
 * following works for jboss:
 * 
 * <code>
 * &lt;driver-class&gt;org.postgis.DriverWrapper&lt;/driver-class&gt;
 * </code>
 * If you don't like or want to use the DriverWrapper, you have two
 * alternatives, see the README file.
 * 
 * Also note that the addDataType() methods known from earlier pgjdbc versions
 * are deprecated in pgjdbc 8.0, see the commented code variants in the
 * addGisTypes() method.
 * 
 * @author Markus Schaber <schabios@logi-track.com>
 *  
 */
public class DriverWrapper extends Driver {

    private static final String POSTGRES_PROTOCOL = "jdbc:postgresql:";
    private static final String POSTGIS_PROTOCOL = "jdbc:postgresql_postGIS:";
    public static final String REVISION = "$Revision$";
    public final TypesAdder typesAdder;

    /**
     * Default constructor.
     * 
     * This also loads the appropriate TypesAdder for our SQL Driver instance.
     * 
     * @throws SQLException
     */
    public DriverWrapper() throws SQLException {
        super();
        typesAdder = getTypesAdder(this);
        // The debug method is @since 7.2
        if (super.getMajorVersion() > 8 || super.getMinorVersion() > 1) {
            Driver.debug("DriverWrapper loaded TypesAdder: " + typesAdder.getClass().getName());
        }
    }

    protected static TypesAdder getTypesAdder(Driver d) throws SQLException {
        if (d.getMajorVersion() == 7) {
            if (d.getMinorVersion() >= 3) {
                return loadTypesAdder(74);
            } else {
                return loadTypesAdder(72);
            }
        } else {
            return loadTypesAdder(80);
        }
    }

    private static TypesAdder loadTypesAdder(int i) throws SQLException {
        try {
            Class klass = Class.forName("org.postgis.DriverWrapper$TypesAdder" + i);
            return (TypesAdder) klass.newInstance();
        } catch (Exception e) {
            throw new SQLException("Cannot create TypesAdder instance! " + e.getMessage());
        }
    }

    static {
        try {
            // Try to register ourself to the DriverManager
            java.sql.DriverManager.registerDriver(new DriverWrapper());
        } catch (SQLException e) {
            Driver.info("Error registering PostGIS Wrapper Driver", e);
        }
    }

    /**
     * Creates a postgresql connection, and then adds the PostGIS data types to
     * it calling addpgtypes()
     * 
     * @param url the URL of the database to connect to
     * @param info a list of arbitrary tag/value pairs as connection arguments
     * @return a connection to the URL or null if it isnt us
     * @exception SQLException if a database access error occurs
     * 
     * @see java.sql.Driver#connect
     * @see org.postgresql.Driver
     */
    public java.sql.Connection connect(String url, Properties info) throws SQLException {
        url = mangleURL(url);
        Connection result = super.connect(url, info);
        typesAdder.addGT(result);
        return result;
    }

    /**
     * Check whether the driver thinks he can handle the given URL.
     * 
     * @see java.sql.Driver#acceptsURL
     * @param url the URL of the driver
     * @return true if this driver accepts the given URL
     * @exception SQLException Passed through from the underlying PostgreSQL
     *                driver, should not happen.
     */
    public boolean acceptsURL(String url) throws SQLException {
        try {
            url = mangleURL(url);
        } catch (SQLException e) {
            return false;
        }
        return super.acceptsURL(url);
    }

    /**
     * Returns our own CVS version plus postgres Version
     */
    public static String getVersion() {
        return "PostGisWrapper " + REVISION + ", wrapping " + Driver.getVersion();
    }

    /*
     * Here follows the addGISTypes() stuff. This is a little tricky because the
     * pgjdbc people had several, partially incompatible API changes during 7.2
     * and 8.0. We still want to support all those releases, however.
     *  
     */
    /**
     * adds the JTS/PostGIS Data types to a PG 7.3+ Connection. If you use
     * PostgreSQL jdbc drivers V8.0 or newer, those methods are deprecated due
     * to some class loader problems (but still work for now), and you may want
     * to use the method below instead.
     *  
     */
    public static void addGISTypes(PGConnection pgconn) {
        pgconn.addDataType("geometry", "org.postgis.PGgeometry");
        pgconn.addDataType("box3d", "org.postgis.PGbox3d");
        pgconn.addDataType("box2d", "org.postgis.PGbox2d");
    }

    /**
     * adds the JTS/PostGIS Data types to a PG 8.0+ Connection.
     */
    public static void addGISTypes80(PGConnection pgconn) throws SQLException {
        pgconn.addDataType("geometry", org.postgis.PGgeometry.class);
        pgconn.addDataType("box3d", org.postgis.PGbox3d.class);
        pgconn.addDataType("box2d", org.postgis.PGbox2d.class);
    }

    /**
     * adds the JTS/PostGIS Data types to a PG 7.2 Connection.
     */
    public static void addGISTypes72(org.postgresql.Connection pgconn) {
        pgconn.addDataType("geometry", "org.postgis.PGgeometry");
        pgconn.addDataType("box3d", "org.postgis.PGbox3d");
        pgconn.addDataType("box2d", "org.postgis.PGbox2d");
    }

    /**
     * Mangles the PostGIS URL to return the original PostGreSQL URL
     */
    public static String mangleURL(String url) throws SQLException {
        if (url.startsWith(POSTGIS_PROTOCOL)) {
            return POSTGRES_PROTOCOL + url.substring(POSTGIS_PROTOCOL.length());
        } else {
            throw new SQLException("Unknown protocol or subprotocol in url " + url);
        }
    }

    /** Base class for the three typewrapper implementations */
    protected static abstract class TypesAdder {
        public abstract void addGT(java.sql.Connection conn) throws SQLException;
    }

    /** addGISTypes for V7.3 and V7.4 pgjdbc */
    protected static final class TypesAdder74 extends TypesAdder {
        public void addGT(java.sql.Connection conn) {
            addGISTypes((PGConnection) conn);
        }
    }

    /** addGISTypes for V7.2 pgjdbc */
    protected static class TypesAdder72 extends TypesAdder {
        public void addGT(java.sql.Connection conn) {
            addGISTypes72((org.postgresql.Connection) conn);

        }
    }

    /** addGISTypes for V8.0 (and hopefully newer) pgjdbc */
    protected static class TypesAdder80 extends TypesAdder {
        public void addGT(java.sql.Connection conn) throws SQLException {
            addGISTypes80((PGConnection) conn);
        }
    }
}