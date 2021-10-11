#
# Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
#
# "@(#)Makefile 1.10 96/07/17 SMI"
#
# SPMI service library makefile
#

LIBRARY	= libspmisvc.a
VERS	= .1

OBJECTS	= \
	svc_checkpoint.o \
	svc_dfltmnt.o \
	svc_do_upgrade.o \
	svc_dsr_archive_list.o \
	svc_fdiskconfig.o \
	svc_fs_space.o \
	svc_global.o \
	svc_mountall.o \
	svc_resource.o \
	svc_resourceobj.o \
	svc_sdiskconfig.o \
	svc_sdiskfreespace.o \
	svc_service_free.o \
	svc_services.o \
	svc_setaction.o \
	svc_slice_list.o \
	svc_sp_free_results.o \
	svc_sp_print_results.o \
	svc_templates.o \
	svc_updateconfig.o \
	svc_updatedisk.o \
	svc_updateserial.o \
	svc_updatesoft.o \
	svc_updatesys.o \
	svc_upg_recover.o \
	svc_upgradeable.o \
	svc_vfstab.o \
	svc_write_script.o

SRCS =	$(OBJECTS:.o=.c)

PRIVHDRS	= spmisvc_lib.h \
		  svc_dsr_archive_list_in.h \
		  svc_strings.h \
		  svc_templates.h
EXPHDRS		= spmisvc_api.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

INCLUDE		 = -I../libspmisoft \
		   -I../libspmistore \
		   -I../libspmicommon
CPPFLAGS	+= ${INCLUDE} -D${ARCH}
CFLAGS		+= $(DEBUG_CFLAGS) -Xa ${CPPFLAGS}
LDFLAGS		+=

LINTERR		= lint_errors
LINTFILES	= ${SRCS:%.c=${ARCH}/%.ln}
LINTFLAGS	= -umx ${CPPFLAGS}

CLOBBERFILES	= *.po *.mo

MSG_DOMAIN	= SUNW_INSTALL_LIBSVC

.KEEP_STATE:

all: $(HDRS) .WAIT static dynamic
	@true

static: $(LIBS)

dynamic: $(DYNLIB) .WAIT $(DYNLIBLINK)

install:	all .WAIT \
		$(ROOTADMINLIB) $(ROOTADMINLIBS) $(ROOTADMINLIBDYNLIB) \
		$(ROOTADMINLIBDYNLIBLINK) \
		$(ROOTOPTADMINLIBDYNLIB) $(ROOTOPTADMINLIBDYNLIBLINK) \
		msgs $(INSTMSGS)

install_test:	all .WAIT \
		$(ROOTADMINLIB) $(ROOTADMINLIBS) $(ROOTADMINLIBDYNLIB) \
		$(ROOTADMINLIBDYNLIBLINK) $(INSTMSGS)

install_h:	$(ROOTUSRINCLEXP)

driver: objs/$(ARCH)/driver.o objs/$(ARCH)/$(LIBRARY)
		$(CC) -o objs/$(ARCH)/$@ objs/$(ARCH)/$@.o\
                        -u print_disk \
                        objs/$(ARCH)/$(LIBRARY) \
                        -Bstatic -lspmistore -lspmisoft -lspmicommon \
                        -Bdynamic -ldl -ldevinfo -lnsl -ladm -lintl -lgen

svc_upgradeable: objs/$(ARCH)/$(LIBRARY)
		$(CC) -o objs/$(ARCH)/$@ -DMODULE_TEST $@.c $(INCLUDE) \
                        -u print_disk \
                        objs/$(ARCH)/svc_upgradeable.o \
                        objs/$(ARCH)/$(LIBRARY) \
                        -Bstatic -lspmistore -lspmisoft -lspmicommon \
                        -Bdynamic -ldl -ldevinfo -lnsl -ladm -lintl -lgen

msgs: ${MSG_DOMAIN}.po

${MSG_DOMAIN}.po: ${SRCS} ${HDRS}
	@egrep '#define.*gettext' $(SRCS) *.h | sed 's/#//' > tmp.c
	@sed 's/\\\\n/\\n/g' svc_templates.c > svc_templatesTMP.c
	@$(ROOTADMIN)/bin/xgetsh -d $(MSG_DOMAIN) svc_templatesTMP.c
	@-sed '/^domain /d' $(MSG_DOMAIN).po > templates.po
	@xgettext -s tmp.c $?
	@rm -f tmp.c
	@-rm messages.po 2> /dev/null
	@-cat templates.po >> $(MSG_DOMAIN).po
	@-rm templates.po svc_templatesTMP.c
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
 
# Create a message file with "translated" test strings.
# Turn the po file into an mo file ready for testing.
msgs_test: ${SRCS} ${HDRS}
	@echo "Making test messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} -s \
			-c "i18n:" -m "öýê!" - >/dev/null 2>&1
	@msgfmt -o ${MSG_DOMAIN}.mo ${MSG_DOMAIN}.po

lint:  ${SRCS} ${HDRS}
	${LINT.c} ${SRCS}

cstyle:	$(SRCS) $(PRIVHDRS) $(PUBHDRS)
	cstyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)

include ../Makefile.targ
