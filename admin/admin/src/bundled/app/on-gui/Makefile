#
#ident	"@(#)Makefile 1.3 94/10/06 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# on-gui/Makefile
#
# include global definitions
include ../../../Makefile.master

SUBDIRS= lpalerts

all:=		TARGET= all
install:=	TARGET= install
clean:=		TARGET= clean
clobber:=	TARGET= clobber
lint:=		TARGET= lint

.KEEP_STATE:

all install clean clobber lint: $(SUBDIRS)

install_h:

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

FRC:
