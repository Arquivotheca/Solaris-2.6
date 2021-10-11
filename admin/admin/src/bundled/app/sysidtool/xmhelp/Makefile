#
#
#ident "@(#)Makefile 1.1	93/10/12 SMI"
#
# Makefile for sysidtool
#
include ../../../../Makefile.master

SUBDIRS=	topics reference howto

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

all install clean clobber: ${SUBDIRS}

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)
