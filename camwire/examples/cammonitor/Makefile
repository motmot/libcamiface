# Makefile for Camwire cammonitor example.
# $Id: Makefile,v 7.0 2005/02/18 02:10:38 johanns Exp $

# Camwire source and build base directory:
BASE = ../..
include $(BASE)/Makefile.common

MAN = man/man1

OBJECTS = cammonitor.o \
		display.o \
		display-sdl.o \
		display-xv.o \
		$(BASE)/src/libcamwire_1394.a

DEPENDS = cammonitor.c \
		display.c \
		display-sdl.c \
		display-xv.c

SOURCES = $(shell ls *.c *.h 2>/dev/null | tr "\n" " ")

INSTALLFILES = $(INSTALLDIR)/$(BIN)/cammonitor \
		$(INSTALLDIR)/$(MAN)/cammonitor.1

INCS += -I$(shell sdl-config --cflags)

LDFLAGS += -L/usr/X11R6/lib
LDLIBS += -lX11 -lXv -lXext
LDLIBS += -lcamwire_1394
LDLIBS += $(shell sdl-config --libs)

example_cammonitor: .depend cammonitor

install: example_cammonitor $(INSTALLFILES) Makefile
uninstall:
	$(RM) $(INSTALLFILES)

clean:
	$(RM) *.o *~ core
clean-all: clean
	$(RM) cammonitor

.PHONY: example_cammonitor install uninstall clean clean-all

# Dependencies:

.depend: $(SOURCES) Makefile $(BASE)/Makefile.common
	$(MAKEDEPEND) $(DEPENDS) > $@
include .depend

# Build:

cammonitor: $(OBJECTS)

#cammonitor.o: cammonitor.c display.h display-codes.h Makefile
#display.o: display.c display-sdl.h display-xv.h display.h display-codes.h \
#		Makefile
#display-sdl.o: display-sdl.c display-codes.h display-sdl.h Makefile
#display-xv.o: display-xv.c display-codes.h display-xv.h Makefile

# Install:

$(INSTALLDIR)/$(BIN)/cammonitor: cammonitor
	$(CP) $< $@
$(INSTALLDIR)/$(MAN)/cammonitor.1: cammonitor.1
	$(CP) $< $@
