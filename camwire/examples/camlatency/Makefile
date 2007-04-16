# Makefile for Camwire camlatency example.
# $Id: Makefile,v 7.0 2005/02/18 02:09:14 johanns Exp $

# Camwire source and build base directory:
BASE = ../..
include $(BASE)/Makefile.common

DEPENDS = camlatency.c
SOURCES = $(shell ls *.c *.h 2>/dev/null | tr "\n" " ")

INSTALLFILES = $(INSTALLDIR)/$(BIN)/camlatency

INCS += -I$(shell sdl-config --cflags)

LDFLAGS += -L/usr/X11R6/lib
LDLIBS += -lX11 -lXv -lXext
LDLIBS += -lcamwire_1394
LDLIBS += $(shell sdl-config --libs)

example_camlatency: .depend camlatency

install: example_camlatency $(INSTALLFILES) Makefile
uninstall:
	$(RM) $(INSTALLFILES)

clean:
	$(RM) *.o *~ core
clean-all: clean
	$(RM) camlatency

.PHONY: example_camlatency install uninstall clean clean-all

# Dependencies:

.depend: $(SOURCES) Makefile $(BASE)/Makefile.common
	$(MAKEDEPEND) $(DEPENDS) > $@
include .depend

# Build:

camlatency: camlatency.o

# Install:

$(INSTALLDIR)/$(BIN)/camlatency: camlatency
	$(CP) $< $@
