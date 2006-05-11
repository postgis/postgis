%define pg_version      %(rpm -q --queryformat '%{VERSION}' postgresql-devel)
%{!?jdbc2:%define jdbc2 0}
%{!?utils:%define utils 1}

Summary:        Geographic Information Systems Extensions to PostgreSQL
Name:           postgis
Version:        1.1.2
Release:        1
License:        GPL v2
Group:          Applications/Databases
Source0:        http://postgis.refractions.net/download/%{name}-%{version}.tar.gz
Source2:       postgis-jdbcdedectver.sh
Source4:        filter-requires-perl-Pg.sh
Vendor:         The PostGIS Project
URL:            http://postgis.refractions.net/
BuildRequires:  postgresql-devel,proj-devel, geos-devel >= 2.1.1
Requires:       postgresql = %{pg_version} geos proj
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-%(%{__id_u} -n)

%description
PostGIS adds support for geographic objects to the PostgreSQL object-relational
database. In effect, PostGIS "spatially enables" the PostgreSQL server,
allowing it to be used as a backend spatial database for geographic information
systems (GIS), much like ESRI's SDE or Oracle's Spatial extension. PostGIS
follows the OpenGIS "Simple Features Specification for SQL" and will be
submitted for conformance testing at version 1.0.

%if %jdbc2
%package jdbc2
Summary: The JDBC2 driver for PostGIS
Group: Applications/Interfaces
Provides: %{name}_%{version}.jar
Requires: postgis
                                                                                                    
%description jdbc2
The postgis-jdbc2 package provides the essential jdbc2 driver for PostGIS.
%endif

%if %utils
%package utils
Summary: The utils for PostGIS
Group: Applications/Interfaces
Requires: postgis, perl-DBD-Pg
                                                                                                    
%description utils
The postgis-utils package provides the utilities for PostGIS.
%endif
                                                                                                    
%define __perl_requires %{SOURCE4}

%prep
%setup -q

%build
make %{?_smp_mflags} PGXS=1 PGSQL_SRC=/usr/lib/pgsql/pgxs LPATH=\$\(pkglibdir\) shlib="%{name}.so"  'CFLAGS=-Wno-pointer-sign '

%if %jdbc2
export MAKEFILE_DIR=/usr/src/redhat/BUILD/%{name}-%{version}/jdbc2
sh %{SOURCE2}
 make -C jdbc2
%endif
                                                                                                    
%if %utils
 make -C utils
%endif

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT PGXS=1 PGSQL_SRC=/usr/lib/pgsql/pgxs
install lwgeom/%{name}.so $RPM_BUILD_ROOT%{_libdir}/pgsql
#install -d  $RPM_BUILD_ROOT/usr/share/pgsql/postgresql/contrib/
#install -m 755 *.sql $RPM_BUILD_ROOT/usr/share/pgsql/postgresql/contrib/
install -d  $RPM_BUILD_ROOT/usr/share/pgsql/contrib/
install -m 755 *.sql $RPM_BUILD_ROOT/usr/share/pgsql/contrib/

%if %jdbc2
# JDBC2
# Red Hat's standard place to put jarfiles is /usr/share/java
  install -d $RPM_BUILD_ROOT/usr/share/java
  install -m 755 jdbc2/%{name}_%{version}.jar $RPM_BUILD_ROOT/usr/share/java
%endif
                                                                                                    
%if %utils
 install -d $RPM_BUILD_ROOT/usr/bin
 install -m 755 utils/*.pl $RPM_BUILD_ROOT/usr/bin
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(644,root,root,755)
%doc CHANGES CREDITS README.postgis TODO doc/html loader/README.* *.sql doc/postgis.xml  doc/ZMSgeoms.txt 
%attr(755,root,root) %{_bindir}/*
%attr(755,root,root) %{_libdir}/pgsql/*.so*
#%attr(755,root,root) %{_mandir}/man1/*
%attr(755,root,root) /usr/share/pgsql/contrib/*

%if %jdbc2
%files jdbc2
%defattr(644,root,root,755)
%attr(755,root,root) /usr/share/java/%{name}_%{version}.jar
%endif

%if %utils
%files utils
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/create_undef.pl
%attr(755,root,root) %{_bindir}/test_estimation.pl
%attr(755,root,root) %{_bindir}/test_joinestimation.pl
%attr(755,root,root) %{_bindir}/postgis_restore.pl
%attr(755,root,root) %{_bindir}/profile_intersects.pl

%endif

%changelog
* Thu May 11 2006 - Laurent WANDREBECK <lw@hygeos.com> 1.1.2-1
- update to 1.1.2

* Tue Dec 22 2005 - Devrim GUNDUZ <devrim@commandprompt.com> 1.1.0-2
- Final fixes for 1.1.0

* Tue Dec 06 2005 - Devrim GUNDUZ <devrim@gunduz.org>
- Update to 1.1.0

* Mon Oct 03 2005 - Devrim GUNDUZ <devrim@gunduz.org>
- Make PostGIS build against pgxs so that we don't need PostgreSQL sources.
- Fixed all build errors except jdbc (so, defaulted to 0)
- Added new files under %utils
- Removed postgis-jdbc2-makefile.patch (applied to -head)
                                                                                                    
* Tue Sep 27 2005 - Devrim GUNDUZ <devrim@gunduz.org>
- Update to 1.0.4
                                                                                                    
* Sun Apr 20 2005 - Devrim GUNDUZ <devrim@gunuz.org>
- 1.0.0 Gold
                                                                                                    
* Sun Apr 17 2005 - Devrim GUNDUZ <devrim@gunuz.org>
- Modified the spec file so that we can build JDBC2 RPMs...
- Added -utils RPM to package list.
                                                                                                    
* Fri Apr 15 2005 - Devrim GUNDUZ <devrim@gunuz.org>
- Added preun and postun scripts.
                                                                                                    
* Sat Apr 09 2005 - Devrim GUNDUZ <devrim@gunuz.org>
- Initial RPM build
- Fixed libdir so that PostgreSQL installations will not complain about it.
- Enabled --with-geos and modified the old spec.
