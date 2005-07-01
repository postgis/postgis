#!/usr/bin/make -f
# -*- makefile -*-

#prepare control and data files for a library package, taking the library SONAME
#into account: replace the string #SOVER# in the file names with the SONAME
#automatically found in the library using objdump.

# shared library version
SONAME:=$(shell objdump -p ${LIB_NAME} | grep SONAME | \
          awk '{if (match($$2,/\.so\.[0-9]+$$/)) print substr($$2,RSTART+4)}')
INDIR=$(CURDIR)/debian/sofiles.in
OUTDIR=$(CURDIR)/debian
SOVER=\#SOVER\#
SOSUBST=-e s/$(SOVER)/$(SONAME)$(EXTRA_VER)/g
SOINSTALL2FILES=-e s/\.install$$/\.files/

INFILES2:=$(notdir $(wildcard $(INDIR)/*))
MKINFILE2=sed $(SOSUBST) $(INDIR)/$(FILE) > $(OUTDIR)/`echo $(FILE) | sed $(SOSUBST)`
RMINFILE2=rm -f $(OUTDIR)/`echo $(FILE) | sed $(SOSUBST)`

INFILES3:=$(notdir $(wildcard $(INDIR)/*.install))
MKINFILE3=sed $(SOSUBST) $(INDIR)/$(FILE) > $(OUTDIR)/`echo $(FILE) | sed $(SOSUBST) $(SOINSTALL2FILES)`
RMINFILE3=rm -f $(OUTDIR)/`echo $(FILE) | sed $(SOSUBST) $(SOINSTALL2FILES)`

clean:
	mv $(OUTDIR)/control $(OUTDIR)/control.bak
	$(foreach FILE,$(INFILES2),$(RMINFILE2);)
	mv $(OUTDIR)/control.bak $(OUTDIR)/control
	$(foreach FILE,$(INFILES3),$(RMINFILE3);)
	rm -f $(OUTDIR)/*.substvars
	rm -f $(OUTDIR)/*.debhelper
	
build:
	$(foreach FILE,$(INFILES2),$(MKINFILE2);)
	$(foreach FILE,$(INFILES3),$(MKINFILE3);)

soname:
	echo SONAME=$(SONAME)

.PHONY: clean build soname
