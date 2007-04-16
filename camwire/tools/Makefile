# Makefile for Camwire tools.
# $Id: Makefile,v 7.0 2005/02/18 02:14:59 johanns Exp $

# Camwire source and build base directory:
BASE = ..
include $(BASE)/Makefile.common

SOURCES = $(shell ls *.c *.h 2>/dev/null | tr "\n" " ")

TARGETS = unlisten_1394 resetbus_1394

DEPENDS = unlisten_1394.c resetbus_1394.c

INSTALLFILES = $(INSTALLDIR)/$(BIN)/unlisten_1394 \
		$(INSTALLDIR)/$(BIN)/resetbus_1394

LDLIBS += -lcamwire_1394

tools: .depend $(TARGETS)

install: tools $(INSTALLFILES) Makefile
uninstall:
	$(RM) $(INSTALLFILES)

clean:
	$(RM) *.o *~ core
clean-all: clean
	$(RM) unlisten_1394 resetbus_1394

.PHONY: tools install uninstall clean clean-all

# Dependencies:

.depend: $(SOURCES) Makefile $(BASE)/Makefile.common
	$(MAKEDEPEND) $(DEPENDS) > $@
include .depend

# Build:

unlisten_1394: unlisten_1394.o
resetbus_1394: resetbus_1394.o $(BASE)/$(SRC)/libcamwire_1394.a

# Install:

$(INSTALLDIR)/$(BIN)/unlisten_1394: unlisten_1394
	$(CP) $< $@
$(INSTALLDIR)/$(BIN)/resetbus_1394: resetbus_1394
	$(CP) $< $@
