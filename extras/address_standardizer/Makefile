OBJS = \
    address_parser.o \
    address_standardizer.o \
    std_pg_hash.o \
    analyze.o \
    err_param.o \
    export.o \
    gamma.o \
    hash.o \
    lexicon.o \
    pagc_tools.o \
    parseaddress-api.o \
    standard.o \
    tokenize.o

OBJS_test_main = \
    test_main.o \
    analyze.o \
    err_param.o \
    export.o \
    gamma.o \
    hash.o \
    lexicon.o \
    pagc_tools.o \
    standard.o \
    tokenize.o


MODULE_big = address_standardizer
EXTENSION = address_standardizer
DATA_built = address_standardizer--1.0.sql us-lex.sql us-gaz.sql us-rules.sql
DOCS = README.address_standardizer
PG_CPPFLAGS = -g -O0
SHLIB_LINK = -lpcre
EXTRA_CLEAN = usps-st-city-name.txt mk-st-regexp mk-city-regex test_main

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
PGVER := $(shell $(PG_CONFIG) --version)
include $(PGXS)

PERL := $(shell which perl)

address_standardizer--1.0.sql: address_standardizer--1.0.sql.in
	$(PERL) mk-sql.pl '$(PGVER)' address_standardizer--1.0.sql.in > address_standardizer--1.0.sql

us-lex.sql: lexicon.csv
	$(PERL) pagc-data-psql lex lexicon.csv > us-lex.sql

us-gaz.sql: gazeteer.csv
	$(PERL) pagc-data-psql gaz gazeteer.csv > us-gaz.sql

us-rules.sql: rules.txt
	$(PERL) pagc-data-psql rules rules.txt > us-rules.sql

mk-st-regexp: mk-st-regexp.pl
	$(PERL) -c mk-st-regexp.pl
	rm -f mk-st-regexp
	echo "#! " $(PERL) > mk-st-regexp
	cat mk-st-regexp.pl >> mk-st-regexp
	chmod ugo+x mk-st-regexp

mk-city-regex: mk-city-regex.pl usps-st-city-name.txt
	$(PERL) -c mk-city-regex.pl
	rm -f mk-city-regex
	echo "#! " $(PERL) > mk-city-regex
	cat mk-city-regex.pl >> mk-city-regex
	chmod ugo+x mk-city-regex

usps-st-city-name.txt: usps-st-city-orig.txt usps-st-city-adds.txt
	cat usps-st-city-orig.txt usps-st-city-adds.txt | sort -u >usps-st-city-name.txt

parseaddress-stcities.h: mk-city-regex
	./mk-city-regex > parseaddress-stcities.h

parseaddress-regex.h: mk-st-regexp
	./mk-st-regexp > parseaddress-regex.h

dist-clean:
	rm -f mk-st-regexp mk-city-regex usps-st-city-name.txt parseaddress-stcities.h parseaddress-regex.h test_main

test:
	@echo "To run the test on parse_address do the follow:"
	@echo "1. (make && sudo make install) to compile and install extension"
	@echo "2. create a database and install the address_standardizer extension"
	@echo "3. psql test_db -f test-parseaddress.sql"
	@echo "it should report '(0 rows)' if all tests passed or"
	@echo "report which ones failed."

test_main: $(OBJS_test_main)
	gcc -o test_main $(OBJS_test_main) $(LDFLAGS) $(LIBS)

test_main.o: test_main.c pagc_api.h pagc_std_api.h

address_parser.o: address_parser.c parseaddress-api.h

address_standardizer.o: address_standardizer.c std_pg_hash.h pagc_api.h pagc_std_api.h

analyze.o: analyze.c pagc_api.h

err_param.o: err_param.c pagc_api.h

export.o: export.c pagc_api.h pagc_tools.h

gamma.o: gamma.c pagc_api.h pagc_std_api.h gamma.h

hash.o: hash.c hash.h khash.h

lexicon.o: lexicon.c pagc_api.h pagc_std_api.h

pagc_tools.o: pagc_tools.c pagc_tools.h pagc_common.h

parseaddress-api.o: parseaddress-api.c parseaddress-api.h parseaddress-stcities.h parseaddress-regex.h

standard.o: standard.c pagc_api.h

tokenize.o: tokenize.c pagc_api.h

std_pg_hash.o: std_pg_hash.c std_pg_hash.h pagc_api.h pagc_std_api.h



