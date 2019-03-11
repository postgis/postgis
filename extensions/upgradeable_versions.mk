UPGRADEABLE_VERSIONS = \
	2.0.0 \
	2.0.1 \
	2.0.2 \
	2.0.3 \
	2.0.4 \
	2.0.5 \
	2.0.6 \
	2.0.7 \
	2.1.0 \
	2.1.1 \
	2.1.2 \
	2.1.3 \
	2.1.4 \
	2.1.5 \
	2.1.6 \
	2.1.7 \
	2.1.8 \
	2.1.9 \
	2.2.0 \
	2.2.1 \
	2.2.2 \
	2.2.3 \
	2.2.4 \
	2.2.5 \
	2.2.6 \
	2.2.7 \
	2.2.8 \
	2.3.0 \
	2.3.1 \
	2.3.2 \
	2.3.3 \
	2.3.4 \
	2.3.5 \
	2.3.6 \
	2.3.7 \
	2.3.8 \
	2.3.9

# This is to avoid forcing "check-installed-upgrades" as a default
# rule, see https://trac.osgeo.org/postgis/ticket/3420
all:

check-installed-upgrades:
	MODULE=$(EXTENSION); \
	TOVER=$(EXTVERSION); \
  EXDIR=`$(PG_CONFIG) --sharedir`/extension; \
	echo MODULE=$${MODULE}; \
	echo TOVER=$${TOVER}; \
	echo EXDIR=$${EXDIR}; \
  ls $${EXDIR}/$${MODULE}--*--$${TOVER}.sql \
        | grep -v unpackaged \
        | while read fname; do \
    p=`echo "$${fname}" | sed "s/.*$${MODULE}--//;s/\.sql$$//"`;  \
    FROM=`echo $${p} | sed 's/--.*//'`; \
    FF="$${EXDIR}/$${MODULE}--$${FROM}.sql"; \
    if test -f "$${FF}"; then \
      echo "Testing upgrade path $$p"; \
      $(MAKE) -C ../.. installcheck \
        RUNTESTFLAGS="-v --extension --upgrade-path $$p" \
        || { \
        echo "Upgrade path $$p failed FF=$${FF}"; \
        exit 1; }; \
    else \
        echo "No install available for upgradeable ext version $${FROM}"; \
    fi; \
  done; \

