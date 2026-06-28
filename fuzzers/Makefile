FUZZER_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
FUZZER_SRC_DIR ?= $(abspath $(dir $(FUZZER_MAKEFILE)))
POSTGIS_BUILD_DIR ?= $(abspath ..)
FUZZER_OUT ?= /tmp
FUZZER_WORK ?= $(FUZZER_OUT)/postgis-fuzzer-check
FUZZER_SOURCES := $(notdir $(wildcard $(FUZZER_SRC_DIR)/*_fuzzer.cpp $(FUZZER_SRC_DIR)/*_fuzzer.c))
FUZZER_NAMES := $(basename $(FUZZER_SOURCES))
SEED_CORPORA := $(wildcard $(FUZZER_SRC_DIR)/*_seed_corpus.zip)
# The standalone fuzzer smoke build includes liblwgeom.h outside liblwgeom's
# Makefile, so it must inherit the configured dependency include paths.
POSTGIS_CONFIGURED_CPPFLAGS := $(shell sed -n 's/^CPPFLAGS = //p' $(POSTGIS_BUILD_DIR)/liblwgeom/Makefile 2>/dev/null | sed 's/$$(RYU_INCLUDE)//g; s/-I$$(builddir)//g; s/-I$$(srcdir)//g' | sed 1q)
POSTGIS_CONFIGURED_LDFLAGS := $(shell sed -n 's/^LDFLAGS = //p' $(POSTGIS_BUILD_DIR)/liblwgeom/Makefile 2>/dev/null | sed 1q)
POSTGIS_CONFIGURED_SANITIZE_FLAGS := $(shell cxx="$(CXX)"; set -- $$cxx; test "$$#" -gt 0 || set -- c++; sed -n 's/^CFLAGS = //p' $(POSTGIS_BUILD_DIR)/liblwgeom/Makefile 2>/dev/null | tr ' ' '\n' | awk '/^-fsanitize/ { print }' | while read flag; do printf 'int main(void){return 0;}\n' | "$$@" "$$flag" -x c++ -fsyntax-only - >/dev/null 2>&1 && printf "%s " "$$flag"; done)
POSTGIS_FUZZER_CPPFLAGS = $(POSTGIS_CONFIGURED_CPPFLAGS) $(CPPFLAGS)
POSTGIS_FUZZER_LDFLAGS = $(POSTGIS_CONFIGURED_SANITIZE_FLAGS) $(POSTGIS_CONFIGURED_LDFLAGS) $(LDFLAGS)

.PHONY: clean dummyfuzzers check check-corpus

clean:
	$(RM) -f *.o *.a

fuzzingengine.o: $(FUZZER_SRC_DIR)/fuzzingengine.c
	$(CC) $(CFLAGS) -c -o $@ $<

dummyfuzzers: fuzzingengine.o
	mkdir -p "$(FUZZER_OUT)"
	$(AR) r libFuzzingEngine.a fuzzingengine.o
	CC="${CC}" CXX="${CXX}" CPPFLAGS="$(POSTGIS_FUZZER_CPPFLAGS)" CXXFLAGS="-L$(CURDIR) ${CXXFLAGS}" LDFLAGS="$(POSTGIS_FUZZER_LDFLAGS)" POSTGIS_BUILD_DIR="$(POSTGIS_BUILD_DIR)" SRC="$(FUZZER_SRC_DIR)" OUT="$(FUZZER_OUT)" $(FUZZER_SRC_DIR)/build_google_oss_fuzzers.sh
	OUT="$(FUZZER_OUT)" $(FUZZER_SRC_DIR)/build_seed_corpus.sh

check:
	@cxx="$${CXX:-$(CXX)}"; \
	set -- $${cxx}; \
	if [ "$$#" -eq 0 ] || ! command -v "$$1" >/dev/null 2>&1 || ! "$$@" --version >/dev/null 2>&1; then \
		echo "C++ compiler '$${cxx}' not available; skipping fuzzer smoke check."; \
	elif ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists json-c proj; then \
		echo "pkg-config metadata for fuzzer dependencies not found; skipping fuzzer smoke check."; \
	elif ! command -v xml2-config >/dev/null 2>&1 && ! pkg-config --exists libxml-2.0; then \
		echo "libxml2 fuzzer dependency helper not found; skipping fuzzer smoke check."; \
	elif ! command -v geos-config >/dev/null 2>&1 || ! command -v gdal-config >/dev/null 2>&1; then \
		echo "GEOS/GDAL fuzzer dependency helpers not found; skipping fuzzer smoke check."; \
	else \
		set -e; \
		if { command -v python3 >/dev/null 2>&1 && python3 -m zipfile --help >/dev/null 2>&1; } || \
		   { command -v unzip >/dev/null 2>&1 && unzip -v >/dev/null 2>&1; }; then \
			$(MAKE) -f "$(FUZZER_MAKEFILE)" check-corpus; \
		else \
			echo "No seed corpus zip extractor found; running fuzzer smoke check without corpus replay."; \
			$(MAKE) -f "$(FUZZER_MAKEFILE)" dummyfuzzers; \
			for fuzzer in $(FUZZER_NAMES); do \
				"$(FUZZER_OUT)/$${fuzzer}"; \
			done; \
		fi; \
	fi

check-corpus: dummyfuzzers
	@set -e; \
	for fuzzer in $(FUZZER_NAMES); do \
		"$(FUZZER_OUT)/$${fuzzer}"; \
	done
	@set -e; \
	if command -v python3 >/dev/null 2>&1 && python3 -m zipfile --help >/dev/null 2>&1; then \
		extract_zip=python3; \
	elif command -v unzip >/dev/null 2>&1 && unzip -v >/dev/null 2>&1; then \
		extract_zip=unzip; \
	else \
		echo "No seed corpus zip extractor found. Install python3 or unzip, then rerun check-corpus."; \
		exit 1; \
	fi; \
	rm -rf "$(FUZZER_WORK)"; \
	mkdir -p "$(FUZZER_WORK)"; \
	trap 'rm -rf "$(FUZZER_WORK)"' EXIT HUP INT TERM; \
	for corpus in $(SEED_CORPORA) __none__; do \
		if [ "$${corpus}" = "__none__" ]; then \
			continue; \
		fi; \
		corpus_name=$$(basename "$${corpus}"); \
		fuzzer=$${corpus_name%_seed_corpus.zip}; \
		corpus_dir="$(FUZZER_WORK)/$${fuzzer}"; \
		mkdir -p "$${corpus_dir}"; \
		if [ "$${extract_zip}" = "python3" ]; then \
			python3 -m zipfile -e "$${corpus}" "$${corpus_dir}"; \
		else \
			unzip -qq "$${corpus}" -d "$${corpus_dir}"; \
		fi; \
		find "$${corpus_dir}" -type f -print | while IFS= read -r seed; do \
			"$(FUZZER_OUT)/$${fuzzer}" "$${seed}"; \
		done; \
	done
