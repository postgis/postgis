
# this file copied and adapted from PostgreSQL source
# to allow easy build on BSD systems

all install uninstall staged-install clean distclean maintainer-clean test check docs docs-install docs-uninstall utils:
	@if [ ! -f GNUmakefile ] ; then \
		echo "You need to run the 'configure' program first. See the file"; \
		echo "'README.postgis' for installation instructions" ; \
		false ; \
	fi
	@IFS=':' ; \
	 for dir in $$PATH; do \
	   for prog in gmake gnumake make; do \
	     if [ -f $$dir/$$prog ] && ( $$dir/$$prog -f /dev/null --version 2>/dev/null | grep GNU >/dev/null 2>&1 ) ; then \
	       GMAKE=$$dir/$$prog; \
	       break 2; \
	     fi; \
	   done; \
	 done; \
	\
	 if [ x"$${GMAKE+set}" = xset ]; then \
	   echo "Using GNU make found at $${GMAKE}"; \
	   $${GMAKE} $@ ; \
	 else \
	   echo "You must use GNU make to build PostGIS." ; \
	   false; \
	 fi

# This target must also work before configure, so dedicated CI can audit
# contributor credits from a clean checkout with full Git history.
.PHONY: check-contributor-credits
check-contributor-credits:
	@if test ! -e ".git"; then \
		echo "SKIP: contributor credits require a Git worktree"; \
	else \
		python3 -B utils/test_check_contributor_credits.py && \
		python3 -B utils/check_contributor_credits.py --repo .; \
	fi
