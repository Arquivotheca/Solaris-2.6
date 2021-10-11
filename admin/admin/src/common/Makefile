#
#ident	"@(#)Makefile	1.5	94/11/03 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#

SUBDIRS=	cmd lib .WAIT app

$(CH)PACKAGE=	pkg

ROOTINC=	$(ROOT)/usr/include
ROOTINCADMIN=	$(ROOTINC)/admin

all:=		TARGET= all
all:=		PKTARG= all
install:=	TARGET= install
install:=	PKTARG= install
install_h:=	TARGET= install_h
clean:=		TARGET= clean
clobber:=	TARGET= clobber
lint:=		TARGET= lint

.KEEP_STATE:

all install clean clobber: $(SUBDIRS) $(PACKAGE)

headers: cmds install_h

cmds:
	@cd cmd; pwd; $(MAKE) install

install_h:	$(ROOTINCADMIN) $(SUBDIRS)

$(ROOTINCADMIN):
	$(INS.dir)

$(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

$(PACKAGE): FRC
	@cd $@; pwd; $(MAKE) $(PKTARG)

FRC:
