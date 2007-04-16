# Makefile for Camwire camera configuration files
# $Id: Makefile,v 7.0 2005/02/18 02:08:18 johanns Exp $

# Camwire source and build base directory:
BASE = ../..
include $(BASE)/Makefile.common

INSTALLFILES = $(INSTALLDIR)/$(ETC)/A301fc.conf \
		$(INSTALLDIR)/$(ETC)/A602fc.conf \
		$(INSTALLDIR)/$(ETC)/A602f.conf \
		$(INSTALLDIR)/$(ETC)/API-200.conf \
		$(INSTALLDIR)/$(ETC)/README_cameraIds \
		$(INSTALLDIR)/$(ETC)/README_pixcodings

etc:
	@echo "Usage: make [install | uninstall]" && false

install: $(INSTALLFILES) Makefile
uninstall:
	$(RM) $(INSTALLFILES)

.PHONY: etc install uninstall

# Install:
$(INSTALLDIR)/$(ETC)/A301fc.conf: A301fc.conf
	$(CP) $< $@
$(INSTALLDIR)/$(ETC)/A602fc.conf: A602fc.conf
	$(CP) $< $@
$(INSTALLDIR)/$(ETC)/A602f.conf: A602f.conf
	$(CP) $< $@
$(INSTALLDIR)/$(ETC)/API-200.conf: API-200.conf
	$(CP) $< $@
$(INSTALLDIR)/$(ETC)/README_cameraIds: README_cameraIds
	$(CP) $< $@
$(INSTALLDIR)/$(ETC)/README_pixcodings: README_pixcodings
	$(CP) $< $@
