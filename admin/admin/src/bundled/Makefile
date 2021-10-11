#
#ident	"@(#)Makefile 1.3 94/12/02 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# Makefile for SNAG application source
#
# include global definitions
include ../Makefile.master
#

SUBDIRS=	app

$(CH)PACKAGE=	pkg

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

all install patch:	headers $(SUBDIRS) $(PACKAGE)

newpatch:		initpatch patch

clean clobber: $(SUBDIRS)

headers:

initpatch:
	@cd $(PACKAGE); pwd; $(MAKE) initpatch

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

$(PACKAGE): FRC
	@cd $@; pwd; $(MAKE) $(PKTARG)

FRC:
