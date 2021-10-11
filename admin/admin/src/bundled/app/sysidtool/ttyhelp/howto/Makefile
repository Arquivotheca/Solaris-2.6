#
#
#ident "@(#)Makefile 1.7	94/10/06 SMI"
#
# Makefile for sysidtool
#
include ../../../../../Makefile.master

ROOTHELPDIR = $(ROOT)/usr/snadm/classes/locale/C/help/sysidtty.help
HELPTYPE=howto

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

HELPFILES = Howto	\
	navigate.help \
	sysid.help

ROOTHDIR=${ROOTHELPDIR}/${HELPTYPE}
ROOTHELPFILES=$(HELPFILES:%=${ROOTHDIR}/%)

$(ROOTHELPFILES) :=	FILEMODE = 0644
$(ROOTHELPFILES) :=	OWNER = root
$(ROOTHELPFILES) :=	GROUP = sys

all clean clobber:
	@echo "Nothing to do here"

#$(ROOTHDIR):
#	$(INS.dir)

$(ROOTHELPDIR)/$(HELPTYPE)/%: %
	$(INS.file)

install: ${ROOTHDIR} ${ROOTHELPFILES}
