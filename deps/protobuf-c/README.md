# Protobuf-c

This is protobuf-c, a C implementation of the Google Protocol Buffers data serialization format. It includes libprotobuf-c, a pure C library that implements protobuf encoding and decoding,

It is included inside the Postgis directory to ease dependency management

# Details

Only the necessary code for the protobuf C library (libprotobuf-c) is copied from the repository. We currently don't include the necessary files to build protoc-c, so the files generated from the .proto are expected to be available already. Any changes to those .proto files should also regenerate and push the dependent files.

# Dependency changelog

  - 2020-09-22 - Library extraction from https://github.com/protobuf-c/protobuf-c/tree/master/protobuf-c