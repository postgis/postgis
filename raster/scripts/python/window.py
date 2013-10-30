#! /usr/bin/env python
#
# $Id: window.py 9324 2012-02-27 22:08:12Z pramsey $
#
# Calculates coordinates of window corners of given raster dataset.
# It's just a simple helper for testing and debugging WKT Raster.
#
##############################################################################
# Copyright (C) 2009 Mateusz Loskot <mateusz@loskot.net>
# 
# This is free software; you can redistribute and/or modify it under
# the terms of the GNU General Public Licence. See the COPYING file.
##############################################################################
from osgeo import gdal
from osgeo import osr
import osgeo.gdalconst as gdalc
import sys

if len(sys.argv) != 6:
    print "Usage: window.py <raster> <x> <y> <xsize> <ysize>"
    print "\traster - GDAL supported dataset"
    print "\tx - column - 1..N where N is raster X dimension"
    print "\ty - row - 1..N where N is raster Y dimension"
    print "\txsize - x-dimension of requested window (xsize <= xsize of raster - x)"
    print "\tysize - y-dimension of requested window (ysize <= ysize of raster - y)"
    sys.exit(0)

def is_georeferenced(gt):
    if gt[0] == 0.0 and gt[1] == 1.0 and gt[3] == 0.0 and gt[5] == 1.0:
        return False
    else:
        return True

def calculate_corner(gt, x, y):
    if is_georeferenced(gt):
        xgeo = gt[0] + gt[1] * x + gt[2] * y
        ygeo = gt[3] + gt[4] * x + gt[5] * y
    else:
        xgeo = x
        xgeo = y
    return (xgeo, ygeo)

infile = sys.argv[1]
inxoff = int(sys.argv[2])
inyoff = int(sys.argv[3])
inxsize = int(sys.argv[4])
inysize = int(sys.argv[5])
print "=== INPUT ==="
print "File: %s" % infile
print "Window:"
print "- upper-left: %d x %d" % (inxoff, inyoff)
print "- dimensions: %d x %d" % (inxsize, inysize)

ds = gdal.Open(infile, gdalc.GA_ReadOnly);
if ds is None:
    sys.exit('Cannot open input file: ' + str(infile))

xsize = ds.RasterXSize
ysize = ds.RasterYSize
print "=== RASTER ==="
print "- dimensions: %d x %d" % (xsize, ysize)

if inxsize > xsize or inysize > ysize or inxoff > xsize or inyoff > ysize:
    print "Invalid size of input window"
    sys.exit(1)

gt = ds.GetGeoTransform()
res = ( gt[1], gt[5] ) # X/Y pixel resolution
ulp = ( gt[0], gt[3] ) # X/Y upper left pixel corner
rot = ( gt[2], gt[4] ) # X-/Y-axis rotation

if is_georeferenced(gt):
    print "- pixel size:", res
    print "- upper left:", ulp
    print "- rotation  :", rot
else:
    print "No georeferencing is available"
    sys.exit(1)

print "=== WINDOW ==="
print "- upper-left :", calculate_corner(gt, inxoff, inyoff)
print "- lower-left :", calculate_corner(gt, inxoff, ysize)
print "- upper-right:", calculate_corner(gt, xsize, inyoff)
print "- lower-right:", calculate_corner(gt, xsize, ysize)
print "- center     :", calculate_corner(gt, xsize/2, ysize/2)
