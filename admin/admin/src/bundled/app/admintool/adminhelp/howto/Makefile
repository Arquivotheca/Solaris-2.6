#
#
#ident "@(#)Makefile 1.5	94/10/04 SMI"
#
# Makefile for admintool
#
include ../../../../../Makefile.master

ROOTHELPDIR = $(ROOT)/usr/snadm/classes/locale/C/help/admintool
HELPTYPE=howto

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

HELPFILES = Howto	\
	group_add.h.hlp	\
	group_del.h.hlp	\
	group_mod.h.hlp	\
	group_14.h.hlp	\
	user_add.h.hlp	\
	user_del.h.hlp	\
	user_mod.h.hlp	\
	host_add.h.hlp	\
	host_del.h.hlp	\
	host_mod.h.hlp	\
	print_add.h.hlp	\
	print_del.h.hlp	\
	print_mod.h.hlp	\
	software_add.h.hlp	\
	serial_mod.h.hlp

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
