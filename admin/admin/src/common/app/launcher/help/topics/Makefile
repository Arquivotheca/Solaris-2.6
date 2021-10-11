#
#
#ident "@(#)Makefile 1.6	94/11/16 SMI"
#
# Makefile for admintool
#
include ../../../../../Makefile.master

ROOTHELPDIR = $(ROOT)/usr/snadm/classes/locale/C/help/launcher
HELPTYPE = topics

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

HELPFILES = Topics	\
	athelp.t.hlp	\
	atover.t.hlp	\
	database.t.hlp	\
	group.t.hlp	\
	host.t.hlp	\
	nameserv.t.hlp	\
	policy.t.hlp	\
	printer.t.hlp	\
	serial.t.hlp	\
	user.t.hlp	\
	windows.t.hlp


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
