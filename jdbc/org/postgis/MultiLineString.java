package org.postgis;

import org.postgresql.util.*;
import java.sql.*;
import java.util.*;


public class MultiLineString extends Geometry 
{

	LineString[] lines;
	
	public MultiLineString() 
	{
		type = MULTILINESTRING;
	}

	public MultiLineString(LineString[] lines) 
	{
		this();
		this.lines = lines;
		dimension = lines[0].dimension;
	}
	
	public MultiLineString(String value) throws SQLException
	{
		this();
		value = value.trim();
		if ( value.indexOf("MULTILINESTRING") == 0 ) 
		{
			PGtokenizer t = new PGtokenizer(PGtokenizer.removePara(value.substring(15).trim()),',');
			int nlines = t.getSize();
			lines = new LineString[nlines];
			for( int p = 0; p < nlines; p++)
			{
				lines[p] = new LineString(t.getToken(p));
			}
			dimension = lines[0].dimension;
		} else {
			throw new SQLException("postgis.multilinestringgeometry");
		}
	}
	
	public String toString() 
	{
		return "MULTILINESTRING " + getValue();
	}

	public String getValue() 
	{
		StringBuffer b = new StringBuffer("(");
		for( int p = 0; p < lines.length; p++ )
		{
			if( p > 0 ) b.append(",");
			b.append(lines[p].getValue());
		}
		b.append(")");
		return b.toString();
	}
	
	public int numLines() 
	{
		return lines.length;
	}
	
	public LineString getLine(int idx) 
	{
		if( idx >= 0 & idx < lines.length ) {
			return lines[idx];
		}
		else {
			return null;
		}
	}

}
