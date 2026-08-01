# FlatGeobuf

This directory contains the FlatGeobuf 3.26.2 C++ support code and vendored
FlatBuffers 23.3.3 headers used by PostGIS FlatGeobuf input/output.

The upstream project is <https://github.com/flatgeobuf/flatgeobuf>. The current
source matches FlatGeobuf tag 3.26.2. Its C++ sources and schemas are unchanged
from the 3.25.0 source imported by
<https://github.com/postgis/postgis/pull/726>. The FlatGeobuf files do not embed
that package version, so keep this record current when refreshing them.

The FlatBuffers headers under `include/flatbuffers/` currently report version
23.3.3 in `include/flatbuffers/base.h`. Keep the
`-Dflatbuffers=postgis_flatbuffers` namespace override from `Makefile.in` when
refreshing them from upstream.

# Local changes from source

  - Use the PostGIS allocator and C bridge in `flatgeobuf_c.*`.
  - Keep the vendored FlatBuffers copy in a unique namespace.
  - Preserve the PostGIS byte-order fixes for big-endian platforms.
  - The portable C++ `packedrtree.cpp` part of the big-endian fix was submitted
    upstream as https://github.com/flatgeobuf/flatgeobuf/pull/512. If it lands,
    drop that duplicated local delta during the next FlatGeobuf refresh; the
    `geometryreader.cpp` part remains PostGIS-specific glue.

# Changelog

  - 2021-09-20 - Add FlatGeobuf format input/output support.
  - 2022-05-08 - Upgrade FlatBuffers and generated FlatGeobuf headers.
  - 2023-05-05 - Upgrade to FlatGeobuf 3.25.0 and FlatBuffers 23.3.3.
  - 2026-07-27 - Handle FlatGeobuf byte order on big-endian platforms.
  - 2026-08-01 - Refresh the vendored source provenance to FlatGeobuf 3.26.2.
