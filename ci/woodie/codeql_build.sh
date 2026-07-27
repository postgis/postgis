#!/bin/sh

set -e

mkdir -p build/codeql
cd build/codeql

../../configure --without-interrupt-tests
make -j1
