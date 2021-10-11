#
#ident	"@(#)Makefile 1.14 96/07/29 SMI"
#
# Copyright (c) 1992-1996 Sun Microsystems, Inc.  All Rights Reserved.

#
# Makefile for SNAG application source (app/Makefile)
#
# include global definitions
include ../../Makefile.master
#

sparc_SUBDIRS = 413

i386_SUBDIRS =

ppc_SUBDIRS =

COMMON_SUBDIRS= inst sysidtool files jumpstart_sample on-gui admintool swmt

SUBDIRS=	$(COMMON_SUBDIRS) $($(ARCH)_SUBDIRS)

all:=		TARGET=	all
all:=		PKTARG=	all
install:=	TARGET=	install
install:=	PKTARG=	install
patch:=		TARGET=	install
patch:=		PKTARG=	patch
newpatch:=	TARGET=	install
newpatch:=	PKTARG=	patch
clean:=		TARGET=	clean
clean:=		PKTARG=	clean
clobber:=	TARGET=	clobber
clobber:=	PKTARG=	clobber

.KEEP_STATE:

all install patch:	headers $(SUBDIRS)

newpatch:		initpatch patch

clean clobber: $(SUBDIRS)

headers:

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

FRC:
