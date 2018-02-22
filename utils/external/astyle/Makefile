# **********************************************************************
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.net
# *
# * Copyright (C) 2018  Sandro Santilli <strk@kbt.io>
# *
# * This is free software; you can redistribute and/or modify it under
# * the terms of the GNU General Public Licence. See the COPYING file.
# *
# **********************************************************************

INCLUDES = ASLocalizer.h astyle.h astyle_main.h

SRC= \
  astyle_main.cpp \
  ASBeautifier.cpp \
  ASEnhancer.cpp \
  ASFormatter.cpp \
  ASLocalizer.cpp \
  ASResource.cpp

%.o: %.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) -c $< -o $@

OBJ = $(patsubst %.cpp,%.o,$(SRC))

all: astyle

astyle: $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) astyle
