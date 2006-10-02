/*
 * PLJGeometry
 * 
 * PostGIS datatype definition for PLJava
 * 
 * (C) 2006 Markus Schaber, markus.schaber@logix-tt.com
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 2.1 of the License.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA or visit the web at
 * http://www.gnu.org.
 * 
 * $Id$
 */
package org.postgis.pljava;

import java.sql.SQLData;
import java.sql.SQLException;
import java.sql.SQLInput;
import java.sql.SQLOutput;

import com.vividsolutions.jts.geom.CoordinateSequenceFactory;
import com.vividsolutions.jts.geom.Geometry;
import com.vividsolutions.jts.geom.GeometryFactory;
import com.vividsolutions.jts.geom.PrecisionModel;
import com.vividsolutions.jts.geom.impl.PackedCoordinateSequenceFactory;

public class PLJGeometry implements SQLData {
    final static PrecisionModel prec = new PrecisionModel();

    final static CoordinateSequenceFactory csfac = PackedCoordinateSequenceFactory.DOUBLE_FACTORY;

    final static GeometryFactory geofac = new GeometryFactory(prec, 0, csfac);

    final static PLJtsParser parser = new PLJtsParser();
    final static PLJtsWriter writer = new PLJtsWriter();
    
    public static final String m_typeName="public.geometry";
    
    public Geometry geom;
    
    public String getSQLTypeName() {
        return m_typeName;
    }

    public void readSQL(SQLInput stream, String typeName) throws SQLException {
        checkType(typeName);
        
        // skip length marker
        stream.readInt();
        
        // read the Geometry        
        this.geom = parser.parseGeometry(stream);
    }
    
    /** Check whether our given type is actually the one we can handle */
    private static void checkType(String typeName) throws SQLException {
        if (!m_typeName.equalsIgnoreCase(typeName)) {
                throw new SQLException("parser for "+m_typeName+" cannot parse type "+typeName+"!");
        }
    }


    public void writeSQL(SQLOutput stream) throws SQLException {
        // write size marker
        stream.writeInt(writer.estimateBytes(geom));
        
        // write geometry
        writer.writeGeometry(geom, stream);
    }
}
