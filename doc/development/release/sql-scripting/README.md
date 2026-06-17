# SQL Upgrade Scripting

This page covers SQL API changes, upgrade metadata comments, upgrade hooks, and
parser-sensitive SQL style used by PostGIS upgrade-script generators.

## Upgrading SQL API Functions

For most SQL API functions, nothing special is needed beyond noting `Changed:`
or `Availability:` in the relevant `.sql.in` files.

SQL API definitions live in:

| Extension | Relevant files |
|-----------|----------------|
| `postgis` | `postgis/postgis.sql.in`, `postgis/geography.sql.in`, `postgis/postgis_brin.sql.in`, `postgis/postgis_spgist.sql.in` |
| `postgis_raster` | `raster/rt_pg/rtpostgis.sql.in` |
| `postgis_sfcgal` | `sfcgal/sfcgal.sql.in` |
| `postgis_topology` | `topology/sql/*.sql.in` |

Perl scripts stitch together these SQL files and read metadata comments to
decide what to do during upgrades. `utils/create_upgrade.pl` creates upgrade
scripts.

Metadata comments precede the function, type, or object definition:

* `-- Availability: 2.0.0` records the version where the object was introduced.
* `-- Changed:` is informational. It often follows an `Availability:` comment.

```sql
-- Availability: 0.1.0
-- Changed: 2.0.0 use gserialized selectivity estimators
```

* `-- Replaces:` is informational and also instructs the upgrade generator to
  protect the user from upgrade pain. Use it instead of only `Changed:` when
  changing function inputs or outputs.

```sql
-- Availability: 2.1.0
-- Changed: 3.1.0 - add zvalue=0.0 parameter
-- Replaces ST_Force3D(geometry) deprecated in 3.1.0
```

When `utils/create_upgrade.pl` sees `Replaces`, it:

1. Finds the old definition.
2. Renames the old definition to a name such as
   `ST_Force3D_deprecated_by_postgis_310`.
3. Installs the new function definition.
4. At the end of the upgrade script, tries to drop the old function. If the old
   function is bound to user objects, it leaves the function in place and warns
   the user that objects still depend on an old signature.

This is needed because objects bind to OIDs, not names. If a view or
materialized view uses the old signature, it will be bound to the deprecated
function. Dropping it with `CASCADE` would destroy user objects.

For objects such as types and casts, comments are not sufficient for
`create_upgrade.pl` to do the right thing. If removing a signature with no
replacement, put the cleanup in the extension's `after_upgrade` script. If
something must be dropped or changed before the new function can be installed,
put it in the `before_upgrade` script.

Relevant upgrade-hook files:

* `postgis`: `postgis/postgis_before_upgrade.sql`,
  `postgis/postgis_after_upgrade.sql`
* `postgis_raster`: `raster/rt_pg/rtpostgis_drop.sql.in`,
  `raster/rt_pg/rtpostgis_upgrade_cleanup.sql.in`
* `postgis_sfcgal`: `sfcgal/sfcgal_before_upgrade.sql.in`,
  `sfcgal/sfcgal_after_upgrade.sql.in`
* `postgis_topology`: `topology/topology_before_upgrade.sql.in`,
  `topology/topology_after_upgrade.sql.in`

Helper functions are defined in `postgis/common_before_upgrade.sql` and dropped
in `postgis/common_after_upgrade.sql`:

* `_postgis_drop_function_by_signature` drops by input types and is usually the
  preferred helper.
* `_postgis_drop_function_by_identity` is useful when only input type names are
  changing.

Topology upgrade helpers added in 3.6.0 for data-type and table changes:

* `_postgis_drop_cast_by_types`
* `_postgis_add_column_to_table`

## SQL Parser Rules

Keep these rules when writing or editing SQL in `postgis/*.sql.in` and upgrade
scripts. They match how the Perl generators parse and build upgrade and
uninstall scripts.

Use `$$` as the `DO` delimiter. Do not use named delimiters like `$func$`.

```sql
DO $$
BEGIN
  -- your block
END;
$$ LANGUAGE plpgsql;
```

Left-align `CREATE` statements inside `DO`. Anything created in a `DO` block is
picked up by uninstall only if the `CREATE ...` lines are not indented.

```sql
DO $$
BEGIN
CREATE OR REPLACE FUNCTION public.myfunc(...) RETURNS ...
LANGUAGE sql AS $$ SELECT 1 $$;
END;
$$;
-- Good: CREATE starts at column 1
```

When changing signatures or argument names, use the drop hooks:

* `postgis/postgis_before_upgrade.sql` drops or de-aliases old signatures before
  install or upgrade runs.
* `postgis/postgis_after_upgrade.sql` cleans up stragglers after new objects
  exist.

Typical cases include:

* adding default arguments to a function that previously had none
* renaming argument names that would create ambiguity across versions
* replacing one signature with another without leaving duplicates

```sql
-- postgis/postgis_before_upgrade.sql
DROP FUNCTION IF EXISTS public.myfunc(oldtype, oldtype);

-- main install/upgrade then creates the new signature(s)
```

Record `Availability:` and `Changed:` in script comments. The upgrade generator
scans these markers to decide which upgrades need to ship an object, based on
the first `Availability:`, and to emit notes into generated upgrade paths about
behavior or ABI changes from `Changed:`.

```sql
-- Availability: 3.7.0
-- Changed: 3.8.0 behavior on NULL input
CREATE OR REPLACE FUNCTION public.myfunc(...) RETURNS ...
...
```

Quick checklist:

* `DO` uses `$$`, not `$func$`.
* `CREATE` lines start at column 1 inside `DO`.
* Signature and argument changes use `postgis/postgis_before_upgrade.sql` and
  `postgis/postgis_after_upgrade.sql` when needed.
* `Availability:` and `Changed:` lines are current and minimal.
