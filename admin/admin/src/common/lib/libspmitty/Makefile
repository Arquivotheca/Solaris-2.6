#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# "@(#)Makefile 1.1 95/10/20 SMI"
#
# SPMI Xm library makefile
#

LIBRARY	= libspmitty.a
VERS	= .1

OBJECTS	= \
	tty_color.o \
	tty_help.o \
	tty_init.o \
	tty_list.o \
	tty_menu.o \
	tty_msg.o \
	tty_utils.o \
	tty_wins.o

SRCS =	$(OBJECTS:.o=.c)

PRIVHDRS	= \
	tty_strings.h \
	tty_utils.h
EXPHDRS		= spmitty_api.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

# Set up X information...
#MOTIFHOME	:sh= echo "${MOTIFHOME:-/usr/dt}"
#OPENWINHOME	:sh= echo "${OPENWINHOME:-/usr/openwin}"
#XINCLUDEDIR	= -I${MOTIFHOME}/include -I$(OPENWINHOME)/include

INCLUDE		 = -I. -I${ROOTINCADMIN}
CPPFLAGS	+= ${INCLUDE} -D${ARCH}
CFLAGS		+= $(DEBUG_CFLAGS) -Xa ${CPPFLAGS}
LDFLAGS		+=

LINTERR		= lint_errors
LINTFILES	= ${SRCS:%.c=${ARCH}/%.ln}
LINTFLAGS	= -umx ${CPPFLAGS}

CLOBBERFILES	= *.po *.mo

MSG_DOMAIN	= SUNW_INSTALL_LIBTTY

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

#
# Since the library may use strings exported from the libraries,
# we could actually end up generating 'extraneous' .po files here
# (e.g. SUNW_INSTALL_LIBAPP.po).
# We delete all but the library's .po here.
#
msgs: ${MSG_DOMAIN}.po

${MSG_DOMAIN}.po: ${SRCS} ${HDRS}
	@echo "Making messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} -s \
			-c "i18n:" - >/dev/null 2>&1
	@for i in *.po; do \
		if test $${i} != ${MSG_DOMAIN}.po; then \
			echo "deleting $${i}"; \
			rm -f $${i}; \
		fi; \
	done;

# Create a message file with "translated" test strings.
# Turn the po file into an mo file ready for testing.
msgs_test: ${SRCS} ${HDRS}
	@echo "Making test messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} -s \
			-c "i18n:" -m "öýê!" - >/dev/null 2>&1
	@for i in *.po; do \
		if test $${i} != ${MSG_DOMAIN}.po; then \
			echo "deleting $${i}"; \
			rm -f $${i}; \
		fi; \
	done;
	@msgfmt -o ${MSG_DOMAIN}.mo ${MSG_DOMAIN}.po

lint:  ${SRCS} ${HDRS}
	${LINT.c} ${SRCS}

cstyle:	$(SRCS) $(HDRS)
	cstyle $(SRCS) $(HDRS)

depend:
	makedepend -- ${CPPFLAGS} ${CFLAGS} -- ${SRCS}

include ../Makefile.targ
# DO NOT DELETE THIS LINE -- make depend depends on it.
# makedepend dependencies modified using:
# /usr/rmtc/misc/spmi/bin/deps.pl, Version: 1.2
# on Tue Apr  9 14:45:21 US/Mountain 1996

${ARCH}/tty_color.o: spmitty_api.h
${ARCH}/tty_color.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_color.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_color.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_color.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_color.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_color.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_color.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_color.o: tty_utils.h
${ARCH}/tty_help.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_help.o: spmitty_api.h
${ARCH}/tty_help.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_help.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_help.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_help.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_help.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_help.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_help.o: tty_utils.h tty_strings.h
${ARCH}/tty_init.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_init.o: spmitty_api.h
${ARCH}/tty_init.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_init.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_init.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_init.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_init.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_init.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_init.o: tty_utils.h tty_strings.h
${ARCH}/tty_intro.o: spmitty_api.h
${ARCH}/tty_intro.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_intro.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_intro.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_intro.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_intro.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_intro.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_intro.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_intro.o: tty_utils.h tty_strings.h
${ARCH}/tty_menu.o: spmitty_api.h
${ARCH}/tty_menu.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_menu.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_menu.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_menu.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_menu.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_menu.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_menu.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_menu.o: tty_utils.h
${ARCH}/tty_msg.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_msg.o: spmitty_api.h
${ARCH}/tty_msg.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_msg.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_msg.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_msg.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_msg.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_msg.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_msg.o: tty_utils.h
${ARCH}/tty_utils.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_utils.o: spmitty_api.h
${ARCH}/tty_utils.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_utils.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_utils.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_utils.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_utils.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_utils.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_utils.o: tty_utils.h tty_strings.h
${ARCH}/tty_wins.o: ${ROOTINCADMIN}/spmicommon_api.h
${ARCH}/tty_wins.o: spmitty_api.h
${ARCH}/tty_wins.o: ${ROOTINCADMIN}/spmiapp_api.h
${ARCH}/tty_wins.o: ${ROOTINCADMIN}/spmistore_api.h
${ARCH}/tty_wins.o: ${ROOTINCADMIN}/spmisoft_api.h
${ARCH}/tty_wins.o: ${ROOTINCADMIN}/soft_hash.h
${ARCH}/tty_wins.o: ${ROOTINCADMIN}/spmiapp_strings.h
${ARCH}/tty_wins.o: ${ROOTINCADMIN}/spmiapp_ui_msg.h
${ARCH}/tty_wins.o: tty_utils.h tty_strings.h
