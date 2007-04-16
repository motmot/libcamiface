# Makefile for Camwire library archive.
# $Id: Makefile,v 7.0 2005/02/18 02:11:53 johanns Exp $

# Camwire source and build base directory:
BASE = ..
include $(BASE)/Makefile.common

LDLIBS += -lcamwire_1394

OBJECTS = camwirebus_1394.o camwire_1394.o

DEPENDS = camwirebus_1394.c \
		camwire_1394.c \
		camwirebus-test.c

SOURCES = $(shell ls *.c *.h camwire/*.h 2>/dev/null | \
		tr "\n" " ")

INSTALLFILES = $(INSTALLDIR)/$(INC)/camwirebus.h \
		$(INSTALLDIR)/$(INC)/camwire.h \
		$(INSTALLDIR)/$(LIB)/libcamwire_1394.a

library: .depend libcamwire_1394.a camwirebus-test

test: camwirebus-test-OK
testrun:
	$(RM) camwirebus-test-OK && $(MAKE) test

install: library $(INSTALLFILES) Makefile
uninstall:
	$(RM) $(INSTALLFILES)

clean:
	$(RM) *.o *~ core
clean-all: clean
	$(RM) libcamwire_1394.a
	$(RM) camwirebus-test camwirebus-test-OK

.PHONY: library test install uninstall clean clean-all

# Dependencies:

.depend: $(SOURCES) Makefile $(BASE)/Makefile.common
	$(MAKEDEPEND) $(DEPENDS) > $@
include .depend

# Build:

libcamwire_1394.a: $(OBJECTS)

# Test:

camwirebus-test: camwirebus-test.o camwirebus_1394.o
camwirebus-test-OK: camwirebus-test
	./camwirebus-test && touch camwirebus-test-OK

#camwirebus-test.o: camwirebus-test.c camwire/camwirebus.h \
#		camwirebus_internal_1394.h Makefile

# Install:

$(INSTALLDIR)/$(INC)/camwirebus.h: camwire/camwirebus.h
	$(CP) $< $@
$(INSTALLDIR)/$(INC)/camwire.h: camwire/camwire.h
	$(CP) $< $@
$(INSTALLDIR)/$(LIB)/libcamwire_1394.a: libcamwire_1394.a
	$(CP) $< $@
