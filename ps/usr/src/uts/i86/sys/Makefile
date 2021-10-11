#
# uts/i86/sys/Makefile
#
#pragma ident	"@(#)Makefile	1.28	96/06/18 SMI"
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
# include global definitions
include ../../../Makefile.master

HDRS=

# "ws" header files
WSHDRS= \
chan.h		tcl.h		ws.h

ROOTDIR=	$(ROOT)/usr/include/sys
ROOTWSDIR=	$(ROOTDIR)/ws
ROOTDIRS=       $(ROOTDIR) $(ROOTWSDIR)

ROOTHDRS= $(HDRS:%=$(ROOTDIR)/%) \
	  $(WSHDRS:%=$(ROOTWSDIR)/%)

# install rules
$(ROOTDIR)/%: %
	$(INS.file)

$(ROOTDIR)/ws/%: ws/%
	$(INS.file)

.KEEP_STATE:

install_h: $(ROOTDIRS) .WAIT $(ROOTHDRS)

$(ROOTDIRS):
	$(INS.dir)
