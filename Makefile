#-----------------------------------------------------
#
# Configuration directives are in Makefile.config
#
#-----------------------------------------------------

all: liblwgeom loaderdumper

install: all liblwgeom-install loaderdumper-install

clean: liblwgeom-clean loaderdumper-clean

liblwgeom: 
	$(MAKE) -C lwgeom

liblwgeom-clean:
	$(MAKE) -C lwgeom clean

liblwgeom-install:
	$(MAKE) -C lwgeom install

loaderdumper:
	$(MAKE) -C loader

loaderdumper-clean:
	$(MAKE) -C loader clean

loaderdumper-install:
	$(MAKE) -C loader install
