# PostGIS Makefile

# (C) 2004 Paul Ramsey, pramsey@refractions.net
# (C) 2005 Markus Schaber, markus.schaber@logix-tt.com
# (C) 2005 Sandro Santilli, strk@refractions.net
# (C) 2005 Alex Bodnaru, alexbodn@012.net.il

#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# Following are some variable definitions that may have to be changed
# according to your build environment. This can be done either by editing
# this Makefile, or setting them via shell environment prior to calling make.

### Config variable section starts ###

# Configure the helper executables used during build.

JAVAC?=javac -target 1.2 -source 1.2
JAVA?=java
JAR?=jar
MKDIR?=mkdir -p
DELETE?=rm -rvf
CP?=cp

# If you happen to be stuck with ancient jar implementations that do not
# know about the -u option, does not complain about duplicate directorys
# with the same name (e. G. debian woody fastjar), set the DEBUGJAR below
# variable to postgis_debug.fastjar and JAR (above) to fastjar.
DEBUGJAR?=postgis_debug.jar

# Make sure our classpath includes your postgresql.jar. This is somehow
# difficult because some JREs (e. G. those on debian woody or sablevm) seem
# to forget their own system classes (aka bootclasspath) as soon as _any_
# classpath is set, either via environment or via command line. They do not
# implement the concept of distinct classpath and bootclasspath settings,
# as most current java implementations do. This makes it necessary to either
# explicitly include the system classes in the set classpath, or use
# non-standard options line --addclasspath to add both the postgresql.jar
# and our own classes. We also cannot set an unset CLASSPATH variable here
# for this reason. Current (Jan 2005) implementations from SUN, IBM, 
# Blackdown, gij and kaffe are known to work fine.

# If you set POSTGIS_BUILD_CLASSPATH via Environment (even to an empty string), we
# simply use it and do not try to guess anything. If POSTGIS_CLASSPATH is
# not set, we use the contents of CLASSPATH. If CLASSPATH is empty or not
# set, we try to guess a default location that works on most unixoid systems.

ifeq (${CLASSPATH},)
	POSTGIS_BUILD_CLASSPATH?=/usr/share/java/postgresql.jar
else
	POSTGIS_BUILD_CLASSPATH?=${CLASSPATH}
endif

# Set those to --addclasspath for sablevm, to avoid overriding the
# BOOTCLASSPATH, the first one is for the compiler, the second one for JRE.
COMP_ADDCP?=-classpath
EXEC_ADDCP?=-classpath

# Class path entry separator, this is : on Linux/Unix and ; on Windows
CPSEP?=:

# This are used for the online regression tests, override via environment
# as applicable. The jtest needs table creation privilege in current_schema(),
# the others only call self contained SELECTs that do not access any tables.
PGHOST?=localhost
PGPORT?=5432
PGDATABASE?=jdbc_test
PGUSER?=psql
PGPASS?=guess
PGURL?=//$(PGHOST):$(PGPORT)/$(PGDATABASE)

# Where to put the output from regression tests. If you want to have it on
# stdout, set TESTOUT to the empty string. Be careful - this will be deleted
# by "make clean".
TESTOUTPUT?=test.log

### Config variable section ends ###

# build internal constants follow:
SRCDIR=src
EXAMPLES=examples
BUILD=bin
RUNCP=$(POSTGIS_BUILD_CLASSPATH)$(CPSEP)./postgis.jar
BUILDCP=$(STUBBUILD)$(CPSEP)$(POSTGIS_BUILD_CLASSPATH)$(CPSEP)$(SRCDIR)

# Get the list of all source files
SRCCONF=sources.inc

include $(SRCCONF)

# The pgjdbc 7.2/7.4/8.0 compatibility stub stuff
STUBDIR=stubs
STUBBUILD=stubbin/

# What to put into the JAR files:
JARCONTENTS=-C $(BUILD) org \
		-C $(BUILD) examples \
		-C $(SRCDIR) org/postgresql/driverconfig.properties \
		-C . README COPYING_LGPL \
		-C .. COPYING

DEBUGJARCONTENTS=-C $(SRCDIR) org \
		-C $(SRCDIR) examples

# include version numbers
include ../Version.config

# include configuration from central repository, only needed for install
# and installdirs targets, not for building itsself.
-include ../Makefile.config

VERSION=$(JDBC_MAJOR_VERSION)_$(JDBC_MINOR_VERSION)_$(JDBC_MICRO_VERSION)
VERSIONPATH=$(BUILD)/org/postgis
VERSIONTARGET=$(VERSIONPATH)/version.properties

# Output redirect for tests
TESTRD?= >>$(TESTOUTPUT) 

# Preliminary jts support - not stable yet!
JTSDIR=jtssrc
JTSBUILD=jtsbin/

JTSPATH=$(JTSBUILD)$(CPSEP)$(BUILD)$(CPSEP)$(RUNCP)

# Now the makefile targets that do the work:
# The default target:
all:	jar \
	offlinetests

# Finding our sources
#.PHONY: sourceconfig
# avoid deletion of file
.PRECIOUS: $(SRCCONF)
$(SRCCONF):
	@echo  >$(SRCCONF) '# source files to be compiled '
	@echo >>$(SRCCONF) '# For PostGIS jdbc $(VERSION)'
	@echo >>$(SRCCONF) '# To be autogenerated by Makefile '
	@echo >>$(SRCCONF) '# File ID: $$I'd$$ #weird quoting to mask Id keyword tag from CVS
	@echo >>$(SRCCONF) 
	@echo >>$(SRCCONF) SRC = $(shell find $(SRCDIR)/ -name '*.java')
	@echo >>$(SRCCONF) 
	@echo >>$(SRCCONF) STUBSRC= $(shell find $(STUBDIR)/ -name '*.java')
	@echo >>$(SRCCONF) 
	@echo >>$(SRCCONF) JTSSRC = $(shell find $(JTSDIR)/ -name '*.java')
	@echo Regenerated $(SRCCONF).

# Packing the archives

jar:	compile postgis.jar $(DEBUGJAR)
	$(CP) postgis.jar postgis_$(VERSION).jar
	$(CP) postgis_debug.jar postgis_debug_$(VERSION).jar

postgis.jar: compile $(SRCDIR)/org/postgresql/driverconfig.properties
	$(JAR) -cf postgis.jar $(JARCONTENTS)

postgis_debug.jar: postgis.jar $(SRC)
	$(CP) postgis.jar postgis_debug.jar
	$(JAR) -uf postgis_debug.jar $(DEBUGJARCONTENTS)

# See comment for DEBUGJAR above for this rule.
postgis_debug.fastjar: compile $(SRC) $(SRCDIR)/org/postgresql/driverconfig.properties
	$(JAR) -cf postgis_debug.jar $(JARCONTENTS) $(DEBUGJARCONTENTS)

$(BUILD):
	$(MKDIR) $(BUILD)

$(STUBBUILD):
	$(MKDIR) $(STUBBUILD)

$(VERSIONPATH): $(BUILD)
	$(MKDIR) $(VERSIONPATH)

$(VERSIONTARGET): $(SRC) ../Version.config $(VERSIONPATH)
	$(CP) ../Version.config $(VERSIONTARGET)

stubcompile: $(STUBBUILD) $(SRCCONF) $(STUBSRC)
	$(JAVAC) -d $(STUBBUILD) $(STUBSRC)
	touch stubcompile

compile: stubcompile $(BUILD) $(SRCCONF) $(SRC) $(VERSIONTARGET)
	$(JAVAC) $(COMP_ADDCP) "$(BUILDCP)" -d $(BUILD) $(SRC) 
	touch compile

test:	postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/Test $(TESTRD) 

jtest:	postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestServer $(TESTRD) \
	 jdbc:postgresql:$(PGURL) $(PGUSER) $(PGPASS)

ptestoffline: postgis.jar	
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestParser $(TESTRD) offline

ptest: postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestParser $(TESTRD) \
	 "jdbc:postgresql_postGIS:$(PGURL);jdbc:postgresql_autogis:$(PGURL)" \
	 $(PGUSER) $(PGPASS)

boxtestoffline: postgis.jar	
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestBoxes $(TESTRD) offline

boxtest: postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestBoxes $(TESTRD) \
	 jdbc:postgresql_postGIS:$(PGURL) $(PGUSER) $(PGPASS)

autoregistertest: postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/TestAutoregister $(TESTRD) \
	 jdbc:postgresql:$(PGURL) $(PGUSER) $(PGPASS)

# Print all available versions
versions: postgis.jar
	$(JAVA) $(EXEC_ADDCP) "$(RUNCP)" $(EXAMPLES)/VersionPrinter jdbc:postgresql://$(PGHOST):$(PGPORT)/$(PGDATABASE) $(PGUSER) $(PGPASS)

offlinetests: boxtestoffline ptestoffline test

onlinetests: boxtest ptest jtest autoregistertest

# boxtest and ptest include boxtestoffline and ptestoffline, so we only need
# to run test in addition to the onlinetests
alltests: onlinetests test

# Cleaning
clean:
	echo cleaning...
	$(DELETE) $(BUILD) bin stubbin postgis.jar postgis_debug.jar \
		compile stubcompile jtscompile $(JTSBUILD) postgis_jts.jar \
		$(TESTOUTPUT) \
		postgis_$(VERSION).jar \
		postgis_debug_$(VERSION).jar \
		postgis_jts_$(VERSION).jar

maintainer-clean: clean
	$(DELETE) $(SRCCONF)


install: jar installdirs
	@echo Testing for successful inclusion of ../Makefile.config
	test ../Makefile.config
	$(INSTALL_DATA) postgis.jar $(DESTDIR)
	$(INSTALL_DATA) postgis_debug.jar $(DESTDIR)

installdirs:
	@mkdir -p $(DESTDIR)

# Preliminary JTS support

postgis_jts: postgis_jts.jar jtstestoffline
	@echo "Warning! This is beta code. Use at your own risk."

$(JTSBUILD):
	$(MKDIR) $(JTSBUILD)

jtscompile: compile stubcompile $(JTSBUILD) $(SRCCONF) $(JTSSRC)
	$(JAVAC) $(COMP_ADDCP) "$(BUILD)$(CPSEP)$(BUILDCP)" -d $(JTSBUILD) $(JTSSRC) 
	touch jtscompile
	
jtstestoffline: jtscompile	
	$(JAVA) $(EXEC_ADDCP) "$(JTSPATH)" $(EXAMPLES)/JtsTestParser $(TESTRD) offline

jtstest: jtscompile
	$(JAVA) $(EXEC_ADDCP) "$(JTSPATH)" $(EXAMPLES)/JtsTestParser $(TESTRD) \
	jdbc:postgres_jts:$(PGURL) $(PGUSER) $(PGPASS)

postgis_jts.jar: postgis.jar jtscompile $(SRCDIR)/org/postgresql/driverconfig.properties
	$(CP) postgis.jar postgis_jts.jar
	$(JAR) -uf postgis_jts.jar -C $(JTSBUILD) .
	$(CP) postgis_jts.jar postgis_jts_$(VERSION).jar

