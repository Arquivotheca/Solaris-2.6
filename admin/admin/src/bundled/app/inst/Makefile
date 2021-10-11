#
#ident	"@(#)Makefile 1.38 96/02/05 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# install application Makefile

include $(SRC)/Makefile.master

sparc_SUBDIRS =

i386_SUBDIRS = rootdev

ppc_SUBDIRS = rootdev

COMMON_SUBDIRS	= pf get_root pfc buildtoc buildhist \
		  mntpnt pfg dial findcd bpgetfile mem bootargs

SUBDIRS		= $(COMMON_SUBDIRS) $($(ARCH)_SUBDIRS)

all:=		TARGET = all
clean:=		TARGET = clean
clobber:=	TARGET = clobber
cstyle:=	TARGET = cstyle
install:=	TARGET = install
lint:=		TARGET = lint
lintlib:=	TARGET = lintlib

.KEEP_STATE:

all clean cstyle install lint lintlib clobber: ${SUBDIRS}

${SUBDIRS}: FRC
	cd $@; pwd; $(MAKE) $(TARGET)

.WAIT:
