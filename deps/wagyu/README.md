# Wagyu

Wagyu is a library that performs polygon clipping using the [Vatti Clipping Algorithm](https://en.wikipedia.org/wiki/Vatti_clipping_algorithm).  It is based on the [Clipper Library](http://www.angusj.com/delphi/clipper.php) but it adds an extra algorith to correct the topology and guarantee validation accoding to the OGC specification.

Wagyu is a header only library but does have a dependency on [Mapbox Geometry](https://github.com/mapbox/geometry.hpp). It requires a C++ compiler that supports at least C++11.

# Liblwgeom bindings

To be able to use Wagyu in Postgis, we have added a function to bridge between liblwgeom's C code, and the C++ header only library: `lwgeom_wagyu_clip_by_polygon`.

It is integrated in the project as an static library integrated inside postgis.so, so it doesn't require an extra dependency at runtime besides `libstdc++` and `libm` which were already required by other libraries.

# Main things to consider about the library

- It works only with POLYGONTYPE or MULTIPOLYGONTYPE type geometries. Anything else will be dropped.
- It's currently setup to use `int64_t` values internally for extra speed. It could be changed to use double and match liblwgeom but, as it's only used by MVT, this isn't necessary.
- The library is `Intersect`ing the 2 passing geometries. It also supports `Union`, `Difference` or `XOR` but those functionalities aren't exposed.
- The library outputs the geometry in the correct winding order for MVT, which is the opposite of OGC.


# Dependency changelog

  - 2018-12-18 - [Wagyu] Library extraction from https://github.com/mapbox/wagyu
  - 2018-12-18 - [geometry.hpp] Library extraction from https://github.com/mapbox/geometry.hpp
  - 2019-01-08 - [Wagyu] Update from upstream (`insert_sorted_scanbeam`)
  - 2019-01-09 - [Wagyu] Added interrupts (lwgeom_wagyu_interrupt.h):

# Differences with upstream

The following files have differences with upstream:

- Check for interrupts:
    include/mapbox/geometry/wagyu/local_minimum_util.hpp
    include/mapbox/geometry/wagyu/process_maxima.hpp
    include/mapbox/geometry/wagyu/wagyu.hpp
