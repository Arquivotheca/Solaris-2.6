#
# uts/i86/sys/Makefile
#
#pragma ident  "@(#)Makefile 1.1     95/05/22 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# include global definitions
include ../../../Makefile.master

HDRS=

SUBHDRS=  \

ROOTDIR=	$(ROOT)/usr/include/sys
ROOTDIRS=       $(ROOTDIR) $(ROOTDIR)/dktp $(ROOTDIR)/ws
ROOTHDRS=       $(HDRS:%=$(ROOTDIR)/%) $(SUBHDRS:%=$(ROOTDIR)/%)

# install rules
$(ROOTDIR)/%: %
	$(INS.file)

.KEEP_STATE:

install_h: $(ROOTDIRS) $(ROOTHDRS)

$(ROOTDIRS):
	$(INS.dir)
