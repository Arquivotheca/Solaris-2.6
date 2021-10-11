#
# Copyright (c) 1995-1996 by Sun Microsystems, Inc.
# All rights reserved.
#
# "@(#)Makefile 1.11 96/06/17 SMI"
#
# SPMI application library makefile
#

LIBRARY	= libspmiapp.a
VERS	= .1

OBJECTS	= \
	app_bootobj.o \
	app_dsr.o \
	app_disks.o \
	app_lfs.o \
	app_params.o \
	app_profile.o \
	app_sw.o \
	app_ui_msg.o \
	app_upgrade.o \
	app_usedisks.o \
	app_utils.o

SRCS =	$(OBJECTS:.o=.c)

PRIVHDRS	= spmiapp_lib.h app_strings.h app_utils.h
EXPHDRS		= spmiapp_api.h spmiapp_strings.h spmiapp_ui_msg.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

INCLUDE		 = -I. \
		   -I../libspmisvc \
		   -I../libspmisoft \
		   -I../libspmistore \
		   -I../libspmicommon
CPPFLAGS	+= ${INCLUDE} -D${ARCH}
CFLAGS		+= $(DEBUG_CFLAGS) -Xa ${CPPFLAGS}
LDFLAGS		+=

LINTERR		= lint_errors
LINTFILES	= ${SRCS:%.c=${ARCH}/%.ln}
LINTFLAGS	= -umx ${CPPFLAGS}

CLOBBERFILES	= *.po *.mo

MSG_DOMAIN	= SUNW_INSTALL_LIBAPP

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
# This library defines strings that are to be 'exported' for
# use by applications.  The strings are to be extracted into this
# library's msg file and not the user's.  Since the library itself
# may not use the strings, we have to do some funky processing in order
# to ensure that xgettext pulls all the strings into the msg file.  In
# order to do this, the strings must appear dgettext wrapped somewhere
# outside of the #define that defines them (i.e. they have to appear in
# what looks like in-line C code).  We do this by generating a
# psuedo-C file with each string in it such that xgettext will pull
# it out.
# 1. replace everything in the header files that looks like:
#    #define XXX LIBAPPSTR
#	with
#    LIBAPPSTR
#    This is enough to make it look like 'in-line' code so cpp
#    will expand the LIBAPPSTR macro and xgettext can pull out the
#    message.
#    Note that this method works for multi-line defines and it pulls
#    out any related i18n comments too.
# 2. remove #include lines:  floowing is a short example that shows why...
#      ===========================================
#      a.h
#      ===========================================
#      #ifndef _a_h
#      #define _a_h
#
#      #define	APP_STR(x)	dgettext("SUNW_INSTALL_LIBAPP", x)
#
#      /*
#       * This include has to go so that the ones below in the actual b.h that
#       * we already did a sed replacement on will get looked at.
#       * If it stays, then the '#ifndef _b_h' wrapper will cause our sed
#       * replaced ones not to get looked in the compile phase.
#       */
#      #include "b.h"
#
#      #endif /* _a_h */
#
#      ===========================================
#      b.h
#      ===========================================
#      #ifndef _b_h
#      #define _b_h
#
#      #include "a.h"
#
#      /* i18n: comment */
#      #define A_SHORT_MSG     APP_STR("appstr_hello")
#
#      endif /* _b_h */
#
LIBAPPSTR_DUMMY_FILE=_app_dummy_msg.c

msgs: ${MSG_DOMAIN}.po

${MSG_DOMAIN}.po: ${SRCS} ${HDRS}
	@echo "Generating LIBAPPSTR .c file"
	@-rm -f ${LIBAPPSTR_DUMMY_FILE}
	@cat ${EXPHDRS} | \
		sed -e 's/^#define[ 	][ 	]*[A-Za-z_][A-Za-z_]*[ 	][ 	]*LIBAPPSTR/LIBAPPSTR/' | \
		sed -e '/^#include[ 	][ 	]*/d' \
		>> ${LIBAPPSTR_DUMMY_FILE}
	@echo "Making messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${LIBAPPSTR_DUMMY_FILE} ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} \
			-c "i18n:" - >/dev/null 2>&1
	@-rm -f ${LIBAPPSTR_DUMMY_FILE}

# Create a message file with "translated" test strings.
# Turn the po file into an mo file ready for testing.
msgs_test: ${SRCS} ${HDRS}
	@echo "Generating LIBAPPSTR .c file"
	@-rm -f ${LIBAPPSTR_DUMMY_FILE}
	@cat ${EXPHDRS} | \
		sed -e 's/^#define[ 	][ 	]*[A-Za-z_][A-Za-z_]*[ 	][ 	]*LIBAPPSTR/LIBAPPSTR/' | \
		sed -e '/^#include[ 	][ 	]*/d' \
		>> ${LIBAPPSTR_DUMMY_FILE}
	@echo "Making messages file ${MSG_DOMAIN}.po"
	@${COMPILE.c} -C -E ${LIBAPPSTR_DUMMY_FILE} ${SRCS} 2>/dev/null | \
		xgettext -d ${MSG_DOMAIN} \
			-c "i18n:" -m "öýê!" - >/dev/null 2>&1
	@-rm -f ${LIBAPPSTR_DUMMY_FILE}
	@msgfmt -o ${MSG_DOMAIN}.mo ${MSG_DOMAIN}.po

lint:  ${SRCS} ${HDRS}
	${LINT.c} ${SRCS}

cstyle:	$(SRCS) $(PRIVHDRS) $(EXPHDRS)
	cstyle $(SRCS) $(PRIVHDRS) $(EXPHDRS)

include ../Makefile.targ
