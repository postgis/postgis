# FlatGeobuf

This directory contains the FlatGeobuf C++ support code and a vendored
FlatBuffers header snapshot used by PostGIS FlatGeobuf input/output.

The upstream project is <https://github.com/flatgeobuf/flatgeobuf>. The
vendored FlatGeobuf files do not carry an embedded upstream package version, so
refreshes must record the exact upstream commit or release tag used.

The FlatBuffers headers under `include/flatbuffers/` currently report version
23.3.3 in `include/flatbuffers/base.h`. Keep those headers in the
`FlatGeobuf` namespace used by this vendored copy when refreshing from
upstream.

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
  - 2023-05-05 - Upgrade FlatGeobuf dependencies.
  - 2026-07-27 - Handle FlatGeobuf byte order on big-endian platforms.
