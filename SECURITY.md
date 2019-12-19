# Security Policies and Procedures

If you believe you have found a security vulnerability in PostGIS please
report it to us following the procedure below. We appreciate your efforts
to disclose the issue responsibly.

## Reporting a Vulnerability

To report a security issue, please email the team at
[security@postgis.net](mailto:security@postgis.net), which is a private
maintainer-only group. The security team will reply as soon as possible
to acknowledge the receipt of your message and to discuss future steps
or request additional information.

For reporting non-security issues, please use the traditional channels
and open a [Trac ticket](https://trac.osgeo.org/postgis/)
or use the public mailing lists:

 - https://lists.osgeo.org/mailman/listinfo/postgis-users
 - https://lists.osgeo.org/mailman/listinfo/postgis-devel

To help us better diagnose the issue, please include the following
information (as much as you can provide):

- Current PostGIS version: `SELECT postgis_full_version();`.
- Current PostgreSQL version: `SELECT version();`.
- Step by step instructions to reproduce the issue.

## Procedure

Upon receiving a vulnerability report, the security team will:

* Confirm the vulnerability and the affected releases.
* Verify if there are similar problems in the code.
* Patch all releases still under maintenance and release micro versions
  including the fix.

Please note that issues in unsupported releases
(https://trac.osgeo.org/postgis/wiki/UsersWikiPostgreSQLPostGIS)
will likely not be addressed, and issues with third party dependencies
need to be reported to the team maintaining them.
