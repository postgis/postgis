#!/usr/bin/env bash

# Exit on first error
set -e

if test "${CI_PIPELINE_EVENT:-}" = pull_request; then
  if test -z "${CI_COMMIT_TARGET_BRANCH:-}"; then
    echo "CI_COMMIT_TARGET_BRANCH is required for pull-request NEWS checks" >&2
    exit 1
  fi
  git fetch --no-tags origin \
    "+refs/heads/${CI_COMMIT_TARGET_BRANCH}:refs/news-check/target"
  NEWS_CHECK_BASE_REF="refs/news-check/target"
  export NEWS_CHECK_BASE_REF
fi

sh autogen.sh
./configure --without-pgconfig --prefix=/tmp/pgx
make
make check
make install
/tmp/pgx/bin/postgis help
/tmp/pgx/bin/shp2pgsql
/tmp/pgx/bin/raster2pgsql
python3 utils/test_check_all_upgrades.py
