package org.postgis;

import java.io.*;
import java.sql.*;
import org.postgresql.util.*;

/** 
 * This represents the LinearRing GIS datatype. This type is used to 
 * construct the polygon types, but is not
 * stored or retrieved directly from the database.
 */
public class LinearRing extends Geometry
{
	
	/**
	 * The points in the ring.
	 */
	public Point[] points;

	public LinearRing(Point[] points) 
	{
		this.points = points;
		dimension = points[0].dimension;
	}

	/**
	 * This is called to construct a LinearRing from the 
	 * PostGIS string representation of a ring.
	 *
	 * @param value Definition of this ring in the PostGIS 
	 * string format.
	 */
	public LinearRing(String value) throws SQLException
	{
		PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.trim()),',');
		int npoints = t.getSize();
		points = new Point[npoints];
		for( int p = 0; p < npoints; p++ ) {
			points[p] = new Point(t.getToken(p));
		}
		dimension = points[0].dimension;
	}

	/**
	 * @return the LinearRing in the syntax expected by PostGIS
	 */
	public String toString() 
	{
		StringBuffer b = new StringBuffer("(");
		for ( int p = 0; p < points.length; p++ ) 
		{
			if( p > 0 ) b.append(",");
			b.append(points[p].getValue());
		}
		b.append(")");
		return b.toString();
	}
	
	/**
	 * @return the LinearRing in the string syntax expected by PostGIS
	 */
	public String getValue() 
	{
		return toString();
	}
	
	public int numPoints() 
	{
		return points.length;
	}
	
	public Point getPoint(int idx) 
	{
		if ( idx >= 0 & idx < points.length ) {
			return points[idx];
		}
		else {
			return null;
		}
	}
	
}
