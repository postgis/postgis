package org.postgis;

import org.postgresql.util.PGobject;
import org.postgresql.util.PGtokenizer;
import java.sql.*;

/*
 * Updates Oct 2002
 *	- setValue() method now cheaks if the geometry has a SRID. If present,
 *	  it is removed and only the wkt is used to create the new geometry
 */

public class PGgeometry extends PGobject 
{

	Geometry geom;

	public PGgeometry() { }

	public PGgeometry(Geometry geom) {
      		this.geom = geom;
	}

	public PGgeometry(String value) throws SQLException
	{
		setValue(value);
	}

	public void setValue(String value) throws SQLException
	{
		value = value.trim();
		if( value.startsWith("SRID")) {
			//break up geometry into srid and wkt
			PGtokenizer t = new PGtokenizer(value,';');
			value = t.getToken(1);
		}

		if( value.startsWith("MULTIPOLYGON")) {
			geom = new MultiPolygon(value);
		} else if( value.startsWith("MULTILINESTRING")) {
			geom = new MultiLineString(value);
		} else if( value.startsWith("MULTIPOINT")) {
			geom = new MultiPoint(value);
		} else if( value.startsWith("POLYGON")) {
			geom = new Polygon(value);
		} else if( value.startsWith("LINESTRING")) {
			geom = new LineString(value);
		} else if( value.startsWith("POINT")) {
			geom = new Point(value);
		} else {
			throw new SQLException("Unknown type: " + value);
		}
	}

	public Geometry getGeometry() {
		return geom;
	}

	public int getGeoType() {
		return geom.type;
	}

	public String toString() {
		return geom.toString();
	}

	public String getValue() {
		return geom.toString();
	}

	public Object clone() 
	{
		PGgeometry obj = new PGgeometry(geom);
		obj.setType(type);
		return obj;
	}
	
}
