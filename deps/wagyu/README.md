# Wagyu

Wagyu is a library that performs polygon clipping using the [Vatti Clipping Algorithm](https://en.wikipedia.org/wiki/Vatti_clipping_algorithm).  It is based on the [Clipper Library](http://www.angusj.com/delphi/clipper.php) but it adds an extra algorith to correct the topology and guarantee validation accoding to the OGC specification.

Wagyu is a header only library but does have a dependency on [Mapbox Geometry](https://github.com/mapbox/geometry.hpp). It requires a C++ compiler that supports at least C++11.

# Liblwgeom bindings

To be able to use Wagyu in Postgis, we have added a function to bridge between liblwgeom's C code, and the C++ header only library: `lwgeom_wagyu_clip_by_polygon`.

It is integrated in the project as an static library integrated inside postgis.so, so it doesn't require an extra dependency at runtime besides `libstdc++` and `libm` which were already required by other libraries.

# Main considerations about the library

- It works only with POLYGONTYPE or MULTIPOLYGONTYPE type geometries. Anything else will be dropped.
- It's currently setup to use `int32_t` values internally for extra speed. It could be changed to use to `int64_t` or to `double` to match liblwgeom but, as it's only used by MVT, this isn't necessary.
- The library is clipping the geometry to the bounding box passed. It also supports `Intersection`, `Union`, `Difference` or `XOR` of 2 geometries but those functionalities aren't exposed.
- The library outputs the geometry in the correct winding order for MVT, which is the opposite of OGC as the vertical order is switched.


# Dependency changelog

  - 2018-12-18 - [Wagyu] Library extraction from https://github.com/mapbox/wagyu
  - 2018-12-18 - [geometry.hpp] Library extraction from https://github.com/mapbox/geometry.hpp
  - 2019-01-08 - [Wagyu] Update from [upstream (`insert_sorted_scanbeam`)](https://github.com/mapbox/wagyu/commit/438050e1c8caf86d57e51b7d1bf503d000b96634)
  - 2019-01-25 - [Wagyu] Update from [upstream (interrupt support)](https://github.com/mapbox/wagyu/commit/35402803a5355983b854b07a5063014c4894f21a)
