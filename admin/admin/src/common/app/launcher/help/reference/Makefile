#
#
#ident "@(#)Makefile 1.6	94/11/16 SMI"
#
# Makefile for admintool
#
include ../../../../../Makefile.master

ROOTHELPDIR = $(ROOT)/usr/snadm/classes/locale/C/help/launcher
HELPTYPE=reference

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

HELPFILES = Reference	\
	glossary.r.hlp

ROOTHDIR=${ROOTHELPDIR}/${HELPTYPE}
ROOTHELPFILES=$(HELPFILES:%=${ROOTHDIR}/%)

$(ROOTHELPFILES) :=	FILEMODE = 0644
$(ROOTHELPFILES) :=	OWNER = root
$(ROOTHELPFILES) :=	GROUP = sys

all clean clobber:
	echo "Nothing to do here"

#$(ROOTHDIR):
#	$(INS.dir)

$(ROOTHELPDIR)/$(HELPTYPE)/%: %
	$(INS.file)

install: ${ROOTHDIR} ${ROOTHELPFILES}
