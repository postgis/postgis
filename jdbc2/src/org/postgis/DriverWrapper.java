/*
 * PostGisWrapper.java
 * 
 * $Id$
 * 
 * (C) 2004 Markus Schaber, logi-track ag, Zürich, Switzerland
 * 
 * This file currently is beta test code and licensed under the GNU LGPL.
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
 * &lt;driver-class&gt;com.logitrack.gis.util.DriverWrapper&lt;/driver-class&gt;
 * </code>
 * If you don't like or want to use the DriverWrapper, you have two
 * alternatives:
 * 
 * A) If you use a pgjdbc driver newer than 8 Nov 2004, the
 * postgresql.properties file in the org.postgresql directory delivered with
 * this source should auto-register the PostGIS extension.
 * 
 * B) Add the datatypes manually to your connection:
 * 
 * <code>
 * ((org.postgresql.PGConnection) conn).addDataType("geometry", "org.postgis.PGgeometry");
 * ((org.postgresql.PGConnection) conn).addDataType("box3d", "org.postgis.PGbox3d");
 * </code>
 * 
 * (You may need to dig through some wrappers when running in an appserver. E.
 * G. for JBoss, you get a org.jboss.resource.adapter.jdbc.WrappedConnection and
 * have to call getUnderlyingConnection() on it.)
 * 
 * Also note that the addDataType() methods known from earlier pgjdbc versions
 * are deprecated in pgjdbc 8.0, see the commented code variants in the
 * addGisTypes() method for an alternative.
 * 
 * @author Markus Schaber <schabios@logi-track.com>
 *  
 */
public class DriverWrapper extends Driver {

    private static final String POSTGRES_PROTOCOL = "jdbc:postgresql:";
    private static final String POSTGIS_PROTOCOL = "jdbc:postgresql_postGIS:";
    public static final String REVISION = "$Revision$";

    public DriverWrapper() {
        super();
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
        addGISTypes((PGConnection) result);
        return result;
    }

    /**
     * adds the JTS/PostGIS Data types to a PG Connection.
     */
    public static void addGISTypes(PGConnection pgconn) {
        // This is correct for PostgreSQL jdbc drivers up to V7.4
        pgconn.addDataType("geometry", "org.postgis.PGgeometry");
        pgconn.addDataType("box3d", "org.postgis.PGbox3d");
        pgconn.addDataType("box2d", "org.postgis.PGbox2d");

        // If you use PostgreSQL jdbc drivers V8.0 or newer, the above
        // methods are deprecated (but still work for now), and you
        // may want to use the two lines below instead.

        //pgconn.addDataType("geometry", org.postgis.PGgeometry.class);
        //pgconn.addDataType("box3d", org.postgis.PGbox3d.class);
        //pgconn.addDataType("box2d", org.postgis.PGbox2d.class);
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
     * Gets the underlying drivers major version number
     * 
     * @return the drivers major version number
     */

    public int getMajorVersion() {
        return super.getMajorVersion();
    }

    /**
     * Get the underlying drivers minor version number
     * 
     * @return the drivers minor version number
     */
    public int getMinorVersion() {
        return super.getMinorVersion();
    }

    /**
     * Returns our own CVS version plus postgres Version
     */
    public static String getVersion() {
        return "PostGisWrapper " + REVISION + ", wrapping " + Driver.getVersion();
    }
}