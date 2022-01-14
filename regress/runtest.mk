abstopsrcdir := $(realpath $(topsrcdir))
abssrcdir := $(realpath .)

TESTS := $(patsubst $(topsrcdir)/%,$(abstopsrcdir)/%,$(TESTS))
TESTS := $(patsubst $(abssrcdir)/%,./%,$(TESTS))

check-regress:

	@echo "RUNTESTFLAGS: $(RUNTESTFLAGS)"
	@echo "RUNTESTFLAGS_INTERNAL: $(RUNTESTFLAGS_INTERNAL)"

	@$(PERL) $(topsrcdir)/regress/run_test.pl $(RUNTESTFLAGS) $(RUNTESTFLAGS_INTERNAL) $(TESTS)

	@if echo "$(RUNTESTFLAGS)" | grep -vq -- --upgrade; then \
		echo "Running upgrade test as RUNTESTFLAGS did not contain that"; \
		$(PERL) $(topsrcdir)/regress/run_test.pl \
      --upgrade \
      $(RUNTESTFLAGS) \
      $(TESTS); \
	else \
		echo "Skipping upgrade test as RUNTESTFLAGS already requested upgrades"; \
	fi

check-long:
	$(PERL) $(topsrcdir)/regress/run_test.pl $(RUNTESTFLAGS) $(TESTS) $(TESTS_SLOW)

