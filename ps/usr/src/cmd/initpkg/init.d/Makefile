#
# Copyright (c) 1989-1994 by Sun Microsystems, Inc.
# All rights reserved.
#
#ident @(#)Makefile   1.1     96/05/17
#

include $(SRC)/Makefile.master

i386_PROG=	initboot
sparc_PROG=
ppc_PROG=

PROG= $($(MACH)_PROG)

ROOTETC=        $(ROOT)/etc
ROOTETCINITD=		$(ROOTETC)/init.d

DIRS=	$(ROOTETC) $(ROOTETCINITD)

ROOTETCINITDPROG=	$(PROG:%=$(ROOTETCINITD)/%)

DIRMODE = 0755
FILEMODE = 0744
OWNER = root
GROUP = sys

.KEEP_STATE:

all:		$(DIRS) $(PROG)

$(DIRS):
		$(INS.dir)

$(ROOTETCINITD)/%:	%
		$(INS.file)

install:	all $(ROOTETCINITDPROG)

lint:

clean:

clobber:
