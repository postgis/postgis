package examples;

import org.postgis.LineString;
import org.postgis.LinearRing;
import org.postgis.MultiLineString;
import org.postgis.MultiPolygon;
import org.postgis.PGgeometry;
import org.postgis.Point;
import org.postgis.Polygon;

import java.sql.SQLException;

public class Test {

    public static void main(String[] args) throws SQLException {
        String mlng_str = "MULTILINESTRING ((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0))";
        String mplg_str = "MULTIPOLYGON (((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)),((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0)))";
        String plg_str = "POLYGON ((10 10 0,20 10 0,20 20 0,20 10 0,10 10 0),(5 5 0,5 6 0,6 6 0,6 5 0,5 5 0))";
        String lng_str = "LINESTRING  (10 10 20,20 20 20, 50 50 50, 34 34 34)";
        String ptg_str = "POINT(10 10 20)";
        String lr_str = "(10 10 20,34 34 34, 23 19 23 , 10 10 11)";

        System.out.println("LinearRing Test:");
        System.out.println(lr_str);
        LinearRing lr = new LinearRing(lr_str);
        System.out.println(lr.toString());

        System.out.println("Point Test:");
        System.out.println(ptg_str);
        Point ptg = new Point(ptg_str);
        System.out.println(ptg.toString());

        System.out.println("LineString Test:");
        System.out.println(lng_str);
        LineString lng = new LineString(lng_str);
        System.out.println(lng.toString());

        System.out.println("Polygon Test:");
        System.out.println(plg_str);
        Polygon plg = new Polygon(plg_str);
        System.out.println(plg.toString());

        System.out.println("MultiPolygon Test:");
        System.out.println(mplg_str);
        MultiPolygon mplg = new MultiPolygon(mplg_str);
        System.out.println(mplg.toString());

        System.out.println("MultiLineString Test:");
        System.out.println(mlng_str);
        MultiLineString mlng = new MultiLineString(mlng_str);
        System.out.println(mlng.toString());

        System.out.println("PG Test:");
        System.out.println(mlng_str);
        PGgeometry pgf = new PGgeometry(mlng_str);
        System.out.println(pgf.toString());

        System.out.println("finished");
    }
}

