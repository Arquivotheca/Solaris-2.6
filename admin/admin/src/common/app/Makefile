#
#ident	"@(#)Makefile	1.10	96/10/11 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
# All rights reserved.
#

SUBDIRS=	system sadmind apm launcher launcher-new showrev get_netmask

all:=		TARGET= all
install:=	TARGET= install
install_h:=	TARGET= install_h
clean:=		TARGET= clean
clobber:=	TARGET= clobber
lint:=		TARGET= lint

.KEEP_STATE:

all install clean clobber install_h: $(SUBDIRS)

headers: install_h

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

FRC:
