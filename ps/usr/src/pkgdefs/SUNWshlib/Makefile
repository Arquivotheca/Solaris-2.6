#
#ident	"@(#)Makefile	1.1	95/01/20 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
#

include ../Makefile.com

DATAFILES +=
SHLIB_FILES = libNSL_s libX11_s libc_s libnsl_s

SHLIB_PROTO = /shlib
SHLIB_DESTDIR = $(ROOT)/shlib

INST_SHLIB = $(SHLIB_FILES:%=$(SHLIB_DESTDIR)/%)

.KEEP_STATE:

all: $(FILES) $(SHLIB_DESTDIR) $(INST_SHLIB)

install: all pkg

$(SHLIB_DESTDIR):
	mkdir -p $@

$(SHLIB_DESTDIR)/%:  $(SHLIB_PROTO)/%
	$(INS.file)

include ../Makefile.targ
