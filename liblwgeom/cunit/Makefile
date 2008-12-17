
LWGEOM=..
LIBCUNIT=/usr/local/lib/libcunit.a
LIBLWGEOM=$(LWGEOM)/liblwgeom.a

OBJS =	cu_tester.o \
	cu_algorithm.o

#---------------------------------------------------------------

INCLUDES = -I/usr/local/include -I$(LWGEOM)
CFLAGS = -g $(MSDEFINES) $(INCLUDES)
LIBS = $(LIBCUNIT) $(LIBLWGEOM)
LDFLAGS = 

cu_tester: $(OBJS) $(LIBS)

test: cu_tester
	./cu_tester

check: test

clean:
	@rm $(OBJS)
	@rm cu_tester
