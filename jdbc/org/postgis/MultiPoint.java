package org.postgis;

import org.postgresql.util.*;
import java.sql.*;
import java.util.*;


public class MultiPoint extends Geometry 
{

	Point[] points;
	
	public MultiPoint() 
	{
		type = MULTIPOINT;
	}

	public MultiPoint(Point[] points) 
	{
		this();
		this.points = points;
		dimension = points[0].dimension;
	}
	
	public MultiPoint(String value) throws SQLException
	{
		this();
		value = value.trim();
		if ( value.indexOf("MULTIPOINT") == 0 ) 
		{
			PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.substring(10).trim()),',');
			int npoints = t.getSize();
			points = new Point[npoints];
			for( int p = 0; p < npoints; p++)
			{
				points[p] = new Point(t.getToken(p));
			}
			dimension = points[0].dimension;
		} else {
			throw new SQLException("postgis.multipointgeometry");
		}
	}
	
	public String toString() 
	{
		return "MULTIPOINT " + getValue();
	}

	public String getValue() 
	{
		StringBuffer b = new StringBuffer("(");
		for( int p = 0; p < points.length; p++ )
		{
			if( p > 0 ) b.append(",");
			b.append(points[p].getValue());
		}
		b.append(")");
		return b.toString();
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
