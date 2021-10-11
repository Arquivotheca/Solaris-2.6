#
# uts/i86pc/sys/Makefile
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
# All rights reserved.
#
#ident "@(#)Makefile	1.3	96/06/26 SMI"
#
# include global definitions
include ../../../Makefile.master

HDRS= \
elx.h		kd.h		mouse.h

ROOTDIR=	$(ROOT)/usr/include/sys
ROOTDIRS=	$(ROOTDIR)

ROOTHDRS=       $(HDRS:%=$(ROOTDIR)/%)

# install rules
$(ROOTDIR)/%: %
	$(INS.file)

.KEEP_STATE:

install_h: $(ROOTDIRS) $(ROOTHDRS)

$(ROOTDIRS):
	$(INS.dir)
