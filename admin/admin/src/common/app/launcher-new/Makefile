#
#ident  "@(#)Makefile	1.5	95/01/17 SMI"
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved
#
# src/bundled/app/launcher
#

include $(SRC)/Makefile.master

sparc_SUBDIRS =

i386_SUBDIRS =

COMMON_SUBDIRS = soladdapp soldelapp solstice

SUBDIRS=	$(COMMON_SUBDIRS) $($(ARCH)_SUBDIRS)

all:=		TARGET = all
clean:=		TARGET = clean
clobber:=	TARGET = clobber
cstyle:=	TARGET = cstyle
install:=	TARGET = install
install_h:=	TARGET = install_h
lint:=		TARGET = lint
lintlib:=	TARGET = lintlib

.KEEP_STATE:

all clean cstyle install install_h lint lintlib clobber: ${SUBDIRS}

${SUBDIRS}: FRC
	cd $@; pwd; $(MAKE) $(TARGET)

.WAIT:
