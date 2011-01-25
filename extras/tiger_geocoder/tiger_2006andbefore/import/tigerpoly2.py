#!/usr/bin/env python
###############################################################################
# $Id: tigerpoly.py,v 1.3 2003/07/11 14:52:13 warmerda Exp $
#
# Project:  OGR Python samples
# Purpose:  Assemble TIGER Polygons.
# Author:   Frank Warmerdam, warmerdam@pobox.com
#
###############################################################################
# Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
###############################################################################
#
#  $Log: tigerpoly.py,v $
#  Revision 1.3  2003/07/11 14:52:13  warmerda
#  Added logic to replicate all source polygon fields onto output file.
#
#  Revision 1.2  2003/07/11 14:31:17  warmerda
#  Use provided input filename.
#
#  Revision 1.1  2003/03/03 05:17:06  warmerda
#  New
#
#

import osr
import ogr
import sys, os

#############################################################################
class Module:

    def __init__( self ):
        self.lines = {}
        self.poly_line_links = {}

#############################################################################
def Usage():
    print 'Usage: tigerpoly.py infile [outfile].shp'
    print
    sys.exit(1)

#############################################################################
# Argument processing.

infile = None
outfile = None

i = 1
while i < len(sys.argv):
    arg = sys.argv[i]

    if infile is None:
	infile = arg

    elif outfile is None:
	outfile = arg

    else:
	Usage()

    i = i + 1

if infile is None:
    Usage()

#############################################################################
# Open the datasource to operate on.

ds = ogr.Open( infile, update = 0 )

poly_layer = ds.GetLayerByName( 'Polygon' )
lm_layer   = ds.GetLayerByName( 'Landmarks' )

#############################################################################
#	Create output file for the composed polygons.

shp_driver = ogr.GetDriverByName( 'ESRI Shapefile' )

shp_ds_cache = []
def create_layer (outfile, layer, type):
    if os.access( outfile, os.W_OK ):
	shp_driver.DeleteDataSource( outfile )

    defn = layer.GetLayerDefn()
    shp_ds = shp_driver.CreateDataSource( outfile )
    shp_layer = shp_ds.CreateLayer( 'out', geom_type = type )
    field_count = defn.GetFieldCount()

    for fld_index in range(field_count):
	src_fd = defn.GetFieldDefn( fld_index )
	
	fd = ogr.FieldDefn( src_fd.GetName(), src_fd.GetType() )
	fd.SetWidth( src_fd.GetWidth() )
	fd.SetPrecision( src_fd.GetPrecision() )
	shp_layer.CreateField( fd )

    shp_ds_cache.append(shp_ds)
    return shp_layer

poly_out = create_layer("Polygon.shp", poly_layer, ogr.wkbPolygon)
area_out = create_layer("AreaLandmarks.shp", lm_layer, ogr.wkbPolygon)

#############################################################################
# Read all features in the line layer, holding just the geometry in a hash
# for fast lookup by TLID.

line_layer = ds.GetLayerByName( 'CompleteChain' )
line_count = 0

modules_hash = {}

feat = line_layer.GetNextFeature()
geom_id_field = feat.GetFieldIndex( 'TLID' )
tile_ref_field = feat.GetFieldIndex( 'MODULE' )
while feat is not None:
    geom_id = feat.GetField( geom_id_field )
    tile_ref = feat.GetField( tile_ref_field )

    try:
        module = modules_hash[tile_ref]
    except:
        module = Module()
        modules_hash[tile_ref] = module

    module.lines[geom_id] = feat.GetGeometryRef().Clone()
    line_count = line_count + 1

    feat.Destroy()

    feat = line_layer.GetNextFeature()

print 'Got %d lines in %d modules.' % (line_count,len(modules_hash))

#############################################################################
# Read all polygon/chain links and build a hash keyed by POLY_ID listing
# the chains (by TLID) attached to it.

link_layer = ds.GetLayerByName( 'PolyChainLink' )

feat = link_layer.GetNextFeature()
geom_id_field = feat.GetFieldIndex( 'TLID' )
tile_ref_field = feat.GetFieldIndex( 'MODULE' )
lpoly_field = feat.GetFieldIndex( 'POLYIDL' )
lcenid_field = feat.GetFieldIndex( 'CENIDL' )
rpoly_field = feat.GetFieldIndex( 'POLYIDR' )
rcenid_field = feat.GetFieldIndex( 'CENIDR' )

link_count = 0

while feat is not None:
    module = modules_hash[feat.GetField( tile_ref_field )]

    tlid = feat.GetField( geom_id_field )

    lpoly_id = feat.GetField( lpoly_field )
    lcenid_id = feat.GetField( lcenid_field )
    rpoly_id = feat.GetField( rpoly_field )
    rcenid_id = feat.GetField( rcenid_field )

    if lcenid_id == rcenid_id and lpoly_id == rpoly_id:
	feat.Destroy()
	feat = link_layer.GetNextFeature()
	continue

    for cenid, polyid in ((rcenid_id, rpoly_id), (lcenid_id, lpoly_id)):
	if module.poly_line_links.has_key((cenid, polyid)):
	    module.poly_line_links[cenid, polyid].append( tlid )
	else:
	    module.poly_line_links[cenid, polyid] = [ tlid ]

	link_count = link_count + 1

    feat.Destroy()

    feat = link_layer.GetNextFeature()

print 'Processed %d links.' % link_count

#############################################################################
# Process all polygon features.

feat = poly_layer.GetNextFeature()
tile_ref_field = feat.GetFieldIndex( 'MODULE' )
polyid_field = feat.GetFieldIndex( 'POLYID' )
cenid_field  = feat.GetFieldIndex( 'CENID' )

poly_count = 0
degenerate_count = 0

while feat is not None:
    module = modules_hash[feat.GetField( tile_ref_field )]
    polyid = feat.GetField( polyid_field )
    cenid = feat.GetField( cenid_field )

    link_coll = ogr.Geometry( type = ogr.wkbGeometryCollection )
    tlid_list = module.poly_line_links[cenid, polyid]
    for tlid in tlid_list:
	geom = module.lines[tlid]
	link_coll.AddGeometry( geom )

    try:
	poly = ogr.BuildPolygonFromEdges( link_coll, 0, 0 )

	if poly.GetGeometryRef(0).GetPointCount() < 4:
	    degenerate_count = degenerate_count + 1
	    poly.Destroy()
	    feat.Destroy()
	    feat = poly_layer.GetNextFeature()
	    continue

	#print poly.ExportToWkt()
	#feat.SetGeometryDirectly( poly )

        feat2 = ogr.Feature(feature_def=poly_out.GetLayerDefn())

	for fld_index in range(poly_out.GetLayerDefn().GetFieldCount()):
	    feat2.SetField( fld_index, feat.GetField( fld_index ) )

	feat2.SetGeometryDirectly( poly )
	poly_out.CreateFeature( feat2 )
	feat2.Destroy()
	poly_count += 1

    except:
	raise
	print 'BuildPolygonFromEdges failed.'

    feat.Destroy()
    feat = poly_layer.GetNextFeature()

if degenerate_count:
    print 'Discarded %d degenerate polygons.' % degenerate_count

print 'Built %d polygons.' % poly_count

#############################################################################
# Extract landmarks...

area_layer = ds.GetLayerByName( 'AreaLandmarks' )
feat = area_layer.GetNextFeature()
land_field = feat.GetFieldIndex( 'LAND' )
polyid_field = feat.GetFieldIndex( 'POLYID' )
cenid_field = feat.GetFieldIndex( 'CENID' )

area = {}
while feat is not None:
    land = feat.GetField( land_field )
    polyid = feat.GetField( polyid_field )
    cenid = feat.GetField( cenid_field )

    if area.has_key(land):
	area[land].append(( cenid, polyid ))
    else:
	area[land] = [(cenid, polyid)]

    feat.Destroy()
    feat = area_layer.GetNextFeature()

feat = lm_layer.GetNextFeature()
land_field = feat.GetFieldIndex( 'LAND' )
tile_ref_field = feat.GetFieldIndex( 'MODULE' )
area_count = 0

while feat is not None:
    module = modules_hash[feat.GetField( tile_ref_field )]
    land = feat.GetField( land_field )
    if not area.has_key(land):
	# print "Can't find LAND %s in Landmarks" % land
	feat.Destroy()
	feat = lm_layer.GetNextFeature()
	continue
	
    link_coll = ogr.Geometry( type = ogr.wkbGeometryCollection )
    for cenid, polyid in area[land]:
	seen = {}
	tlid_list = module.poly_line_links[cenid, polyid]
	for tlid in tlid_list:
	    if seen.has_key(tlid):
		seen[tlid] += 1
	    else:
		seen[tlid] = 1

	for tlid in seen.keys():
	    if seen[tlid] == 1:
		geom = module.lines[tlid]
		link_coll.AddGeometry( geom )

    try:
	poly = ogr.BuildPolygonFromEdges( link_coll, 1, 1 )
    except:
	print 'BuildPolygonFromEdges failed.'
	feat.Destroy()
	feat = lm_layer.GetNextFeature()

    feat2 = ogr.Feature(feature_def=area_out.GetLayerDefn())
    for fld_index in range(area_out.GetLayerDefn().GetFieldCount()):
	feat2.SetField( fld_index, feat.GetField( fld_index ) )

    feat2.SetGeometryDirectly( poly )
    area_out.CreateFeature( feat2 )
    feat2.Destroy()

    area_count += 1

    feat.Destroy()
    feat = lm_layer.GetNextFeature()

print "Built %d area landmarks." % area_count

#############################################################################
# Cleanup

for shpds in shp_ds_cache:
    shpds.Destroy()
ds.Destroy()
