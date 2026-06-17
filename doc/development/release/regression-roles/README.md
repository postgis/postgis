# Regression Harness Roles

Continuous integration workers are frequently configured with sandboxed
PostgreSQL roles that cannot create databases or install extensions. The
regression harness accepts two environment variables to test the complete
installation flow without granting superuser rights to the calling account.

`POSTGIS_REGRESS_DB_OWNER` tells `regress/run_test.pl` to hand ownership of the
temporary regression database to a privileged role while the less privileged
caller maintains the session. This mirrors production setups where database
creation is delegated to controlled roles.

`POSTGIS_REGRESS_ROLE_EXT_CREATOR` optionally overrides the role used for
extension creation when it is distinct from the database owner.

When both roles are configured in PostgreSQL to create the PostGIS extensions,
the test suite exercises the same upgrade and `CREATE EXTENSION` pathways as a
superuser-driven run. This avoids the traps described in
<https://trac.osgeo.org/postgis/ticket/5212> while keeping the CI account
unprivileged.
