# TODO: always pass --tiger when run_test.pl supports it non-extension based
# TODO: run actual tests

ifeq (,$(findstring --extension,$(RUNTESTFLAGS)))
  override RUNTESTFLAGS := $(RUNTESTFLAGS) --tiger
endif
