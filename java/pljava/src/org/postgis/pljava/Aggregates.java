package org.postgis.pljava;

import java.sql.SQLException;

import org.postgresql.pljava.Session;

public class Aggregates {
    public static void test() throws SQLException {
        Session a = org.postgresql.pljava.SessionManager.current();
        
        a.hashCode();
    }
}
