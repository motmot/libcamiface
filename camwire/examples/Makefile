# Makefile for Camwire examples
# $Id: Makefile,v 7.0 2005/02/18 02:08:59 johanns Exp $

# Camwire source and build base directory:
BASE = ..
include $(BASE)/Makefile.common

examples: camlatency cammonitor

camlatency:
	$(MAKE) -C camlatency
cammonitor:
	$(MAKE) -C cammonitor

install:
	$(MAKE) $@ -C camlatency && \
	$(MAKE) $@ -C cammonitor
uninstall:
	$(MAKE) $@ -C camlatency && \
	$(MAKE) $@ -C cammonitor
clean:
	$(MAKE) $@ -C camlatency && \
	$(MAKE) $@ -C cammonitor
clean-all:
	$(MAKE) $@ -C camlatency && \
	$(MAKE) $@ -C cammonitor

.PHONY: examples camlatency cammonitor install uninstall clean clean-all
