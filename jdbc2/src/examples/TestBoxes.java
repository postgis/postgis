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

import org.postgis.PGbox2d;
import org.postgis.PGbox3d;
import org.postgresql.util.PGobject;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class TestBoxes {

    /** Our test candidates: */
    public static final String[] BOXEN3D = new String[]{
        "BOX3D(1 2 3,4 5 6)", //3d variant
        "BOX3D(1 2,4 5)"// 2d variant
    };
    public static final String[] BOXEN2D = new String[]{"BOX(1 2,3 4)"};

    /** The srid we use for the srid tests */
    public static final int SRID = 4326;

    /** The string prefix we get for the srid tests */
    public static final String SRIDPREFIX = "SRID=" + SRID + ";";

    /** How much tests did fail? */
    public static int failcount = 0;

    /** The actual test method */
    public static void test(String orig, PGobject candidate, Connection[] conns)
            throws SQLException {

        System.out.println("Original:  " + orig);
        String redone = candidate.toString();
        System.out.println("Parsed:    " + redone);

        if (!orig.equals(redone)) {
            System.out.println("--- Recreated Text Rep not equal!");
            failcount++;
        }

        // Let's simulate the way pgjdbc uses to create PGobjects
        PGobject recreated;
        try {
            recreated = (PGobject) candidate.getClass().newInstance();
        } catch (Exception e) {
            System.out.println("--- pgjdbc instantiation failed!");
            System.out.println("--- " + e.getMessage());
            failcount++;
            return;
        }
        recreated.setValue(redone);

        String reparsed = recreated.toString();
        System.out.println("Re-Parsed: " + reparsed);
        if (!recreated.equals(candidate)) {
            System.out.println("--- Recreated boxen are not equal!");
            failcount++;
        } else if (!reparsed.equals(orig)) {
            System.out.println("--- 2nd generation text reps are not equal!");
            failcount++;
        } else {
            System.out.println("Equals:    yes");
        }

        for (int i = 0; i < conns.length; i++) {
            System.out.println("Testing on connection " + i + ": " + conns[i].getCatalog());
            Statement statement = conns[i].createStatement();

            try {
                PGobject sqlGeom = viaSQL(candidate, statement);
                System.out.println("SQLin    : " + sqlGeom.toString());
                if (!candidate.equals(sqlGeom)) {
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
                PGobject sqlreGeom = viaSQL(recreated, statement);
                System.out.println("SQLout  :  " + sqlreGeom.toString());
                if (!candidate.equals(sqlreGeom)) {
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
    private static PGobject viaSQL(PGobject obj, Statement stat) throws SQLException {
        ResultSet rs = stat.executeQuery("SELECT '" + obj.toString() + "'::" + obj.getType());
        rs.next();
        return (PGobject) rs.getObject(1);
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

        for (int i = 0; i < BOXEN3D.length; i++) {
            try {
                PGbox3d candidate = new PGbox3d(BOXEN3D[i]);
                test(BOXEN3D[i], candidate, conns);
            } catch (SQLException e) {
                System.out.println("--- Instantiation of " + BOXEN3D[i] + "failed:");
                System.out.println("--- " + e.getMessage());
                failcount++;
            }
        }

        for (int i = 0; i < BOXEN2D.length; i++) {
            try {
                PGbox2d candidate = new PGbox2d(BOXEN2D[i]);
                test(BOXEN2D[i], candidate, conns);
            } catch (SQLException e) {
                System.out.println("--- Instantiation of " + BOXEN2D[i] + "failed:");
                System.out.println("--- " + e.getMessage());
                failcount++;
            }
        }

        System.out.print("cleaning up...");
        for (int i = 0; i < conns.length; i++) {
            conns[i].close();
        }

        //System.out.println("Finished.");
        System.out.println("Finished, " + failcount + " tests failed!");
    }
}