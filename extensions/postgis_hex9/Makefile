# Makefile — standalone development build for postgis_hex9.
#
# In-tree PostGIS PR integration will use a Makefile.in driven by configure
# (TODO: not yet wired into the top-level extensions/Makefile).
#
# Usage (from extensions/postgis_hex9/):
#   make
#   sudo make install
#   make installcheck   # requires PostgreSQL with postgis already loaded

PG_CONFIG   ?= pg_config
PGXS        := $(shell $(PG_CONFIG) --pgxs)

# ── Extension identity ────────────────────────────────────────────────────────
EXTENSION   = postgis_hex9
MODULE_big  = postgis_hex9-3
# Objects live under ./build/ so Xcode (which scans the source dir for .o
# inputs) doesn't pick them up and double-link the same symbols. Both the
# pattern rule below and PGXS's link step see these paths consistently.
OBJS        = build/lwgeom_hex9.o build/h9_warp_runtime.o
DATA        = postgis_hex9--1.1.0.sql postgis_hex9--1.0.0--1.1.0.sql

# ── Regression tests ──────────────────────────────────────────────────────────
REGRESS      = hex9
REGRESS_OPTS = --load-extension=postgis --load-extension=postgis_hex9

# ── PostGIS source headers ────────────────────────────────────────────────────
# Headers come from the configured PostGIS source tree (not the working branch).
POSTGIS_SRC := /Users/ben/Documents/Projects/postgis-3.6.2
PG_CPPFLAGS  = \
    -I$(POSTGIS_SRC)/liblwgeom \
    -I$(POSTGIS_SRC)/libpgcommon \
    -I$(POSTGIS_SRC) \
    -I.

# ── macOS: postgis-3.dylib is a Mach-O bundle; cannot link against it. ────────
# liblwgeom symbols are resolved at runtime in _PG_init via h9_lwgeom_resolve()
# (see h9_lwgeom_shim.h) — load_file($libdir/postgis-3) + dlsym — so no LOAD is
# needed in hex9.sql.in and the module loads in any backend order. The module
# carries no undefined liblwgeom symbols, but -undefined dynamic_lookup is kept
# for the remaining PG-core symbols resolved via -bundle_loader postgres.
ifeq ($(shell uname -s),Darwin)
SHLIB_LINK = -undefined dynamic_lookup
endif

include $(PGXS)

# ── macOS: fix stale SDK path baked into pg_config --cppflags after Xcode upgrade
ifeq ($(shell uname -s),Darwin)
  _REAL_SDK := $(shell xcrun --sdk macosx --show-sdk-path 2>/dev/null)
  ifneq ($(_REAL_SDK),)
    override CFLAGS   := $(shell echo "$(CFLAGS)"   | sed "s|-isysroot [^ ]*|-isysroot $(_REAL_SDK)|g")
    override CPPFLAGS := $(shell echo "$(CPPFLAGS)" | sed "s|-isysroot [^ ]*|-isysroot $(_REAL_SDK)|g")
    override LDFLAGS  := $(shell echo "$(LDFLAGS)"  | sed "s|-isysroot [^ ]*|-isysroot $(_REAL_SDK)|g")
  endif
endif

# ── C++ compilation ───────────────────────────────────────────────────────────
# PGXS only knows about .c; add an explicit pattern rule for .cpp.
# Output goes into build/ — see OBJS comment.
# H9_GIT_REV stamps every build so h9_version() can be matched against the
# loaded dylib (`-dirty` suffix when working tree has uncommitted edits).
H9_GIT_REV := $(shell git -C "$(dir $(lastword $(MAKEFILE_LIST)))" describe --always --dirty 2>/dev/null || echo unknown)
build/%.o: %.cpp | build
	$(CXX) $(CFLAGS) -std=c++17 -fno-exceptions $(CPPFLAGS) $(PG_CPPFLAGS) -DH9_GIT_REV='"$(H9_GIT_REV)"' -MMD -MP -c -o $@ $<

# Header dependency tracking: -MMD -MP emits build/*.d listing each object's
# header prerequisites, so editing a header (e.g. h9_ct.h, h9_grid.h) triggers
# a rebuild. Without this, header-only changes silently produced a stale .dylib.
-include $(OBJS:.o=.d)

build:
	mkdir -p build

clean: clean-build
.PHONY: clean-build
clean-build:
	rm -rf build

# ── SQL file generation ───────────────────────────────────────────────────────
MODULE_PATH := $(shell $(PG_CONFIG) --pkglibdir)/$(MODULE_big)

postgis_hex9--1.1.0.sql: hex9.sql.in
	sed 's|MODULE_PATHNAME|$(MODULE_PATH)|g' $< > $@

all: postgis_hex9--1.1.0.sql

clean: clean-hex9-generated
.PHONY: clean-hex9-generated
clean-hex9-generated:
	rm -f postgis_hex9--1.1.0.sql
