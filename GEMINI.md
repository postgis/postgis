# PostGIS

## Coverage Edge

The goal of this project is to add the GEOSCoveragedEdges implementation to PostGIS, exposing it as a Postgresql function, in the same manner as ST_CoverageUnion, taking in an array of geometry or a geometry and outputting a multilinestring.

The C code should all fit in the postgis/lwgeom_window.c file, but there should also be addition to the xml documentation in the appropriate file, in the postgis.sql.in file for the binding, and some entries in regression in a regress/core/coverage.sql test file and expected results file.
don't forget to gate on the GEOS version that will include this feature, which is 3.15. Same for the C code.

## Development

### Build
```
sh ../postgis.config && make clean && make```
```

### Test
```
make -j1 check
```

## See Also

* doc/developer.md
* deps/geos/geos_c.h
* postgis/lwgeom_window.c
* postgis/lwgeom_geos.c
* doc/reference_cluster.xml

## Next Steps

1. Prepare a development plan
