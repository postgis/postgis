/*
 * Test.java
 * 
 * PostGIS extension for PostgreSQL JDBC driver - example and test classes
 * 
 * (C) 2004 Paul Ramsey, pramsey@refractions.net
 * 
 * (C) 2005 Markus Schaber, schabios@logi-track.com
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA or visit the web at
 * http://www.gnu.org.
 * 
 * $Id$
 */

package examples;

import org.postgis.Geometry;
import org.postgis.PGgeometry;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class TestParser {

    /**
     * Our set of geometries to test.
     */
    public static final String[] testset = new String[]{
        "POINT(10 10 20)",
        "MULTIPOINT(10 10 10, 20 20 20)",
        "LINESTRING(10 10 20,20 20 20, 50 50 50, 34 34 34)",
        "POLYGON((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0))",
        "MULTIPOLYGON(((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)),((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)))",
        "MULTILINESTRING((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0))",
        "GEOMETRYCOLLECTION(POINT(10 10 20),POINT(20 20 20))",
        "GEOMETRYCOLLECTION(LINESTRING(10 10 20,20 20 20, 50 50 50, 34 34 34),LINESTRING(10 10 20,20 20 20, 50 50 50, 34 34 34))",
        "GEOMETRYCOLLECTION(POLYGON((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)),POLYGON((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)))",
        "GEOMETRYCOLLECTION(MULTIPOINT(10 10 10, 20 20 20),MULTIPOINT(10 10 10, 20 20 20))",
        "GEOMETRYCOLLECTION(MULTILINESTRING((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)))",
        "GEOMETRYCOLLECTION(MULTIPOLYGON(((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)),((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0))),MULTIPOLYGON(((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)),((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0))))",
        "GEOMETRYCOLLECTION(POINT(10 10 20),LINESTRING(10 10 20,20 20 20, 50 50 50, 34 34 34),POLYGON((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)))",
        "GEOMETRYCOLLECTION(POINT(10 10 20),MULTIPOINT(10 10 10, 20 20 20),LINESTRING(10 10 20,20 20 20, 50 50 50, 34 34 34),POLYGON((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)),MULTIPOLYGON(((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)),((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0))),MULTILINESTRING((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)))",
        "GEOMETRYCOLLECTION(EMPTY)", // Old (bad) PostGIS 0.8 Representation
        "GEOMETRYCOLLECTION EMPTY", // new (correct) representation
        "POINT EMPTY", // new (correct) representation
        "LINESTRING EMPTY", // new (correct) representation
        "POLYGON EMPTY", // new (correct) representation
        "MULTIPOINT EMPTY", // new (correct) representation
        "MULTILINESTRING EMPTY", // new (correct) representation
        "MULTIPOLYGON EMPTY", // new (correct) representation
    };

    /** The srid we use for the srid tests */
    public static final int SRID = 4326;

    /** The string prefix we get for the srid tests */
    public static final String SRIDPREFIX = "SRID=" + SRID + ";";

    /** How much tests did fail? */
    public static int failcount = 0;

    /** The actual test method */
    public static void test(String WKT, Connection[] conns) throws SQLException {
        System.out.println("Original:  " + WKT);
        Geometry geom = PGgeometry.geomFromString(WKT);
        String parsed = geom.toString();
        System.out.println("Parsed:    " + parsed);
        Geometry regeom = PGgeometry.geomFromString(parsed);
        String reparsed = regeom.toString();
        System.out.println("Re-Parsed: " + reparsed);
        if (!geom.equals(regeom)) {
            System.out.println("--- Geometries are not equal!");
            failcount++;
        } else if (!reparsed.equals(parsed)) {
            System.out.println("--- Text Reps are not equal!");
            failcount++;
        } else {
            System.out.println("Equals:    yes");
        }
        for (int i = 0; i < conns.length; i++) {
            System.out.println("Testing on connection " + i + ": " + conns[i].getCatalog());
            Statement statement = conns[i].createStatement();

            try {
                Geometry sqlGeom = viaSQL(WKT, statement);
                System.out.println("SQLin    : " + sqlGeom.toString());
                if (!geom.equals(sqlGeom)) {
                    System.out.println("--- Geometries after SQL are not equal!");
                    failcount++;
                } else {
                    System.out.println("Eq SQL in: yes");
                }
            } catch (SQLException e) {
                System.out.println("--- Server side error: " + e.toString());
                failcount++;
            }

            try {
                Geometry sqlreGeom = viaSQL(parsed, statement);
                System.out.println("SQLout  :  " + sqlreGeom.toString());
                if (!geom.equals(sqlreGeom)) {
                    System.out.println("--- reparsed Geometries after SQL are not equal!");
                    failcount++;
                } else {
                    System.out.println("Eq SQLout: yes");
                }
            } catch (SQLException e) {
                System.out.println("--- Server side error: " + e.toString());
                failcount++;
            }

            statement.close();
        }
        System.out.println("***");
    }

    /** Pass a geometry representation through the SQL server */
    private static Geometry viaSQL(String rep, Statement stat) throws SQLException {
        ResultSet rs = stat.executeQuery("SELECT geometry_in('" + rep + "')");
        rs.next();
        return ((PGgeometry) rs.getObject(1)).getGeometry();
    }

    /**
     * Connect to the databases
     * 
     * We use DriverWrapper here. For alternatives, see the DriverWrapper
     * Javadoc
     * 
     * @param dbuser
     * 
     * @throws ClassNotFoundException
     * 
     * @see org.postgis.DriverWrapper
     *  
     */
    public static Connection connect(String url, String dbuser, String dbpass) throws SQLException,
            ClassNotFoundException {
        Connection conn;
        Class.forName("org.postgis.DriverWrapper");
        conn = DriverManager.getConnection(url, dbuser, dbpass);
        return conn;
    }

    /** Our apps entry point */
    public static void main(String[] args) throws SQLException, ClassNotFoundException {
        String[] dburls;
        String dbuser = null;
        String dbpass = null;

        if (args.length == 1 && args[0].equalsIgnoreCase("offline")) {
            System.out.println("Performing only offline tests");
            dburls = new String[0];
        } else if (args.length == 3) {
            System.out.println("Performing offline and online tests");
            dburls = args[0].split(";");
            dbuser = args[1];
            dbpass = args[2];
        } else {
            System.err.println("Usage: java examples/TestParser dburls user pass [tablename]");
            System.err.println("   or: java examples/TestParser offline");
            System.err.println();
            System.err.println("dburls has one or more jdbc urls separated by ; in the following format");
            System.err.println("jdbc:postgresql://HOST:PORT/DATABASENAME");
            System.err.println("tablename is 'jdbc_test' by default.");
            System.exit(1);
            // Signal the compiler that code flow ends here.
            throw new AssertionError();
        }

        Connection[] conns;
        conns = new Connection[dburls.length];
        for (int i = 0; i < dburls.length; i++) {
            System.out.println("Creating JDBC connection to " + dburls[i]);
            conns[i] = connect(dburls[i], dbuser, dbpass);
        }

        System.out.println("Performing tests...");
        System.out.println("***");

        for (int i = 0; i < testset.length; i++) {
            test(testset[i], conns);
            test(SRIDPREFIX + testset[i], conns);
        }

        System.out.print("cleaning up...");
        for (int i = 0; i < conns.length; i++) {
            conns[i].close();
        }

        //System.out.println("Finished.");
        System.out.println("Finished, " + failcount + " tests failed!");
    }
}