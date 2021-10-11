#
# Copyright (c) 1995 by Sun Microsystems, Inc.
# All rights reserved.
#
# "@(#)Makefile 1.4 96/06/09 SMI"
#
# SPMI common library makefile
#

LIBRARY	= libspmicommon.a
VERS	= .1

OBJECTS	= \
	common_arch.o \
	common_boolean.o \
	common_client.o \
	common_linklist.o \
	common_misc.o \
	common_mmap.o \
	common_mount.o \
	common_pathcanon.o \
	common_post.o \
	common_proc.o \
	common_process_control.o \
	common_scriptwrite.o \
	common_strlist.o \
	common_util.o

SRCS =	$(OBJECTS:.o=.c)

PRIVHDRS	= \
	spmicommon_lib.h \
	common_strings.h \
	common_linklist_in.h \
	common_process_control_in.h
EXPHDRS		= spmicommon_api.h
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

INCLUDE		 = 
CPPFLAGS	+= ${INCLUDE} -D${ARCH}
CFLAGS		+= $(DEBUG_CFLAGS) -Xa ${CPPFLAGS}
LDFLAGS		+=

LINTERR		= lint_errors
LINTFILES	= ${SRCS:%.c=${ARCH}/%.ln}
LINTFLAGS	= -umx ${CPPFLAGS}

CLOBBERFILES	= *.po *.mo

MSG_DOMAIN	= SUNW_INSTALL_LIBCOMMON

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

common_proc:	objs/$(ARCH)/$(LIBRARY)
		$(CC) -o objs/$(ARCH)/$@ -DMODULE_TEST $@.c $(INCLUDE) \
			objs/$(ARCH)/common_proc.o \
			objs/$(ARCH)/$(LIBRARY)

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
