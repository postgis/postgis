package examples;

import java.sql.*;
import java.util.*;
import java.lang.*;
import org.postgis.*;

public class TestServer 
{

	public static void main(String[] args) 
	{
		Connection conn;
		
		String dbname = "tb";
		String dbuser = "dblasby";
		String dbpass = "";
		String dbhost = "ox";
		String dbport = "5555";
		
		String dbtable = "jdbc_test";
		
		String dropSQL = "drop table " + dbtable;
		String createSQL = "create table " + dbtable + " (geom geometry, id int4)";
		String insertPointSQL = "insert into " + dbtable + " values ('POINT (10 10 10)',1)";
		String insertPolygonSQL = "insert into " + dbtable + " values ('POLYGON ((0 0 0,0 10 0,10 10 0,10 0 0,0 0 0))',2)";
		
		try {
		
			System.out.println("Creating JDBC connection...");
			Class.forName("org.postgresql.Driver");
			String url = "jdbc:postgresql://" + dbhost + ":" + dbport + "/" + dbname;
			conn = DriverManager.getConnection(url, dbuser, dbpass);
			System.out.println("Adding geometric type entries...");
			((org.postgresql.Connection)conn).addDataType("geometry","org.postgis.PGgeometry");
			((org.postgresql.Connection)conn).addDataType("box3d","org.postgis.PGbox3d");
			Statement s = conn.createStatement();
			System.out.println("Creating table with geometric types...");
			s.execute(dropSQL);
			s.execute(createSQL);
			System.out.println("Inserting point...");
			s.execute(insertPointSQL);
			System.out.println("Inserting polygon...");
			s.execute(insertPolygonSQL);
			System.out.println("Done.");
			s = conn.createStatement();
			System.out.println("Querying table...");
			ResultSet r = s.executeQuery("select * from " + dbtable);
			while( r.next() ) 
			{
				Object obj =  r.getObject(1);
				int id = r.getInt(2);
				System.out.println("Row " + id + ":");
				System.out.println(obj.toString());
			}
			s.close();
			conn.close();
		}
		catch( Exception e ) {
			e.printStackTrace();
		}

			

	}


}
