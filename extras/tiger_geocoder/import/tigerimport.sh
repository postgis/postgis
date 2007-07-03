#!/bin/bash
#
# tigerimport.sh - (c) 2005 Schuyler Erle <schuyler@geocoder.us>
#   Last updated Fri Mar  4 08:13:28 PST 2005
#
# tigerimport.sh imports the CompleteChain, Landmarks, AreaLandMarks, and
# Polygon layers from a TIGER/Line .ZIP file into a PostGIS database. You
# must have ogr2ogr from GDAL (http://gdal.org), as well as the GDAL
# Python extensions.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You can download a copy of the GPL from http://www.gnu.org/copyleft/gpl.html,
# or write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA  02111-1307, USA.

MODE=$1
DATABASE=$2
ZIP=$3
APPEND=$4

TIGERPOLY=tigerpoly2.py

case $MODE in
    append) PRE="-append"; POST="" ;;
    update) PRE="-update"; POST="" ;;
    create) PRE=""; POST="-lco OVERWRITE=YES" ;;
    *) echo "Usage: $0 [append|update|create] <TIGER/Line .zip> <database>";
       exit -1;;
esac

FILE=$(basename $ZIP)
FILE=${FILE#TGR}
FILE=${FILE%.ZIP}
TIGER=TGR$FILE.RT1

SRID=4269
#cat > nad83_srs.txt <<End
#GEOGCS["NAD83",DATUM["North_American_Datum_1983",SPHEROID["GRS 1980",6378137,298.257222101,AUTHORITY["EPSG","7019"]],AUTHORITY["EPSG","6269"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4269"]]
#End

# GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137,298.257222101]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]

OGROPTS="-f PostgreSQL"
SRS="-a_srs EPSG:$SRID"
POST="$POST -lco DIM=2"

unzip $ZIP

echo Importing CompleteChain into database $DATABASE...
ogr2ogr $PRE $OGROPTS $SRS "PG:dbname=$DATABASE user=tiger password=&UjHudJaf3" $TIGER CompleteChain $POST

#echo Importing PolyChainLink into database $DATABASE...
#ogr2ogr $OGROPTS $SRS "PG:dbname=$DATABASE user=tiger password=&UjHudJaf3" $TIGER PolyChainLink \
#    -lco OVERWRITE=YES

echo Importing Landmarks into database $DATABASE...
ogr2ogr $PRE $OGROPTS $SRS "PG:dbname=$DATABASE user=tiger password=&UjHudJaf3" $TIGER Landmarks $POST \
    -nlt GEOMETRY

#echo Importing AreaLandmarks into database $DATABASE...
#ogr2ogr $OGROPTS $SRS "PG:dbname=$DATABASE user=tiger password=&UjHudJaf3" $TIGER AreaLandmarks \
#    -lco OVERWRITE=YES

if [ ! -z "$POLY" ]; then
  echo Extracting polygons from $TIGER...
  python $TIGERPOLY $TIGER tmp_poly_$$.shp

  echo Importing polygons into database $DATABASE...
  export PGPASSFILE=pgpass.conf
  shp2pgsql $APPEND -s $SRID -i -D Polygon Polygon | psql -U tiger -d $DATABASE

  shp2pgsql $APPEND -s $SRID -i -D AreaLandmarks AreaLandmarks | psql -U tiger -d $DATABASE
fi

#ogr2ogr $PRE -f "ESRI Shapefile" $SRS "PG:dbname=$DATABASE user=tiger password=&UjHudJaf3" Polygon.shp $POST \
#    -nlt GEOMETRY -nln polygon

#ogr2ogr $PRE -f "ESRI Shapefile" $SRS "PG:dbname=$DATABASE user=tiger password=&UjHudJaf3" AreaLandmarks.shp $POST \
#    -nlt GEOMETRY -nln polygon

#echo Updating area landmarks in database $DATABASE...
#psql -U tiger -d $DATABASE <<End
#    update landmarks set wkb_geometry =
#	(select polygonize(c.wkb_geometry)
#	    from arealandmarks a, polychainlink p, completechain c
#	    where a.land = landmarks.land
#	    and a.polyid = polyidr
#	    and (polyidl) not in (
#		select a2.polyid from arealandmarks a2 where a.land = a2.land)
#	    and c.tlid = p.tlid
#	    group by a.land)
#	where file = $FILE;
#End
#
#echo Updating point landmarks in database $DATABASE...
#psql -U tiger -d $DATABASE <<End
#    update landmarks set wkb_geometry =
#	GeomFromEWKT( 'SRID=$SRID;POINT(' || (lalong / 1000000) || ' '
#					   || (lalat / 1000000) || ')' )
#	where wkb_geometry is null
#	and lalat is not null and lalong is not null;
#End
#
#echo Fixing broken landmark polygons in database $DATABASE...
#psql -U tiger -d $DATABASE <<End
#    update landmarks
#	set wkb_geometry = Buffer(wkb_geometry, 0)
#	where file = $FILE and not isvalid(wkb_geometry);
#End
#
#echo Cleaning up database $DATABASE...
#psql -U tiger -d $DATABASE <<End
#    select DropGeometryColumn( 'tiger', 'arealandmarks', 'wkb_geometry' );
#    select DropGeometryColumn( 'tiger', 'polychainlink', 'wkb_geometry' );
#
#    drop table arealandmarks;
#    drop table polychainlink;
#End

echo Cleaning up file system...
rm -f tmp_poly_$$.{shp,shx,dbf,prj} Polygon.* AreaLandmarks.* TGR$FILE.{RT*,MET}
