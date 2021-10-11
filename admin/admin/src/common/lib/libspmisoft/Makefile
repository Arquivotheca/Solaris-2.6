#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# "@(#)Makefile 1.4 96/06/07 SMI"
#
# SPMI software library makefile
#

LIBRARY	= libspmisoft.a
VERS	= .1

OBJECTS	= \
	soft_admin.o \
	soft_arch.o \
	soft_depend.o \
	soft_do_upgrade.o \
	soft_dump.o \
	soft_find_modified.o \
	soft_free.o \
	soft_hash.o \
	soft_install.o \
	soft_locale.o \
	soft_locale_lookup.o \
	soft_media.o \
	soft_module.o \
	soft_pkghist.o \
	soft_platform.o \
	soft_prod.o \
	soft_progress.o \
	soft_service.o \
	soft_sp_calc.o \
	soft_sp_load.o \
	soft_sp_space.o \
	soft_sp_spacetab.o \
	soft_sp_util.o \
	soft_swi_admin.o \
	soft_swi_arch.o \
	soft_swi_depend.o \
	soft_swi_dump.o \
	soft_swi_install.o \
	soft_swi_locale.o \
	soft_swi_media.o \
	soft_swi_module.o \
	soft_swi_platform.o \
	soft_swi_prod.o \
	soft_swi_sp_space.o \
	soft_swi_sp_util.o \
	soft_swi_update_actions.o \
	soft_swi_util.o \
	soft_swi_v_version.o \
	soft_templates.o \
	soft_update_actions.o \
	soft_util.o \
	soft_version.o \
	soft_view.o \
	soft_walktree.o

SRCS = $(OBJECTS:.o=.c)

PRIVHDRS = \
	soft_templates.h \
	soft_version.h \
	spmisoft_lib.h \
	sw_space.h \
	sw_swi.h \
	sw_swi_defines.h
EXPHDRS = \
	spmisoft_api.h \
	soft_hash.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

INCLUDE		 = -I../libspmicommon
CPPFLAGS	+= ${INCLUDE} -D${ARCH}
CFLAGS		+= $(DEBUG_CFLAGS) -Xa ${CPPFLAGS}
LDFLAGS		+=

LINTERR		= lint_errors
LINTFILES	= ${SRCS:%.c=${ARCH}/%.ln}
LINTFLAGS	= -umx ${CPPFLAGS}

CLOBBERFILES	= *.po *.mo

MSG_DOMAIN	= SUNW_INSTALL_LIBSOFT

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

msgs: ${MSG_DOMAIN}.po

${MSG_DOMAIN}.po: ${SRCS} ${HDRS}
	@echo "Making messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} -s \
			-c "i18n:" - >/dev/null 2>&1
 
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
