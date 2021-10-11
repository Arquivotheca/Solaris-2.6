#
#
#ident "@(#)Makefile 1.4	94/01/07 SMI"
#
# Makefile for admintool
#
include ../../../../../Makefile.master

ROOTHELPDIR = $(ROOT)/usr/snadm/classes/locale/C/help/admintool
HELPTYPE = topics

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

HELPFILES = Topics	\
	athelp.t.hlp	\
	group.t.hlp	\
	group_unix.t.hlp	\
	group_14.t.hlp	\
	user.t.hlp	\
	user_account.t.hlp	\
	user_passwd_setting.t.hlp	\
	user_passwd_opts.t.hlp	\
	host.t.hlp	\
	printer.t.hlp	\
	serial.t.hlp	\
	software.t.hlp	\
	media.t.hlp	\
	admintool.t.hlp	\
	host_IP.t.hlp	\
	pkg_admin.t.hlp

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
