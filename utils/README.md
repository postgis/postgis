# PostGIS utilities

This directory contains repository maintenance and development tools that do
not belong to one build subsystem.

## General utilities

* `test_estimation.pl` tests the selectivity estimator for the `&&` overlap
  operator.
* `postgis_restore.pl` restores a spatial database dump into a new or upgraded
  PostGIS database.
* `create_upgrade.pl` creates a PostGIS procedure upgrade script and reports
  changes that cannot be upgraded cleanly.
* `profile_intersects.pl` compares `distance() = 0` and `intersects()` timings.

Documentation builders, validators, and publishing helpers live in
[`utils/docs/`](docs/README.md). Keeping them below one directory makes their
ownership visible without treating every script in `utils/` as documentation
infrastructure.
