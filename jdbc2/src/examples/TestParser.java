/*
 * Test client and server side Parsing of canonical text representations, as
 * well as the PostGisWrapper jdbc extension.
 * 
 * (C) 2005 Markus Schaber, logi-track ag, Zürich, Switzerland
 * 
 * This file is licensed under the GNU GPL. *** put full notice here ***
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

    public static final String[] testsridset = new String[testset.length];
    public static final int SRID = 4326;

    static {
        String prefix = "SRID=" + SRID + ";";
        for (int i = 0; i < testset.length; i++) {
            testsridset[i] = prefix + testset[i];
        }
    }

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
        } else if (!reparsed.equals(parsed)) {
            System.out.println("--- Text Reps are not equal!");
        } else {
            System.out.println("Equals:    yes");
        }
        for (int i = 0; i < conns.length; i++) {
            System.out.println("Testing on connection " + i + ": "+ conns[i].getCatalog());
            Statement statement = conns[i].createStatement();

            try {
                Geometry sqlGeom = viaSQL(WKT, statement);
                System.out.println("SQLin    : " + sqlGeom.toString());
                if (!geom.equals(sqlGeom)) {
                    System.out.println("--- Geometries after SQL are not equal!");
                } else {
                    System.out.println("Eq SQL in: yes");
                }
            } catch (SQLException e) {
                System.out.println("--- Server side error: " + e.toString());
            }

            try {
                Geometry sqlreGeom = viaSQL(parsed, statement);
                System.out.println("SQLout  :  " + sqlreGeom.toString());
                if (!geom.equals(sqlreGeom)) {
                    System.out.println("--- reparsed Geometries after SQL are not equal!");
                } else {
                    System.out.println("Eq SQLout: yes");
                }
            } catch (SQLException e) {
                System.out.println("--- Server side error: " + e.toString());
            }

            statement.close();
        }
        System.out.println("***");
    }

    private static Geometry viaSQL(String rep, Statement stat) throws SQLException {
        ResultSet rs = stat.executeQuery("SELECT geometry_in('" + rep + "')");
        rs.next();
        return ((PGgeometry) rs.getObject(1)).getGeometry();
    }

    /**
     * The list of databases to test (can be multiple to test against different
     * postGIS releases.
     * 
     * Currently, all of them need the same username and password
     */

    public static final String[] databases = new String[]{
        "jdbc:postgresql_postGIS://localhost:5432/hwgeom",
        "jdbc:postgresql_postGIS://localhost:5432/lwgeom"};

    public final static String dbuser = "postgistest";
    public final static String dbpass = "testpostgis";

    /**
     * Connect to the databases
     * 
     * We use DriverWrapper here. For alternatives, see the DriverWrapper
     * Javadoc
     * 
     * @throws ClassNotFoundException
     * 
     * @see org.postgis.DriverWrapper
     *  
     */
    public static Connection connect(String url) throws SQLException, ClassNotFoundException {
        Connection conn;
        Class.forName("org.postgis.DriverWrapper");
        conn = DriverManager.getConnection(url, dbuser, dbpass);
        return conn;
    }

    public static void main(String[] args) throws SQLException, ClassNotFoundException {
        String[] URLs = databases;

        // allow to run offline tests...
        if (args.length == 1 && args[0].equalsIgnoreCase("offline")) {
            System.out.println("Performing only offline tests");
            URLs = new String[0];
        } else if (args.length!=0) {
            System.err.println("usage: java [offline]");
            System.exit(1);
        }
        Connection[] conns;
        conns = new Connection[URLs.length];
        for (int i = 0; i < URLs.length; i++) {
            System.out.println("Creating JDBC connection to " + URLs[i]);
            conns[i] = connect(URLs[i]);
        }

        System.out.println("Performing tests...");
        System.out.println("***");

        for (int i = 0; i < testset.length; i++) {
            test(testset[i], conns);
            test(testsridset[i], conns);
        }

        System.out.print("cleaning up...");
        for (int i = 0; i < conns.length; i++) {
            conns[i].close();

        }
        System.out.println("Finished.");
    }
}