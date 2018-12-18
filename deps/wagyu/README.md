# Wagyu

Wagyu is a library that performs polygon clipping using the [Vatti Clipping Algorithm](https://en.wikipedia.org/wiki/Vatti_clipping_algorithm).  It is based on the [Clipper Library](http://www.angusj.com/delphi/clipper.php) but it adds an extra algorith to correct the topology and guarantee validation accoding to the OGC specification.

Wagyu is a header only library but does have a dependency on [Mapbox Geometry](https://github.com/mapbox/geometry.hpp). It requires a C++ compiler that supports at least C++11.

# Liblwgeom bindings


# Changelog

  - 2018-12-18 - [Wagyu] Library extraction from https://github.com/mapbox/wagyu
  - 2018-12-18 - [geometry.hpp] Library extraction from https://github.com/mapbox/geometry.hpp