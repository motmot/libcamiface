# Top-level Makefile: recursively makes all the others.
# $Id: Makefile,v 7.0 2005/02/18 02:04:10 johanns Exp $

BASE = .
include $(BASE)/Makefile.common

THISDISTRIB = camwire-0.7.0
DISTRIBTARGET = $(THISDISTRIB).tar.gz

all: library examples
library:
	$(MAKE) -C $(SRC)
tools:
	$(MAKE) -C $(TOOLS)
examples:
	$(MAKE) -C $(EXAMPLES)
distrib: $(DISTRIBTARGET)

install:
	$(MAKE) $@ -C $(SRC) && \
	$(MAKE) $@ -C $(TOOLS) && \
	$(MAKE) $@ -C $(EXAMPLES) && \
	$(MAKE) $@ -C $(ETC)
uninstall:
	$(MAKE) $@ -C $(SRC) && \
	$(MAKE) $@ -C $(TOOLS) && \
	$(MAKE) $@ -C $(EXAMPLES) && \
	$(MAKE) $@ -C $(ETC)

clean:
	$(MAKE) $@ -C $(SRC) && \
	$(MAKE) $@ -C $(TOOLS) && \
	$(MAKE) $@ -C $(EXAMPLES)
clean-all:
	$(MAKE) $@ -C $(SRC) && \
	$(MAKE) $@ -C $(TOOLS) && \
	$(MAKE) $@ -C $(EXAMPLES)

.PHONY: all library tools examples install uninstall clean clean-all

# Distribution:

DISTRIBDIR = $(THISDISTRIB)
DISTRIBFILES =	$(DISTRIBDIR)/README* \
		$(DISTRIBDIR)/CHANGELOG \
		$(DISTRIBDIR)/COPYING \
		$(DISTRIBDIR)/INSTALL \
		$(DISTRIBDIR)/Makefile \
		$(DISTRIBDIR)/Makefile.common \
		$(DISTRIBDIR)/src/Makefile \
		$(DISTRIBDIR)/src/*.h \
		$(DISTRIBDIR)/src/*.c \
		$(DISTRIBDIR)/src/camwire/*.h \
		$(DISTRIBDIR)/tools/Makefile \
		$(DISTRIBDIR)/tools/*.c \
		$(DISTRIBDIR)/examples/Makefile \
		$(DISTRIBDIR)/examples/cammonitor/Makefile \
		$(DISTRIBDIR)/examples/cammonitor/*.1 \
		$(DISTRIBDIR)/examples/cammonitor/*.h \
		$(DISTRIBDIR)/examples/cammonitor/*.c \
		$(DISTRIBDIR)/examples/camlatency/Makefile \
		$(DISTRIBDIR)/examples/camlatency/*.c \
		$(DISTRIBDIR)/etc/camwire/Makefile \
		$(DISTRIBDIR)/etc/camwire/*.conf \
		$(DISTRIBDIR)/etc/camwire/README*

$(DISTRIBTARGET): $(DISTRIBDIR)
	-tar -cvzf $(DISTRIBTARGET) $(DISTRIBFILES)
	rm -fr $(DISTRIBDIR)
$(DISTRIBDIR):
	-cp -pr . $(DISTRIBDIR) 2>/dev/null
