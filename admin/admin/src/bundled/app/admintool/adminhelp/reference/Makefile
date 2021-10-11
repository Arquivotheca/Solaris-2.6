#
#
#ident "@(#)Makefile 1.5	94/10/04 SMI"
#
# Makefile for admintool
#
include ../../../../../Makefile.master

ROOTHELPDIR = $(ROOT)/usr/snadm/classes/locale/C/help/admintool
HELPTYPE=reference

all:=		TARGET=	all
install:=	TARGET=	install
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber

HELPFILES = Reference	\
	user_window.r.hlp	\
	window_glossary.r.hlp	\
	group_window.r.hlp	\
	hosts_window.r.hlp	\
	printer_window.r.hlp	\
	printer_remote_window.r.hlp	\
	serial_window.r.hlp	\
	software_window.r.hlp	\
	software_custom_window.r.hlp	\
	key_shortcuts.r.hlp	\
	glossary.r.hlp	\
	existing_files.r.hlp	\
	existing_partial_install.r.hlp	\
	existing_pkgs.r.hlp	\
	incorrect_run_level.r.hlp	\
	install_depend_unmet.r.hlp	\
	install_setuid_files.r.hlp	\
	insufficient_space.r.hlp	\
	interactive.r.hlp	\
	mail_receipt.r.hlp	\
	removal_depend_unmet.r.hlp	\
	run_setuid_scripts.r.hlp	\
	show_copyrights.r.hlp

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
