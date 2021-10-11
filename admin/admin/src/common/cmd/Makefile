#
#ident	"@(#)Makefile	1.4	95/03/06 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#

SUBDIRS=	mkmsgs xgetsh genmsg

all:=		TARGET= all
install:=	TARGET= install
install_h:=	TARGET= install_h
clean:=		TARGET= clean
clobber:=	TARGET= clobber
lint:=		TARGET= lint

.KEEP_STATE:

all install clean clobber: $(SUBDIRS)

headers: install_h

install_h:

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

FRC:
