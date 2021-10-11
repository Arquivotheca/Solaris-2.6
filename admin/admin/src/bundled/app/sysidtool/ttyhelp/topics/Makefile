#
#
#ident "@(#)Makefile 1.7	94/10/18 SMI"
#
# Makefile for sysidtool
#
include ../../../../../Makefile.master

ROOTHELPDIR = $(ROOT)/usr/snadm/classes/locale/C/help/sysidtty.help
HELPTYPE = topics

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

HELPFILES = Topics	\
	confirm.help	\
	date.help	\
	domain.help	\
	hostname.help	\
	ipaddr.help	\
	location.help	\
	name_service.help	\
	navigate.help	\
	netif.help	\
	netmask.help	\
	nsserver.help	\
	standalone.help	\
	subnet.help	\
	sysid.help	\
	timezone.help

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
