package org.postgis;

import org.postgresql.util.*;
import java.sql.*;
import java.util.*;


public class Polygon extends Geometry 
{

	LinearRing[] rings;
	
	public Polygon() 
	{
		type = POLYGON;
	}

	public Polygon(LinearRing[] rings) 
	{
		this();
		this.rings = rings;
		dimension = rings[0].dimension;
	}
	
	public Polygon(String value) throws SQLException
	{
		this();
		value = value.trim();
		if ( value.indexOf("POLYGON") == 0 ) 
		{
			value = value.substring(7).trim();
		}
		PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value),',');
		int nrings = t.getSize();
		rings = new LinearRing[nrings];
		for( int r = 0; r < nrings; r++)
		{
			rings[r] = new LinearRing(t.getToken(r));
		}
		dimension = rings[0].dimension;
	}
	
	public String toString() 
	{
		return "POLYGON " + getValue();
	}

	public String getValue() 
	{
		StringBuffer b = new StringBuffer("(");
		for( int r = 0; r < rings.length; r++ )
		{
			if( r > 0 ) b.append(",");
			b.append(rings[r].toString());
		}
		b.append(")");
		return b.toString();
	}
	
	public int numRings()
	{
		return rings.length;
	}
	
	public LinearRing getRing(int idx)
	{
		if( idx >= 0 & idx < rings.length ) {
			return rings[idx];
		}
		else {
			return null;
		}
	}
}
