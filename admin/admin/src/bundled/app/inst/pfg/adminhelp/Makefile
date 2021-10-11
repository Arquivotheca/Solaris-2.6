#
#
#ident @(#)Makefile	1.1	94/10/10
#
#
include $(SRC)/Makefile.master

SUBDIRS=	topics reference howto

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

all install clean clobber: ${SUBDIRS}

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)
