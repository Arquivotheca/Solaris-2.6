#
# Real Makefile SID -- change $-signs to %-signs in the line below
# to use this as a real Makefile, delete the space between '#' and
# ident, and put a space between '#' and ident for the template SID.
#
#ident @(#)Makefile	1.71	95/02/24
#
# ident  Derived from tempate: @(#)Makefile.library	1.2	92/09/21
#

LIBRARY		= libsw.a
VERS		= .2

OBJECTS 	= \
		api_free.o \
		api_services.o \
		arch.o \
		admin.o	\
		client.o \
        	depend.o \
		do_upgrade.o \
		dump.o \
		find_modified.o \
		free.o \
        	hash.o \
		install.o \
		locale.o \
		locale_lookup.o \
		media.o \
        	module.o \
        	mount.o \
		mountall.o \
		pkgexec.o \
		pkghist.o \
		platform.o \
		proc.o \
		prod.o \
		service.o \
		sp_space.o \
		sp_spacetab.o \
		sp_calc.o \
		sp_load.o \
		sp_mmap.o \
		sp_print_results.o \
		sp_free_results.o \
		sp_util.o \
		swi_admin.o \
		swi_arch.o \
		swi_client.o \
		swi_depend.o \
		swi_do_upgrade.o \
		swi_dump.o \
		swi_install.o \
		swi_locale.o \
		swi_media.o \
		swi_module.o \
		swi_mount.o \
		swi_mountall.o \
		swi_pkgexec.o \
		swi_platform.o \
		swi_prod.o \
		swi_sp_calc.o \
		swi_sp_free_results.o \
		swi_sp_print_results.o \
		swi_sp_space.o \
		swi_sp_util.o \
		swi_update_actions.o \
		swi_upg_recover.o \
		swi_util.o \
		swi_v_version.o \
		templates.o \
		update_actions.o \
		upg_recover.o \
		util.o \
		view.o  \
		v_version.o \
		walktree.o \
		write_script.o

SRCS =	$(OBJECTS:.o=.c)

PRIVHDRS	= sw_lib.h templates.h
EXPHDRS		= hash.h sw_api.h swmgmt_api.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

API_DOC		= swlib_api
DITROFF		= /usr/rmtc/doctools/bin/ditroff
TPSCRIPT	= /usr/rmtc/doctools/bin/tpscript

CLEANFILES	= $(API_DOC).ps

include ../Makefile.lib


INCLUDE		= -I. -I../libinst
#
# ----- WORKAROUND for bugid  1182070  -------
# The following conditional assigment can be removed when
# this bugid is resolved in the CBE.
pics/i386/sp_spacetab.o   := CFLAGS    = -Xa -DSVR4 ${INCLUDE} -K PIC
#
CFLAGS		+= $(DEBUG_CFLAGS) -Xa -DSVR4 ${INCLUDE}
LDFLAGS		+= ${CFLAGS} ${LIBDIRS}

LINTFLAGS	= ${INCLUDE} -u -m -x

MSG_DOMAIN=SUNW_INSTALL_SWLIB

.KEEP_STATE:

all: static dynamic .WAIT

static: $(LIBS)

dynamic: $(DYNLIB) .WAIT $(DYNLIBLINK)

$(OBJECTS): $(HDRS)

install:	all .WAIT \
		$(ROOTADMINLIBDIR) $(ROOTADMINLIBS) $(ROOTADMINLIBDYNLIB) \
		$(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK) \
		msgs $(INSTMSGS)

install_h:	$(ROOTUSRINCLEXP)

msgs: $(SRCS) $(HDRS)
	@egrep '#define.*gettext' $(SRCS) *.h | sed 's/#//' > tmp.c
	@sed 's/\\\\n/\\n/g' templates.c > tmptemplates.c
	@$(ROOTADMIN)/bin/xgetsh -d $(MSG_DOMAIN) tmptemplates.c
	@-sed '/^domain /d' $(MSG_DOMAIN).po > templates.po
	@xgettext -s tmp.c $?
	@rm -f tmp.c
	@-rm messages.po 2> /dev/null
	@-cat templates.po >> $(MSG_DOMAIN).po
	@-rm templates.po tmptemplates.c
	@#
	@# Add cluster names to .po file so that they are localized.
	@#
	@echo msgid \"Entire Distribution plus OEM support\" >> ${MSG_DOMAIN}.po
	@echo msgstr >> ${MSG_DOMAIN}.po
	@echo msgid \"Entire Distribution\" >> ${MSG_DOMAIN}.po
	@echo msgstr >> ${MSG_DOMAIN}.po
	@echo msgid \"Developer System Support\" >> ${MSG_DOMAIN}.po
	@echo msgstr >> ${MSG_DOMAIN}.po
	@echo msgid \"End User System Support\" >> ${MSG_DOMAIN}.po
	@echo msgstr >> ${MSG_DOMAIN}.po
	@echo msgid \"Core System Support\" >> ${MSG_DOMAIN}.po
	@echo msgstr >> ${MSG_DOMAIN}.po

lint:		$(SRCS) $(PRIVHDRS) $(EXPHDRS)
		$(LINT.c) $(SRCS)

cstyle:		$(SRCS) $(PRIVHDRS) $(EXPHDRS)
		cstyle $(SRCS) $(PRIVHDRS) $(EXPHDRS)

docs:	$(API_DOC).ps

$(API_DOC).ps:	$(API_DOC).mm swmgmt_api.h
	sed -n -e '/DOC EXTRACT START/,/DOC EXTRACT END/p' swmgmt_api.h | \
		sed -e '/DOC EXTRACT/d' > h.extract
	$(DITROFF) -Tps -mm $(API_DOC).mm > troff.out
	$(TPSCRIPT) troff.out > $(API_DOC).ps
	@rm -f troff.out h.extract

include ../Makefile.targ

.WAIT:
