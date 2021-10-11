#
#ident  "@(#)Makefile	1.16	96/04/02 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#

SUBDIRS=	libadmutil \
		libadmldb \
		libjobsched \
		libprinter \
		libserial \
		libadmcom \
		libadmapm \
		libadmagt \
		libadmsec \
		liblauncher \
		liblauncher-new \
		libadmapp \
		libsysman \
		libspmicommon \
		libspmisoft \
		libspmistore \
		libspmisvc \
		libspmitty \
		libspmixm \
		libspmiapp

ROOTINC=	$(ROOT)/usr/include
ROOTINCADMIN=	$(ROOTINC)/admin

all:=		TARGET= all
install:=	TARGET= install
install_h:=	TARGET= install_h
clean:=		TARGET= clean
clobber:=	TARGET= clobber
lint:=		TARGET= lint

.KEEP_STATE:

all install clean clobber: $(SUBDIRS)

headers: install_h

install_h:	$(ROOTINCADMIN) $(SUBDIRS)

$(ROOTINCADMIN):
	$(INS.dir)

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

FRC:
