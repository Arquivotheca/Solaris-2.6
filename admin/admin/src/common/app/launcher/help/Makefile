#
#
#ident "@(#)Makefile 1.3	94/10/04 SMI"
#
# Makefile for admintool
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
