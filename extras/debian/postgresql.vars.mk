#!/usr/bin/make -f
# -*- makefile -*-

# this makefile includes the Makefile.config from postgis, and echoes a variable value, by the
# variable name supplied in ${REQVAR}

include Makefile.config

all:
ifeq (,$(${REQVAR}))
	@echo $(shell ${PG_CONFIG_PATH}pg_config --configure | \
                awk 'BEGIN {RS = " "; FS = "=";} \
	           {if (match($$1,"--${REQVAR}$$")) \
		    { \
			gsub("'\''$$", "", $$2); \
			gsub("^/", "", $$2); \
			print $$2; \
		    } \
		   }' \
	     )
else
	@echo $(${REQVAR}) | awk '{gsub("^/", ""); print $$0;}'
endif

.PHONY: all
