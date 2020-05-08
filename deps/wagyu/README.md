# Wagyu

Wagyu is a library that performs polygon clipping using the [Vatti Clipping Algorithm](https://en.wikipedia.org/wiki/Vatti_clipping_algorithm).  It is based on the [Clipper Library](http://www.angusj.com/delphi/clipper.php) but it adds an extra phase to correct the topology and guarantee validation accoding to the OGC specification.

Wagyu is a header only library but depends on [Mapbox Geometry](https://github.com/mapbox/geometry.hpp). It requires a C++ compiler that supports at least C++11.

# liblwgeom bindings

To be able to use Wagyu in Postgis, several functions have been added as a bridge between liblwgeom's C code, and the C++ header only library:
  - `lwgeom_wagyu_clip_by_box`: Main function to clip and force validation over a polygon.
  - `libwagyu_version`: Returns a static string with the version of the wagyu library.
  - `lwgeom_wagyu_interruptRequest`: Function to request the interruption of the wagyu library.

It is integrated in the project as an static library inside postgis.so, so it doesn't require an extra dependency at runtime besides `libstdc++` and `libm` which were a requisite.

# Main considerations about the library

- It works only with POLYGONTYPE or MULTIPOLYGONTYPE type geometries. Anything else will be dropped.
- It's currently setup to use `int32_t` values internally for extra speed. It could be changed to use to `int64_t` or to `double` to match liblwgeom but, as it's only used by MVT, this isn't necessary.
- The library is clipping the geometry to the bounding box passed. It also supports `Intersection` of 2 geometries, `Union`, `Difference` or `XOR` of 2 geometries but those functionalities aren't exposed.
- The library outputs the geometry in the correct winding order for **MVT**, which is the opposite of OGC as the vertical order is switched.


# Dependency changelog

  - 2019-02-05 - [Wagyu] Library extraction from https://github.com/mapbox/wagyu
  - 2019-02-05 - [geometry.hpp] Library extraction from https://github.com/mapbox/geometry.hpp
  - 2020-05-08 - [Wagyu] Update to 0.5.0
