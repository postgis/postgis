#!/usr/bin/env bash

set -e

if ! command -v apt-get >/dev/null 2>&1; then
    echo "apt-get not found"
    exit 1
fi

if [ "$(id -u)" -ne 0 ]; then
    echo "This script must run as root inside the OSS-Fuzz builder image"
    exit 1
fi

export DEBIAN_FRONTEND="${DEBIAN_FRONTEND:-noninteractive}"

apt-get update
apt-get install -y --no-install-recommends \
    make autoconf automake libtool g++ postgresql-server-dev-all \
    libgeos-dev libproj-dev libxml2-dev pkg-config libjson-c-dev \
    libc++-dev libc++abi-dev patchelf

rm -rf /var/lib/apt/lists/*
