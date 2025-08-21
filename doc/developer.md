-------------------------------------------------------------------------
title:  PostGIS Developer How-To
author:	Name: Regina Obe
date:   "2025-08-12"
category: Development Docs
-------------------------------------------------------------------------

# PostGIS Developer How-To
## Terminology

* PostGIS version naming are 3 digits separated by a period

  Major.Minor.Patch

| Version | Meaning | Example |
|---------|---------|---------|
| `3.0.0` | Major release (new API surface) | First release of a major version |
| `3.6.0` | Minor release (new features, backwards‑compatible) | Added new functions |
| `3.0.1` | Patch release (bug fixes, data changes) | Small fixes to `3.0.0` |

* SQL Api functions are functions that are exposed to the user in the database.
  They are either SQL, plpgsql, or C backed functions.

* C Api functions are library functions that back SQL Api functions.

Naming of library files
========================
Since PostGIS 3.0 library files are major versioned.  Prior to 3 they were minor versioned.
This means you'll find in the wild the following - the extension .so will be different based on OS

 - postgis-2.4.so, postgis_topology-2.4.so, rtpostgis-2.4.so
 - postgis-2.5.so, postgis_topology-2.5.so, rtpostgis-2.5.so
 - postgis-3.so, postgis_raster-3.so, postgis_sfcgal-3.so, postgis_topology-3.so

PostGIS 3 was a bit rocky in that some things were missed in 3.0 release and fixed in 3.1.
Namely, postgis_sfcgal prior to 3.1 was embedded in the postgis-*.so which meant not all postgis libs
had the same set of functions, which was BAD.  When 3.1 rolled around the siamese beast was separated to form
postgis-3.so, postgis_sfcgal-3.so.

Also in 3.0.0, the raster functionality was removed from the postgis extension and moved to an extension called postgis_raster.
The libraries of these were always separate, however the name rtpostgis- that packaged all the raster functions, did not match the extension
name so was renamed, but for all purposes rtpostgis of the past is equivalent to postgis_raster of the present.

All these changes were made to ease upgrade pains with pg_upgrade.
As a developer you still have the option of having your lib files minor versioned 
for easier testing in the same cluster. 

To take advantage of this feature, when configuring postgis compile, use the switch *--with-library-minor-version*

## Does and Don'ts

* Don't introduce new SQL Api functions in a Patch release.
* Don't change structure definitions e.g. geometry_columns, geometry/raster type
  in a patch release.

* Do introduce new SQL Api functions in a Minor release.
* Functions that are not exposed via the SQL Api can be introduced any time.
* Only major versions can remove SQL api functions
  or C api functions without stubbing (which we will cover shortly).

* Only PostGIS first release of a major can introduce functionality that requires a pg_dump / pg_restore. 
* Don't require newer versions of library in a micro, but you can require new versions in first release of a minor.
  For example we often drop support for older versions of GEOS and PostgreSQL in a new minor release.



## When removing objects impacts upgrade

There are several types of removals that impact user upgrades and should be carefully thought thru.

* SQL Api functions
* C Api functions
* Types, Views, Tables are also exposed via SQL Api

Functions internal to postgis that are never exposed and only used within postgis libraries 
can be shuffled around to your hearts content.


## UPGRADING C Api functions 

You should avoid ever removing C Api function in Minor and patch releases of PostGIS.

If there is a C Api function that you badly want to remove you need to stub it so the signature still 
exists but throws an error.

These functions should be removed from whatever file 
they were in and stubbed in a deprecation file. Ideally you should never do this in a micro. Only minor.
For Major such as when PostGIS 4 comes out we could in theory skip the legacy file and just chuck the function entirely.
We could even blank out all the legacy files.

A function can be stubbed in 3.0.0, but not 3.0.1, though there are cases where you might as long as 
you carefully fix up the SQL signature exposing it. The reason to avoid not doing this in a micro is people often do not 
run ALTER EXTENSION or SELECT postgis_extensions_upgrade() in a micro, so taking these out will break production code.

  * For the postgis extension, these should go in postgis/postgis_legacy.c the stub will look something like this. 

      POSTGIS_DEPRECATE("2.0.0", postgis_uses_stats)

    Note the specification of the version it was removed and the name of the function

  * For postgis_sfcgal, deprecated C api functions should go in sfcgal/postgis_sfcgal_legacy.c 
  * For postgis_raster, raster/rt_pg/rtpg_legacy.c
  * postgis_topology extension has never had any deprecated functions so there is currently no legacy file for it.
    If there comes a need to deprecate C functions, then a file topology/postgis_topology_legacy.c will be created to store these.
  * postgis_tiger_geocoder is all sql and plpgsql so it has no C backing functions. 

Why do we even bother replacing a good function with a function that throws an error?  Because of pg_upgrade tool used 
to upgrade PostgreSQL clusters. When pg_upgrade runs, it does not use the regular CREATE EXTENSION routine that loads function definitions from 
a file. Instead it uses a naked CREATE EXTENSION and then tries to load all functions /types /etc/ from the old databases as they existed
meaning they point at the same .so, .dll, .dylib whatever.  When it tries to load these in, it validates the library to make sure said 
functions exist in the library.  If these functions don't it will bail and pg_upgrade will fail.  It however will do fine 
not complain if the function exists even if all the function knows how to do is throw an error.

WHY oh WHY does it use old signatures in an old database instead of a fresh CREATE EXTENSION install?
Primarily because objects are referenced by object identifiers AKA oids in views, tables, you name it, and if you create new function 
from scratch, even if it has the same exact definition as the old, it does not have the same OID.
As such all db internal references would be broken 
if you try to overlay the old def structures onto the new extension install.

So this whole care of legacy functions is to appease pg_upgrade.


## UPGRADING SQL Api functions 

For most SQL Api functions, nothing special needs to be done beyond
noting a CHANGED in version or AVAILABILITY in the respective .sql.in files.

The SQL Api definitions are found in following places:

| Extension | Relevant Files |
|-----------|----------------|
| `postgis` | `postgis/postgis_sql.in`, `postgis/geography_sql.in`, `postgis/postgis_bin_sql.in`, `postgis/postgis_spgist_sql.in` |
| `postgis_raster` | `raster/rt_pg/rtpostgis_sql.in` |
| `postgis_sfcgal` | `sfcgal/sfcgal_sql.in` |
| `postgis_topology` | `topology/sql/*.sql.in` |

We use perl scripts to stitch together these various SQL and 
read meta comments to determine what to do during an upgrade.
The utils/create_upgrade.pl is the script that is tasked with creating upgrade scripts.

The various notes you put in .sql.in files take the following form and precede the function/type/etc definition:

* -- Availability: Is only informational 2.0.0 where 2.0.0 represents the version it was introduced in.
* -- Changed: Is only informational. You'll often see an Availability comment followed by a changed comment.
    e.g. 

        -- Availability: 0.1.0
        -- Changed: 2.0.0 use gserialized selectivity estimators
  
* -- Replaces: is both informational and also instructs the perl upgrade script to protect the user from some upgrade pains.
     You use Replaces instead of just a simple Changed, if you are changing inputs to the function or changing 
     outputs of the function. So any change to the api

    Such a comment would look something like:

      -- Availability: 2.1.0
      -- Changed: 3.1.0 - add zvalue=0.0 parameter
      -- Replaces ST_Force3D(geometry) deprecated in 3.1.0

    When utils/create_upgrade.pl script comes across a Replaces clause, the perl script will change the statement to do the following:

     1) Finds the old definition
     2) renames the old definition to ST_Force3D_deprecated_by_postgis_310 
     3) Installs the new version of the function.
     4) At the end of the upgrade script, it will try to drop the function. If the old function is bound 
       to user objects, it will leave the old function alone and warn the user as part of the upgrade, that they have objects bound to an old signature. 

    Why do we do this?  Because all objects are bound by oids and not names. So if a user has a view or materialized view, it will be bound to 
    ST_Force3D_deprecated_by_postgis_310 and not the new ST_Force3D.  We can't drop things bound to user objects without executing a 
    DROP ... CASCADE, which will destroy user objects.


For some objects such as types and casts, comments are not sufficient to get create_upgrade.pl to do the right thing.
In these cases you need to do this work yourself as needed.
Case in point, if you are removing a signature and have no replacement for it. You just want to drop it.
You'll put the code in the relevant **after_upgrade** script corresponding to the extension you are changing.
If you need something dropped or need to make some system changes that would prevent new function from being installed, then you would put this in a **before_upgrade** script.

The relevant files for each extension are as follows:

* postgis - postgis/postgis_before_upgrade.sql , postgis/postgis_after_upgrade.sql
* postgis_raster - raster/rtpostgis_drop.sql.in, rtpostgis_upgrade_cleanup.sql.in 
    (the rtpostgis_upgrade_cleanup.sql.in is equivalent to the after_upgrade script of other extensions)
* postgis_sfcgal - sfcgal/sfcgal_before_upgrade.sql.in, sfcgal/sfcgal_after_upgrade.sql.in
* postgis_topology - topology/topology_before_upgrade.sql.in, topology/topology_after_upgrade.sql.in

Helper function calls you'll find in these scripts are the following and are defined in postgis/common_before_upgrade.sql and dropped in 
postgis/common_after_upgrade.sql

* _postgis_drop_function_by_signature - generally you want to use this one as it only cares about input types of functions
* _postgis_drop_function_by_identity - this one you'll want to use if for some awful reason, all you are doing is changing the names of input types.

These ones are new in 3.6.0 to handle the very delicate major upgrade of topology which required changing of data types and tables

* _postgis_drop_cast_by_types
* _postgis_add_column_to_table


Dependency Library Guarding
============================

On many occasions, we'll introduce functionality that can only be used if PostGIS is compiled
with a dependency library higher than X? Where X is some version of a dependency library.

Dependency guards need to be put in both the C library files and our test files.
On some occassions where we need to do something different based on version of PostgreSQL,
you'll see guards in the SQL files as well.

We have guards in place in the code to handle these for dependency libraries

* PostgreSQL
  - sql.in and c files
    
    ```c
    #if POSTGIS_PGSQL_VERSION >= 150
    /* code that requires PostgreSQL 15+ */
    #endif
    ```
  - regress/../tests.mk.in


* GEOS 
  - c: 
    ```c
      #if POSTGIS_GEOS_VERSION < 31000
      /* GEOS < 3.1 code goes here */
      #endif
    ```
  - test files:
      * **`regress/**/tests.mk.in`** 
      * raster/rt_pg/tests/tests.mi.in

    ```makefile
       ifeq ($(shell expr "$(POSTGIS_GEOS_VERSION)" ">=" 31000),1)
        TESTS += \
          # add tests that require GEOS 3.1 or higher to run
       endif
    ```

* SFCGAL
  - c files
    ```c
      #if POSTGIS_SFCGAL_VERSION >= 20100
      /* SFCGAL 2.1+ required */
      #endif
    ```
  - tests: 
    * sfcgal/regress/tests.mk.in

    ```makefile
    ifeq ($(shell expr "$(POSTGIS_SFCGAL_VERSION)" ">=" 20100),1)
      TESTS += \
        # add tests that require SFCGAL 2.1 or higher
    endif
    ```
* PROJ
  - c files
    ```c
      #if POSTGIS_PROJ_VERSION > 60000
        /* Code to run for Proj 6.0 or higher */
      #endif
    ```

* GDAL
  - c files 
    ```c
      #if POSTGIS_GDAL_VERSION < 30700
          /* Logic for GDAL < 3.7 **/
      #endif
    ``` 

Even if a user can't use a certain function, that function still needs to be exposed,
but instead of doing something functional, will output an error that the extension needs to be compiled
with lib xxx or higher. Also the function has to be available in the C lib, as such the guarding is always done
on the C lib side and very rarely on the sql files.






