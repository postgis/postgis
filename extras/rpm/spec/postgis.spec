%define pg_version	%(rpm -q --queryformat '%{VERSION}' postgresql-devel)
%{!?jdbc2:%define jdbc2 1}
%{!?utils:%define utils 1}

Summary:	Geographic Information Systems Extensions to PostgreSQL
Name:		postgis
Version:	1.0.4
Release:	1
License:	GPL v2
Group:		Applications/Databases
Source0:	http://postgis.refractions.net/download/%{name}-%{version}.tar.gz
Source2:	postgis-jdbcdedectver.sh
Source4:	filter-requires-perl-Pg.sh
Patch10:	postgis-jdbc2-makefile.patch
Vendor:		The PostGIS Project
URL:		http://postgis.refractions.net/
BuildRequires:	postgresql-devel,proj-devel, geos-devel >= 2.1.1
Requires:	postgresql = %{pg_version} geos proj 
BuildRoot:	%{tmpdir}/%{name}-%{version}-%{release}-%(id -u -n)

%description
This package contains a module which implements GIS simple features,
ties the features to rtree indexing, and provides some spatial
functions for accessing and analyzing geographic data.

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
%setup -q -n %{name}-%{version}

%if %jdbc2
%patch10 -p0
export MAKEFILE_DIR=/usr/src/redhat/BUILD/%{name}-%{version}/jdbc2
sh %{SOURCE2}
%endif

%build
./configure --with-geos --with-proj --enable-autoconf

%{__make} \
	libdir=%{_libdir}/pgsql \
	shlib="%{name}.so"

%if %jdbc2
 make -C jdbc2
%endif

%if %utils
 make -C utils
%endif

%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT{%{_libdir}/pgsql,%{_bindir}}

%{__make} -C loader install \
	bindir=$RPM_BUILD_ROOT%{_bindir} \
	INSTALL_PROGRAM=install

install lwgeom/%{name}.so $RPM_BUILD_ROOT%{_libdir}/pgsql

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

%post
%postun

%files 
%defattr(644,root,root,755)
%doc CHANGES CREDITS README.postgis TODO doc/html examples/wkb_reader loader/README.* *.sql doc/postgis.xml  doc/ZMSgeoms.txt loader/README.pgsql2shp loader/README.shp2pgsql
%attr(755,root,root) %{_bindir}/*
%attr(755,root,root) %{_libdir}/pgsql/%{name}.so

%files jdbc2
%defattr(644,root,root,755)
%attr(755,root,root) /usr/share/java/%{name}_%{version}.jar

%if %utils
%files utils
%defattr(644,root,root,755)
%attr(755,root,root) /usr/bin/profile_intersects.pl
%attr(755,root,root) /usr/bin/test_estimation.pl
%attr(755,root,root) /usr/bin/postgis_restore.pl
%attr(755,root,root) /usr/bin/test_joinestimation.pl
%endif

%changelog
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

