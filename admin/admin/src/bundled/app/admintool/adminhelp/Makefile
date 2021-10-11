#
#
#ident "@(#)Makefile	1.1 94/11/23 Sun Microsystems"
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
