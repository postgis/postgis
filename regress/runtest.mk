abstopsrcdir := $(realpath $(topsrcdir))
abssrcdir := $(realpath .)

TESTS := $(patsubst $(topsrcdir)/%,$(abstopsrcdir)/%,$(TESTS))
TESTS := $(patsubst $(abssrcdir)/%,./%,$(TESTS))


check-regress:

	@$(PERL) $(topsrcdir)/regress/run_test.pl $(RUNTESTFLAGS) $(TESTS)

	#
	# Will now run upgrade test if RUNTESTFLAGS was not already doing that
	#

	@if echo "$(RUNTESTFLAGS)" | grep -vq -- --upgrade; then \
		$(PERL) $(topsrcdir)/regress/run_test.pl --upgrade $(RUNTESTFLAGS) $(TESTS); \
	fi

check-long:
	$(PERL) $(topsrcdir)/regress/run_test.pl $(RUNTESTFLAGS) $(TESTS) $(TESTS_SLOW)

