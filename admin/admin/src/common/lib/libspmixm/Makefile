#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# "@(#)Makefile 1.1 95/10/20 SMI"
#
# SPMI Xm library makefile
#

LIBRARY	= libspmixm.a
VERS	= .1

OBJECTS	= \
	xm_adminhelp.o \
	xm_msg.o \
	xm_utils.o

SRCS =	$(OBJECTS:.o=.c)

PRIVHDRS	= xm_utils.h xm_adminhelp.h
EXPHDRS		= spmixm_api.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

# Set up X information...
MOTIFHOME	:sh= echo "${MOTIFHOME:-/usr/dt}"
OPENWINHOME	:sh= echo "${OPENWINHOME:-/usr/openwin}"
XINCLUDEDIR	= -I${MOTIFHOME}/include -I$(OPENWINHOME)/include

INCLUDE		= -I. \
		  -I${ROOTINCADMIN} \
		  ${XINCLUDEDIR}
CPPFLAGS	+= ${INCLUDE} -D${ARCH}
CFLAGS		+= $(DEBUG_CFLAGS) -Xa ${CPPFLAGS}
LDFLAGS		+=

LINTERR		= lint_errors
LINTFILES	= ${SRCS:%.c=${ARCH}/%.ln}
LINTFLAGS	= -umx ${CPPFLAGS}

CLOBBERFILES	= *.po *.mo

MSG_DOMAIN	= SUNW_INSTALL_LIBXM

.KEEP_STATE:

abc:
	@echo $(ROOTINCADMIN)
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
	${COMPILE.c} -C -E ${SRCS} 2>/dev/null | \
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

cstyle:	$(SRCS) $(PRIVHDRS) $(PUBHDRS)
	cstyle $(SRCS) $(PRIVHDRS) $(PUBHDRS)

include ../Makefile.targ
